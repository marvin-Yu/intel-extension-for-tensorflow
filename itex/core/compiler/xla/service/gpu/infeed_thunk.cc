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

#include "itex/core/compiler/xla/service/gpu/infeed_thunk.h"

#include <utility>
#include <vector>

#include "itex/core/compiler/xla/service/buffer_assignment.h"
#include "itex/core/compiler/xla/service/gpu/buffer_allocations.h"
#include "itex/core/compiler/xla/service/gpu/infeed_manager.h"
#include "itex/core/compiler/xla/shape_util.h"
#include "itex/core/compiler/xla/status_macros.h"
#include "itex/core/compiler/xla/util.h"

namespace itex_xla {
namespace gpu {

InfeedThunk::InfeedThunk(ThunkInfo thunk_info,
                         std::vector<ShapedSlice> dest_slices)
    : Thunk(Kind::kInfeed, thunk_info), dest_slices_(std::move(dest_slices)) {}

Status InfeedThunk::ExecuteOnStream(const ExecuteParams& params) {
  se::Stream* stream = params.stream;
  const BufferAllocations& buffer_allocations = *params.buffer_allocations;

  ITEX_VLOG(2) << "Infeeding to GPU";
  ShapeTree<se::ScopedDeviceMemory<uint8_t>> source_buffers =
      GetOrCreateInfeedManager(stream->parent())->BlockingGetNextDestination();

  size_t index = 0;
  for (auto& source : source_buffers.leaves()) {
    // Assert that the shapes are compatible.
    const ShapeIndex& shape_index = source.first;
    se::ScopedDeviceMemory<uint8_t>& buffer = source.second;
    const Shape& source_shape =
        ShapeUtil::GetSubshape(source_buffers.shape(), shape_index);
    TF_RET_CHECK(ShapeUtil::Equal(dest_slices_[index].shape, source_shape))
        << "Mismatch between infeed source buffer shape "
        << ShapeUtil::HumanStringWithLayout(source_shape)
        << " and infeed dest buffer shape "
        << ShapeUtil::HumanStringWithLayout(dest_slices_[index].shape);
    se::DeviceMemoryBase dest_address =
        buffer_allocations.GetDeviceAddress(dest_slices_[index++].slice);
    stream->ThenMemcpy(&dest_address, *buffer.ptr(), buffer.ptr()->size());
  }

  // Make sure that all dest slices have been copied into.
  ITEX_CHECK_EQ(index, dest_slices_.size())
      << "Infeed did not populate all destination buffers";

  Status block_status = stream->BlockHostUntilDone();
  if (!block_status.ok()) {
    return InternalError("Failed to complete data transfer on stream %p: %s",
                         stream, block_status.error_message());
  }

  ITEX_VLOG(2) << "Infeeding to GPU complete";
  return Status::OK();
}

}  // namespace gpu
}  // namespace itex_xla
