/* Copyright (c) 2021 Intel Corporation

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

#ifndef ITEX_CORE_UTILS_MUTEX_DATA_H_
#define ITEX_CORE_UTILS_MUTEX_DATA_H_

namespace itex {
namespace internal {

// The internal state of a mutex.
struct MuData {
  void* space[2];
};

// The internal state of a condition_variable.
struct CVData {
  void* space[2];
};

}  // namespace internal
}  // namespace itex

#endif  // ITEX_CORE_UTILS_MUTEX_DATA_H_
