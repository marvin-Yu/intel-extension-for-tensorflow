/* Copyright (c) 2023 Intel Corporation

Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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

// Utility functions related to layouts of Shapes.

#ifndef ITEX_CORE_COMPILER_XLA_INDEX_UTIL_H_
#define ITEX_CORE_COMPILER_XLA_INDEX_UTIL_H_

#include <vector>

#include "absl/types/span.h"
#include "itex/core/compiler/xla/shape.h"
#include "itex/core/compiler/xla/types.h"
#include "protos/xla.pb.h"

namespace itex_xla {

// Namespaced collection of (static) utilities related to indexing into
// multidimensional arrays.
class IndexUtil {
 public:
  // Converts a multidimensional index (eg {x, y, z}) into a linear index based
  // on the shape and its layout. The first index in the multi_index is
  // dimension 0.
  static int64_t MultidimensionalIndexToLinearIndex(
      const Shape& shape, absl::Span<const int64_t> multi_index);

  // Converts a linear index into multidimensional index (eg {x, y, z}) based on
  // the shape and its layout. The first index in the returned multidimensional
  // index is dimension 0.
  static std::vector<int64_t> LinearIndexToMultidimensionalIndex(
      const Shape& shape, int64_t linear_index);

  // Bumps a sequence of indices; e.g. {0,0,0,0} up by one index value; e.g. to
  // {0,0,0,1}. This is akin to std::next_permutation. If the index hits a limit
  // for the provided shape, the next most significant index is bumped, in a
  // counting-up process.
  //
  // E.g. for shape f32[2,3]
  //  {0,0}=>{0,1}
  //  {0,1}=>{0,2}
  //  {0,2}=>{1,0}
  //  etc.
  //
  // This is useful for traversing the indices in a literal.
  //
  // Returns true iff the indices were successfully bumped; false if we've hit
  // the limit where it can no longer be bumped in-bounds.
  static bool BumpIndices(const Shape& shape, absl::Span<int64_t> indices);

  // Calculates the stride size (in number of elements, not byte size) of a
  // given logical shape dimension (from 0 to rank-1).
  // Example:
  //  GetDimensionStride(F32[5,8,10,4]{3,2,1,0}, 1) ==
  //    sizeof(dimension(3)) * sizeof(dimension(2)) == 4 * 10
  static int64_t GetDimensionStride(const Shape& shape, int64_t dimension);

  // Returns true iff the given multi-index is contained in the bounds for the
  // shape.
  static bool IndexInBounds(const Shape& shape,
                            absl::Span<const int64_t> index);

  // Compares the given indices in lexicographic order.  lhs[0] and rhs[0] are
  // compared first, and lhs[rank-1] and rhs[rank-1] last.  If lhs is larger,
  // then -1 is returned. If rhs is larger, then 1 is returned.  Otherwise, 0 is
  // returned.
  static int CompareIndices(absl::Span<const int64_t> lhs,
                            absl::Span<const int64_t> rhs);

 private:
  IndexUtil(const IndexUtil&) = delete;
  IndexUtil& operator=(const IndexUtil&) = delete;
};

}  // namespace itex_xla

#endif  // ITEX_CORE_COMPILER_XLA_INDEX_UTIL_H_
