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

#ifndef ITEX_CORE_KERNELS_GPU_AGGREGATE_OPS_H_
#define ITEX_CORE_KERNELS_GPU_AGGREGATE_OPS_H_

#include "itex/core/utils/tensor_types.h"
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"

namespace itex {
namespace functor {
// Functor definitions for Aggregate ops, must be compilable by nvcc.
template <typename Device, typename T>
struct Add2EigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::Flat out,
                      typename TTypes<T>::ConstFlat in1,
                      typename TTypes<T>::ConstFlat in2) {
    out.device(d) = in1 + in2;
  }
};

template <typename Device, typename T>
struct Add2Functor {
  void operator()(const Device& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstFlat in1,
                  typename TTypes<T>::ConstFlat in2) {
    Add2EigenImpl<Device, T>::Compute(d, out, in1, in2);
  }
};

template <typename Device, typename T>
struct Add3EigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::Flat out,
                      typename TTypes<T>::ConstFlat in1,
                      typename TTypes<T>::ConstFlat in2,
                      typename TTypes<T>::ConstFlat in3) {
    out.device(d) = in1 + in2 + in3;
  }
};

template <typename Device, typename T>
struct Add3Functor {
  void operator()(const Device& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstFlat in1,
                  typename TTypes<T>::ConstFlat in2,
                  typename TTypes<T>::ConstFlat in3) {
    Add3EigenImpl<Device, T>::Compute(d, out, in1, in2, in3);
  }
};

template <typename Device, typename T>
struct Add4EigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::Flat out,
                      typename TTypes<T>::ConstFlat in1,
                      typename TTypes<T>::ConstFlat in2,
                      typename TTypes<T>::ConstFlat in3,
                      typename TTypes<T>::ConstFlat in4) {
    out.device(d) = in1 + in2 + in3 + in4;
  }
};

template <typename Device, typename T>
struct Add4Functor {
  void operator()(const Device& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstFlat in1,
                  typename TTypes<T>::ConstFlat in2,
                  typename TTypes<T>::ConstFlat in3,
                  typename TTypes<T>::ConstFlat in4) {
    Add4EigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4);
  }
};

template <typename Device, typename T>
struct Add5EigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::Flat out,
                      typename TTypes<T>::ConstFlat in1,
                      typename TTypes<T>::ConstFlat in2,
                      typename TTypes<T>::ConstFlat in3,
                      typename TTypes<T>::ConstFlat in4,
                      typename TTypes<T>::ConstFlat in5) {
    out.device(d) = in1 + in2 + in3 + in4 + in5;
  }
};

template <typename Device, typename T>
struct Add5Functor {
  void operator()(const Device& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstFlat in1,
                  typename TTypes<T>::ConstFlat in2,
                  typename TTypes<T>::ConstFlat in3,
                  typename TTypes<T>::ConstFlat in4,
                  typename TTypes<T>::ConstFlat in5) {
    Add5EigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4, in5);
  }
};

template <typename Device, typename T>
struct Add6EigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::Flat out,
                      typename TTypes<T>::ConstFlat in1,
                      typename TTypes<T>::ConstFlat in2,
                      typename TTypes<T>::ConstFlat in3,
                      typename TTypes<T>::ConstFlat in4,
                      typename TTypes<T>::ConstFlat in5,
                      typename TTypes<T>::ConstFlat in6) {
    out.device(d) = in1 + in2 + in3 + in4 + in5 + in6;
  }
};

template <typename Device, typename T>
struct Add6Functor {
  void operator()(const Device& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstFlat in1,
                  typename TTypes<T>::ConstFlat in2,
                  typename TTypes<T>::ConstFlat in3,
                  typename TTypes<T>::ConstFlat in4,
                  typename TTypes<T>::ConstFlat in5,
                  typename TTypes<T>::ConstFlat in6) {
    Add6EigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4, in5, in6);
  }
};

template <typename Device, typename T>
struct Add7EigenImpl {
  static void Compute(const Device& d, typename TTypes<T>::Flat out,
                      typename TTypes<T>::ConstFlat in1,
                      typename TTypes<T>::ConstFlat in2,
                      typename TTypes<T>::ConstFlat in3,
                      typename TTypes<T>::ConstFlat in4,
                      typename TTypes<T>::ConstFlat in5,
                      typename TTypes<T>::ConstFlat in6,
                      typename TTypes<T>::ConstFlat in7) {
    out.device(d) = in1 + in2 + in3 + in4 + in5 + in6 + in7;
  }
};

