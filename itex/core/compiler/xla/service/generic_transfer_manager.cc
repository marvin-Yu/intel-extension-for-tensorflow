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

#include "itex/core/compiler/xla/service/generic_transfer_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "itex/core/compiler/xla/layout_util.h"
#include "itex/core/compiler/xla/literal.h"
#include "itex/core/compiler/xla/shape_util.h"
#include "itex/core/compiler/xla/status_macros.h"
#include "itex/core/compiler/xla/stream_executor/stream_executor.h"
#include "itex/core/compiler/xla/types.h"
#include "itex/core/compiler/xla/util.h"
#include "itex/core/utils/errors.h"
#include "itex/core/utils/logging.h"

namespace itex_xla {

GenericTransferManager::GenericTransferManager(se::Platform::Id platform_id,
                                               size_t pointer_size)
    : platform_id_(platform_id), pointer_size_(pointer_size) {}

se::Platform::Id GenericTransferManager::PlatformId() const {
  return platform_id_;
}

Status GenericTransferManager::WriteSingleTupleIndexTable(
    se::Stream* stream, absl::Span<const se::DeviceMemoryBase> elements,
    const Shape& shape, se::DeviceMemoryBase* region) {
  TF_RET_CHECK(elements.size() == ShapeUtil::TupleElementCount(shape));

  auto element_pointers = std::make_shared<std::vector<const void*>>();
  element_pointers->reserve(elements.size());
  for (const se::DeviceMemoryBase& element : elements) {
    element_pointers->push_back(element.opaque());
  }
  TF_RETURN_IF_ERROR(TransferBufferToDevice(
      stream, GetByteSizeRequirement(shape), element_pointers->data(), region));
  // Ensure the buffer is transferred before we destroy element_pointers.
  stream->ThenDoHostCallback([element_pointers{std::move(element_pointers)}]() {
    // /* holds reference to element_pointers in closure */
  });
  return Status::OK();
}

void GenericTransferManager::TransferLiteralFromDevice(
    se::Stream* stream, const ShapedBuffer& device_buffer,
    MutableBorrowingLiteral literal, std::function<void(Status)> done,
    const TransferMetadata* /*transfer_metadata*/) {
  ITEX_VLOG(2) << "transferring literal from device ordinal "
               << stream->parent()->device_ordinal()
               << "; device buffer: " << device_buffer;
  Status status = [&]() -> Status {
    TF_RET_CHECK(stream->parent()->device_ordinal() ==
                 device_buffer.device_ordinal());

    TF_RETURN_IF_ERROR(ShapeUtil::ForEachSubshapeWithStatus(
        device_buffer.on_device_shape(),
        [&](const Shape& subshape, const ShapeIndex& index) -> Status {
          if (subshape.IsArray()) {
            stream->ThenMemcpy(
                /*host_dst=*/literal.untyped_data(index),
                /*gpu_src=*/device_buffer.buffer(index),
                // With bounded dynamic shapes, the shape of the device buffer
                // (bounded allocation) can be bigger than the literal.
                /*size=*/
                GetByteSizeRequirement(
                    ShapeUtil::GetSubshape(literal.shape(), index)));
          }
          return Status::OK();
        }));
    return Status::OK();
  }();
  if (!status.ok()) {
    done(status);
    return;
  }

  done(stream->BlockHostUntilDone());
}

Status GenericTransferManager::TransferLiteralToDeviceAsync(
    se::Stream* stream, const LiteralSlice& literal,
    const ShapedBuffer& device_buffer,
    const TransferMetadata* /*transfer_metadata*/) {
  const Shape& shape = literal.shape();
  ITEX_VLOG(2) << "transferring literal shape to device: "
               << ShapeUtil::HumanString(shape)
               << "; device buffer: " << device_buffer;

  TF_RET_CHECK(
      ShapeUtil::Compatible(literal.shape(), device_buffer.on_device_shape()));
  TF_RET_CHECK(stream->parent()->device_ordinal() ==
               device_buffer.device_ordinal());

  TF_RETURN_IF_ERROR(WriteTupleIndexTablesAsync(stream, device_buffer));

  return ShapeUtil::ForEachSubshapeWithStatus(
      device_buffer.on_device_shape(),
      [&](const Shape& device_subshape, const ShapeIndex& index) -> Status {
        se::DeviceMemoryBase device_memory = device_buffer.buffer(index);
        if (device_subshape.IsArray()) {
          TF_RET_CHECK(GetByteSizeRequirement(device_subshape) ==
                       device_memory.size());
          // Element is array-shaped: transfer array data to device buffer.
          const auto subliteral = LiteralSlice(literal, index);
          Literal relayed_out_literal;
          const void* source;
          if (LayoutUtil::Equal(device_subshape.layout(),
                                subliteral.shape().layout())) {
            source = subliteral.untyped_data();
            return TransferBufferToDevice(
                stream,
                /*size=*/GetByteSizeRequirement(device_subshape), source,
                &device_memory);
          } else {
            // Relayout data before transferring.
            relayed_out_literal = subliteral.Relayout(device_subshape.layout(),
                                                      /*shape_index=*/{});
            source = relayed_out_literal.untyped_data();
            TF_RETURN_IF_ERROR(TransferBufferToDevice(
                stream,
                /*size=*/GetByteSizeRequirement(device_subshape), source,
                &device_memory));
            return stream->BlockHostUntilDone();
          }
        }
        return Status::OK();
      });
}

Status GenericTransferManager::TransferLiteralToInfeed(
    se::StreamExecutor* executor, const LiteralSlice& literal) {
  return Unimplemented("Generic transfer to Infeed");
}

Status GenericTransferManager::TransferLiteralFromOutfeed(
    se::StreamExecutor* executor, MutableBorrowingLiteral literal) {
  return Unimplemented("Generic transfer from Outfeed");
}

Status GenericTransferManager::ResetDevices(
    absl::Span<se::StreamExecutor* const>
    /*executors*/) {
  return Unimplemented(
      "Device reset is not yet supported on this platform (b/30481585)");
}

int64_t GenericTransferManager::GetByteSizeRequirement(
    const Shape& shape) const {
  if (shape.is_static() || shape.IsTuple()) {
    return ShapeUtil::ByteSizeOf(shape, pointer_size_);
  }
  int64_t metadata_size = sizeof(int32_t) * shape.dimensions_size();
  return ShapeUtil::ByteSizeOf(shape, pointer_size_) + metadata_size;
}

}  // namespace itex_xla
