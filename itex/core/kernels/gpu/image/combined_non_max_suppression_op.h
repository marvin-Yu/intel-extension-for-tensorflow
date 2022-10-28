/* Copyright (c) 2021-2022 Intel Corporation

Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#ifndef ITEX_CORE_KERNELS_GPU_IMAGE_COMBINED_NON_MAX_SUPPRESSION_OP_H_
#define ITEX_CORE_KERNELS_GPU_IMAGE_COMBINED_NON_MAX_SUPPRESSION_OP_H_

#include "itex/core/utils/op_kernel.h"
#include "itex/core/utils/plugin_tensor.h"
#include "itex/core/utils/tensor_types.h"
#include "itex/core/utils/types.h"
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"

namespace itex {
namespace functor {

typedef Eigen::GpuDevice GPUDevice;

template <typename Device>
struct CombinedNonMaxSuppressionFunctor;

template <>
struct CombinedNonMaxSuppressionFunctor<GPUDevice> {
  void operator()(OpKernelContext* context, const Tensor& inp_boxes,
                  const Tensor& inp_scores, int max_size_per_class,
                  int max_total_size_per_batch, float iou_threshold,
                  float score_threshold, bool pad_per_class, bool clip_boxes);
};

}  // namespace functor
}  // namespace itex

#endif  // ITEX_CORE_KERNELS_GPU_IMAGE_COMBINED_NON_MAX_SUPPRESSION_OP_H_
