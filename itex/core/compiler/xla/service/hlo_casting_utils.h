/* Copyright (c) 2023 Intel Corporation

Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

// Casting utility functions for HLO instructions.

#ifndef ITEX_CORE_COMPILER_XLA_SERVICE_HLO_CASTING_UTILS_H_
#define ITEX_CORE_COMPILER_XLA_SERVICE_HLO_CASTING_UTILS_H_

#include <type_traits>

#include "itex/core/compiler/xla/service/hlo_instruction.h"
#include "itex/core/utils/logging.h"

namespace itex_xla {

template <class T>
using EnableIfDerivedFromHlo =
    typename std::enable_if<std::is_base_of<HloInstruction, T>::value>::type;

// TODO(b/93238915): Switch implementation from C++'s dynamic_cast to LLVM-like
// RTTI if it turns out to be a performance issue.
// Casts an HloInstruction pointer to one of its subclasses, dies if argument is
// nullptr or runtime information does not match.
//
// Similar to LLVM's cast.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
const T* Cast(const HloInstruction* instruction) {
  ITEX_CHECK(instruction != nullptr);
  const T* casted = dynamic_cast<const T*>(instruction);
  ITEX_CHECK(casted != nullptr)
      << "Invalid HloInstruction casting. Destination type: "
      << typeid(T).name() << ". Instruction: " << instruction->name();
  return casted;
}

// Non-const overload of Cast.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
T* Cast(HloInstruction* instruction) {
  return const_cast<T*>(
      Cast<T>(const_cast<const HloInstruction*>(instruction)));
}

// Works just like the Cast, except that it allows for a null pointer as an
// argument which it then propagates.
//
// Similar to LLVM's cast_or_null.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
const T* CastOrNull(const HloInstruction* instruction) {
  return instruction != nullptr ? Cast<T>(instruction) : nullptr;
}

// Non-const overload of CastOrNull.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
T* CastOrNull(HloInstruction* instruction) {
  return const_cast<T*>(
      CastOrNull<T>(const_cast<const HloInstruction*>(instruction)));
}

// Casts an HloInstruction pointer to one of its subclasses, dies if argument is
// nullptr, returns nullptr if runtime information does not match.
//
// Similar to LLVM's dyn_cast.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
const T* DynCast(const HloInstruction* instruction) {
  ITEX_CHECK(instruction != nullptr);
  return dynamic_cast<const T*>(instruction);
}

// Non-const overload of DynCast.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
T* DynCast(HloInstruction* instruction) {
  return const_cast<T*>(
      DynCast<T>(const_cast<const HloInstruction*>(instruction)));
}

// Works just like the DynCast, except that it allows for a null pointer as an
// argument which it then propagates.
//
// Similar to LLVM's dyn_cast_or_null.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
const T* DynCastOrNull(const HloInstruction* instruction) {
  return instruction != nullptr ? DynCast<T>(instruction) : nullptr;
}

// Non-const overload of DynCastOrNull.
template <class T, EnableIfDerivedFromHlo<T>* = nullptr>
T* DynCastOrNull(HloInstruction* instruction) {
  return const_cast<T*>(
      DynCastOrNull<T>(const_cast<const HloInstruction*>(instruction)));
}

}  // namespace itex_xla

#endif  // ITEX_CORE_COMPILER_XLA_SERVICE_HLO_CASTING_UTILS_H_
