/* Copyright (c) 2021-2022 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

///////////////////////////////////////////////////////////////////////////
// Legacy code to be compatible with the pb generated by Intel Quantization
// Tools, such as QuantizedMatMulWithBias,
// QuantizedMatMulWithBiasAndReluAndRequantize,.etc
///////////////////////////////////////////////////////////////////////////

// Implements a quantized eight-bit version of the matmul operation with bias,
// relu and requantization fusion support utilizing OneDnn u8s8s32 inner
// product API. Right now, this version can support
//   - Input: quantized as uint8 via either MIN_FIRST or SCALE mode.
//            SCALE mode is selected when input is guaranteed to be non-
//            negative, e.g., MatMul is fed by Relu. Otherwise, MIN_FIRST is
//            selected.
//   - Weight: quantized to int8 via SCALE mode.
//   - Bias: float32/int32. For int32, it is quantized according to input and
//           filter min-max values.
// Other than that, this op does not support other input combination yet.
// When input is quantized to uint8 via MIN_FIRST, bias needs compensation.
// The detailed algorithm is illustrated as below:
//
// Af32 is the original fp32 activation 2D tensor.
// Min(Af32) is the minimum scalar value of Af32.
// Max(Af32) is the maximum scalar value of Af32.
// Qa is the quantization scale for activation.
// Au8 is the quantized unsigned int8 activation tensor.
// With SCALE quantization (used for non-negative Af32), Qa and Au8 can be
// calculated as below:
//    Qa = 255.0 / Max(Af32)
//    Au8 = round(Qa * Af32).
// With MIN_FIRST quantization, Q'a and A'u8 can be calculated as below:
//    Q'a = 255.0 / (Max(Af32) - Min(Af32))
//    A'u8 = round(Q'a * (Af32 - Min(Af32) * ones(Af32))),
// where, ones(.) is a tensor of all 1s with the same shape of its argument and
// round(.) rounds a number to its nearest integer.
//
// Wf32 is the original fp32 2D weight tensor.
// MaxAbs(Wf32) is the maximum absolute scalar value of Wf32.
// Qw is the quantization scale of weight.
// Ws8 is the quantized signed int8 weight tensor.
// Qw and Ws8 can be calculated as below:
//    Qw = 127.0 / MaxAbs(Wf32)
//    Ws8 = round(Qw * Wf32).
//
// Bf32 is the original fp32 1D bias tensor matching the innermost dim of
// Wf32.
// With SCALE quantization of activation, the scaled bias, Bs32, is calculated
// as below:
//      Bs32 = Qa * Qw * Bf32.
// With MIN_FIRST quantization of activation, the scaled bias tensor with
// compensation, B's32, is calculated as below:
//      B's32 = Q'a * Qw * Bf32 + Q'a * Qw * Min(Af32) * 1 * Wf32
//            = Q'a * Qw * Bf32 + Q'a * Min(Af32) * 1 * Ws8.
// where, 1 denotes a row vector matching the outermost dim of Wf32.
//
// The QuantizedMatMulWithBias op calculates 32bit integer output as below:
//  - with SCALE activation quantization:
//    Xs32 = Au8 * Ws8 + 1' * Bs32
//         = Qa * Qw * Af32 * Wf32  + Qa * Qw * 1' * Bf32
//         = Qa * Qw * (Af32 * Wf32 + 1' * Bf32) = Qa * Qw * Xf32,
//    where, 1' denotes a column vector matching the outermost dim of Af32 and
//    Xf32 represents the output of original fp32 MatMul with BiasAdd fusion.
//
//  - with MIN_FIRST activation quantization:
//    Xs32 = A'u8 * Ws8 + 1' * B's32
//         = Q'a * (Af32 - Min(Af32) * ones(Af32)) * Qw * Wf32 +
//           Q'a * Qw * 1' * Bf32 + Q'a * Qw * Min(Af32) * 1' * 1 * Wf32
//         = Q'a * Qw * (Af32 * Wf32 + 1' * Bf32)
//         = Q'a * Qw * Xf32.
//    Note that 1' * 1 = ones(Af32).
//
// The QuantizedMatMulWithBiasAndRelu op does the same calculation as above
// except adding relu function for the 32bit integer output.
//
// The QuantizedMatMulWithBiasAndReluAndRequantize op does one more step of
// requantize calculation based on above. Since the fusion ends with a Relu the
// activation Xf32 at Relu, in the original fp32 graph, is guaranteed to be
// non-negative. The requantize scale Qr is calculated from offline calibration.
//    Qr = 255 / Max(Xf32)
//    Xu8 = Qr * Xf32.
//
// More information of this implementation can be found in
// https://software.intel.com/en-us/articles/lower-numerical-precision-deep-learning-inference-and-training
#include "itex/core/kernels/common/no_ops.h"
#include "itex/core/kernels/legacy/matmul_common.h"
#include "itex/core/kernels/onednn/block/quantized_ops.h"
#include "itex/core/utils/errors.h"
#include "itex/core/utils/onednn/onednn_layout_util.h"
#include "itex/core/utils/onednn/onednn_post_op_util.h"
#include "itex/core/utils/onednn/onednn_util.h"
#include "itex/core/utils/op_kernel.h"
#include "itex/core/utils/op_requires.h"
#include "itex/core/utils/plugin_tensor.h"
#include "itex/core/utils/quantization_util.h"
#include "itex/core/utils/register_types.h"
#include "itex/core/utils/types.h"
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"

namespace itex {

using memory = dnnl::memory;

template <typename Device, typename Tinput, typename Tweight, typename Tbias,
          typename Toutput>
class OneDnnQuantizedMatMulOp
    : public LegacyOneDnnQuantizedMatMulOpBase<Device, Tinput, Tweight, Tbias,
                                               Toutput> {
 public:
  explicit OneDnnQuantizedMatMulOp(OpKernelConstruction* context)
      : LegacyOneDnnQuantizedMatMulOpBase<Device, Tinput, Tweight, Tbias,
                                          Toutput>(context) {
    // Quantize mode assignment
    string mode_string;
    OP_REQUIRES_OK(context, context->GetAttr("input_quant_mode", &mode_string));
    if (mode_string == "MIN_FIRST") {
      this->mode_ = QuantizeMode::MIN_FIRST;
    } else if (mode_string == "SCALED") {
      this->mode_ = QuantizeMode::SCALED;
    } else {
      context->CtxFailure(errors::InvalidArgument(
          "Quantization mode must be either MIN_FIRST or SCALED, but received ",
          mode_string));
    }

    // weight/bias const flag set
    if (context->HasAttr("is_weight_const")) {
      OP_REQUIRES_OK(context, context->GetAttr("is_weight_const",
                                               &(this->is_weight_const_)));
    }
    this->is_bias_const_ = true;

    // PostOpUtil set
    std::vector<string> fused_ops;
    fused_ops.push_back("Quantized");
    fused_ops.push_back("BiasAdd");
    OP_REQUIRES(context, this->post_op_util_.AddOps(fused_ops),
                errors::InvalidArgument(
                    "Found unsupported fusion in QuantizedMatMul."));

    OP_REQUIRES_OK(context,
                   context->GetAttr("transpose_a", &this->transpose_a_));
    OP_REQUIRES_OK(context,
                   context->GetAttr("transpose_b", &this->transpose_b_));

    // Set input/output tensor index
    this->kSrcMinRangeIndex = 3;
    this->kSrcMaxRangeIndex = 4;
    this->kFilterMinRangeIndex = 5;
    this->kFilterMaxRangeIndex = 6;
    this->kMinFreezedIndex = 7;
    this->kMaxFreezedIndex = 8;
    this->kDstMinRangeIndex = 1;
    this->kDstMaxRangeIndex = 2;
  }

  void Compute(OpKernelContext* context) override {
    LegacyOneDnnQuantizedMatMulOpBase<Device, Tinput, Tweight, Tbias,
                                      Toutput>::Compute(context);
  }

  void ExtendInt8PostOps(OpKernelContext* context) override {
    // When the output type is quint8, the output data is requantized into
    // quint8. A post_op "output_scale" is added to do the conversion.
    if (std::is_same<Toutput, quint8>::value ||
        std::is_same<Toutput, qint8>::value ||
        std::is_same<Toutput, float>::value ||
        std::is_same<Toutput, Eigen::bfloat16>::value) {
      float min_output_value;
      float max_output_value;
      this->ComputeOutputRangeForInt32(context, &min_output_value,
                                       &max_output_value);
      float scale_int32 =
          std::max(std::abs(min_output_value), std::abs(max_output_value));
      const float min_freezed_output =
          context->input(this->kMinFreezedIndex).template flat<float>()(0);
      const float max_freezed_output =
          context->input(this->kMaxFreezedIndex).template flat<float>()(0);
      float scale_eightbit =
          std::max(std::abs(min_freezed_output), std::abs(max_freezed_output));
      float scale = 1.0;
      if (std::is_same<Toutput, quint8>::value) {
        scale = scale_int32 / scale_eightbit / static_cast<float>(1u << 23);
      } else if (std::is_same<Toutput, qint8>::value) {
        scale = scale_int32 / scale_eightbit / static_cast<float>(1u << 24);
      } else if (std::is_same<Toutput, float>::value ||
                 std::is_same<Toutput, Eigen::bfloat16>::value) {
        scale = scale_int32 / static_cast<float>(1u << 31);
      } else {
        // TODO(itex): keeping the default qint8 as before. Change to error
        // later.
        scale = scale_int32 / scale_eightbit / static_cast<float>(1u << 24);
      }
      this->post_op_util_.SetOutputScale({scale});
    }
  }
};

template <typename Device, typename Tinput, typename Tweight, typename Tbias,
          typename Toutput>
class OneDnnQuantizedMatMulReluOp
    : public OneDnnQuantizedMatMulOp<Device, Tinput, Tweight, Tbias, Toutput> {
 public:
  explicit OneDnnQuantizedMatMulReluOp(OpKernelConstruction* context)
      : OneDnnQuantizedMatMulOp<Device, Tinput, Tweight, Tbias, Toutput>(
            context) {
    std::vector<string> fused_ops;
    fused_ops.push_back("Relu");
    OP_REQUIRES(context, this->post_op_util_.AddOps(fused_ops),
                errors::InvalidArgument(
                    "Found unsupported fusion in QuantizedMatMulRelu."));
  }

 protected:
  void ExtendInt8PostOps(OpKernelContext* context) override {
    OneDnnQuantizedMatMulOp<Device, quint8, qint8, Tbias,
                            Toutput>::ExtendInt8PostOps(context);
    this->post_op_util_.SetPostOpScale("Relu", 1.0);
  }
};

#ifdef INTEL_CPU_ONLY
#define REGISTER_ONEDNN_KERNEL(op, kernel, bias_type, output_type)     \
  REGISTER_KERNEL_BUILDER(                                             \
      Name(op)                                                         \
          .Device(DEVICE_CPU)                                          \
          .TypeConstraint<quint8>("T1")                                \
          .TypeConstraint<qint8>("T2") BIAS_TYPE_CONSTRAINT(bias_type) \
          .TypeConstraint<output_type>("Toutput"),                     \
      kernel TEMPLATE_ARGS(CPUDevice, quint8, qint8, bias_type, output_type));
#else
#define REGISTER_ONEDNN_KERNEL(op, kernel, bias_type, output_type)     \
  REGISTER_KERNEL_BUILDER(                                             \
      Name(op)                                                         \
          .Device(DEVICE_GPU)                                          \
          .TypeConstraint<quint8>("T1")                                \
          .TypeConstraint<qint8>("T2") BIAS_TYPE_CONSTRAINT(bias_type) \
          .TypeConstraint<output_type>("Toutput") HOSTMEMORYLIST,      \
      kernel TEMPLATE_ARGS(GPUDevice, quint8, qint8, bias_type, output_type));
#endif  // INTEL_CPU_ONLY

#define REGISTER_ONEDNN_KERNEL_ALL_BIAS_TYPES(op, kernel, output_type) \
  REGISTER_ONEDNN_KERNEL(op, kernel, float, output_type)               \
  REGISTER_ONEDNN_KERNEL(op, kernel, qint32, output_type);

// Concrete OneDnn MatMul INT8 kernel implementation
#define TEMPLATE_ARGS(Device, quint8, qint8, bias_type, output_type) \
<Device, quint8, qint8, bias_type, output_type>

#define BIAS_TYPE_CONSTRAINT(bias_type)
#define HOSTMEMORYLIST                                                \
  .HostMemoryList4("min_a", "max_a", "min_b", "max_b")                \
      .HostMemoryList7("a_meta", "b_meta", "bias_meta", "min_a_meta", \
                       "max_a_meta", "min_b_meta", "max_b_meta")      \
      .HostMemoryList2("min_out", "max_out")                          \
      .HostMemoryList3("out_meta", "min_out_meta", "max_out_meta")
REGISTER_ONEDNN_KERNEL("_OneDnnQuantizedMatMulWithBiasAndRelu",
                       OneDnnQuantizedMatMulReluOp, float, qint32);
#undef HOSTMEMORYLIST
#undef BIAS_TYPE_CONSTRAINT

#define BIAS_TYPE_CONSTRAINT(bias_type) .TypeConstraint<bias_type>("Tbias")
#define HOSTMEMORYLIST                                                \
  .HostMemoryList4("min_a", "max_a", "min_b", "max_b")                \
      .HostMemoryList7("a_meta", "b_meta", "bias_meta", "min_a_meta", \
                       "max_a_meta", "min_b_meta", "max_b_meta")      \
      .HostMemoryList2("min_out", "max_out")                          \
      .HostMemoryList3("out_meta", "min_out_meta", "max_out_meta")
REGISTER_ONEDNN_KERNEL_ALL_BIAS_TYPES("_OneDnnQuantizedMatMulWithBias",
                                      OneDnnQuantizedMatMulOp, qint32);
#undef HOSTMEMORYLIST

#define HOSTMEMORYLIST                                                       \
  .HostMemoryList6("min_a", "max_a", "min_b", "max_b", "min_freezed_output", \
                   "max_freezed_output")                                     \
      .HostMemoryList9("a_meta", "b_meta", "bias_meta", "min_a_meta",        \
                       "max_a_meta", "min_b_meta", "max_b_meta",             \
                       "min_freezed_output_meta", "max_freezed_output_meta") \
      .HostMemoryList2("min_out", "max_out")                                 \
      .HostMemoryList3("out_meta", "min_out_meta", "max_out_meta")
REGISTER_ONEDNN_KERNEL_ALL_BIAS_TYPES(
    "_OneDnnQuantizedMatMulWithBiasAndReluAndRequantize",
    OneDnnQuantizedMatMulReluOp, quint8);
REGISTER_ONEDNN_KERNEL_ALL_BIAS_TYPES(
    "_OneDnnQuantizedMatMulWithBiasAndRequantize", OneDnnQuantizedMatMulOp,
    quint8);
#undef HOSTMEMORYLIST

#define HOSTMEMORYLIST                                                       \
  .HostMemoryList6("min_a", "max_a", "min_b", "max_b", "min_freezed_output", \
                   "max_freezed_output")                                     \
      .HostMemoryList9("a_meta", "b_meta", "bias_meta", "min_a_meta",        \
                       "max_a_meta", "min_b_meta", "max_b_meta",             \
                       "min_freezed_output_meta", "max_freezed_output_meta") \
      .HostMemoryList1("out_meta")
REGISTER_ONEDNN_KERNEL_ALL_BIAS_TYPES(
    "_OneDnnQuantizedMatMulWithBiasAndDequantize", OneDnnQuantizedMatMulOp,
    float);
REGISTER_ONEDNN_KERNEL_ALL_BIAS_TYPES(
    "_OneDnnQuantizedMatMulWithBiasAndDequantize", OneDnnQuantizedMatMulOp,
    Eigen::bfloat16);
#undef HOSTMEMORYLIST
#undef BIAS_TYPE_CONSTRAINT

#undef TEMPLATE_ARGS

}  // namespace itex