template <typename Device, typename T>
struct Add7Functor {
  void operator()(const Device& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstFlat in1,
                  typename TTypes<T>::ConstFlat in2,
                  typename TTypes<T>::ConstFlat in3,
                  typename TTypes<T>::ConstFlat in4,
                  typename TTypes<T>::ConstFlat in5,
                  typename TTypes<T>::ConstFlat in6,
                  typename TTypes<T>::ConstFlat in7) {
    Add7EigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4, in5, in6,
                                      in7);
  }
};

template <typename Device, typename T>
struct Add8EigenImpl {
  static void Compute(
      const Device& d, typename TTypes<T>::Flat out,
      typename TTypes<T>::ConstFlat in1, typename TTypes<T>::ConstFlat in2,
      typename TTypes<T>::ConstFlat in3, typename TTypes<T>::ConstFlat in4,
      typename TTypes<T>::ConstFlat in5, typename TTypes<T>::ConstFlat in6,
      typename TTypes<T>::ConstFlat in7, typename TTypes<T>::ConstFlat in8) {
    out.device(d) = in1 + in2 + in3 + in4 + in5 + in6 + in7 + in8;
  }
};

template <typename Device, typename T>
struct Add8Functor {
  void operator()(
      const Device& d, typename TTypes<T>::Flat out,
      typename TTypes<T>::ConstFlat in1, typename TTypes<T>::ConstFlat in2,
      typename TTypes<T>::ConstFlat in3, typename TTypes<T>::ConstFlat in4,
      typename TTypes<T>::ConstFlat in5, typename TTypes<T>::ConstFlat in6,
      typename TTypes<T>::ConstFlat in7, typename TTypes<T>::ConstFlat in8) {
    Add8EigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4, in5, in6, in7,
                                      in8);
  }
};

template <typename Device, typename T>
struct Add8pEigenImpl {
  static void Compute(
      const Device& d, typename TTypes<T>::Flat out,
      typename TTypes<T>::ConstFlat in1, typename TTypes<T>::ConstFlat in2,
      typename TTypes<T>::ConstFlat in3, typename TTypes<T>::ConstFlat in4,
      typename TTypes<T>::ConstFlat in5, typename TTypes<T>::ConstFlat in6,
      typename TTypes<T>::ConstFlat in7, typename TTypes<T>::ConstFlat in8) {
    out.device(d) += in1 + in2 + in3 + in4 + in5 + in6 + in7 + in8;
  }
};

// Add8p is like Add8 except the underlying implementation should +=
// rather than assign to the output.
template <typename Device, typename T>
struct Add8pFunctor {
  void operator()(
      const Device& d, typename TTypes<T>::Flat out,
      typename TTypes<T>::ConstFlat in1, typename TTypes<T>::ConstFlat in2,
      typename TTypes<T>::ConstFlat in3, typename TTypes<T>::ConstFlat in4,
      typename TTypes<T>::ConstFlat in5, typename TTypes<T>::ConstFlat in6,
      typename TTypes<T>::ConstFlat in7, typename TTypes<T>::ConstFlat in8) {
    Add8pEigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4, in5, in6,
                                       in7, in8);
  }
};

template <typename Device, typename T>
struct Add9EigenImpl {
  static void Compute(
      const Device& d, typename TTypes<T>::Flat out,
      typename TTypes<T>::ConstFlat in1, typename TTypes<T>::ConstFlat in2,
      typename TTypes<T>::ConstFlat in3, typename TTypes<T>::ConstFlat in4,
      typename TTypes<T>::ConstFlat in5, typename TTypes<T>::ConstFlat in6,
      typename TTypes<T>::ConstFlat in7, typename TTypes<T>::ConstFlat in8,
      typename TTypes<T>::ConstFlat in9) {
    out.device(d) = in1 + in2 + in3 + in4 + in5 + in6 + in7 + in8 + in9;
  }
};

template <typename Device, typename T>
struct Add9Functor {
  void operator()(
      const Device& d, typename TTypes<T>::Flat out,
      typename TTypes<T>::ConstFlat in1, typename TTypes<T>::ConstFlat in2,
      typename TTypes<T>::ConstFlat in3, typename TTypes<T>::ConstFlat in4,
      typename TTypes<T>::ConstFlat in5, typename TTypes<T>::ConstFlat in6,
      typename TTypes<T>::ConstFlat in7, typename TTypes<T>::ConstFlat in8,
      typename TTypes<T>::ConstFlat in9) {
    Add9EigenImpl<Device, T>::Compute(d, out, in1, in2, in3, in4, in5, in6, in7,
                                      in8, in9);
  }
};
}  // namespace functor
}  // namespace itex

#endif  // ITEX_CORE_KERNELS_GPU_AGGREGATE_OPS_H_
