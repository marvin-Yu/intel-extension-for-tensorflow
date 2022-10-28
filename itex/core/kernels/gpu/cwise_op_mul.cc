/* Copyright (c) 2021-2022 Intel Corporation

Copyright 2015 The TensorFlow Authors. All Rights Reserved.

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

#include "itex/core/kernels/common/cwise_ops_common.h"

namespace itex {

REGISTER6(BinaryOp, GPU, "Mul", functor::mul, int64, float, Eigen::half,
          itex::uint8, complex64, Eigen::bfloat16);

#ifdef ITEX_ENABLE_DOUBLE
REGISTER2(BinaryOp, GPU, "Mul", functor::mul, double, complex128);
#endif  // ITEX_ENABLE_DOUBLE
REGISTER4(BinaryOp, GPU, "MulNoNan", functor::mul_no_nan, Eigen::half, float,
          Eigen::bfloat16, complex64);

#ifdef ITEX_ENABLE_DOUBLE
REGISTER2(BinaryOp, GPU, "MulNoNan", functor::mul_no_nan, double, complex128);
#endif  // ITEX_ENABLE_DOUBLE

REGISTER_KERNEL_BUILDER(Name("Mul")
                            .Device(DEVICE_GPU)
                            .HostMemory("x")
                            .HostMemory("y")
                            .HostMemory("z")
                            .TypeConstraint<int32>("T"),
                        BinaryOp<CPUDevice, functor::mul<int32>>);
}  // namespace itex
