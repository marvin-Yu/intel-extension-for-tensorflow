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

#include "itex/core/compiler/xla/service/stream_pool.h"

#include <memory>
#include <utility>

#include "itex/core/utils/logging.h"

namespace itex_xla {

StreamPool::Ptr StreamPool::BorrowStream(se::StreamExecutor* executor) {
  std::unique_ptr<se::Stream> stream;
  {
    absl::MutexLock lock(&mu_);
    while (!streams_.empty() && !stream) {
      // Re-use an existing stream from the pool.
      stream = std::move(streams_.back());
      streams_.pop_back();
      if (stream->ok()) {
        ITEX_VLOG(1) << stream->DebugStreamPointers()
                     << " StreamPool reusing existing stream";
      } else {
        ITEX_VLOG(1) << stream->DebugStreamPointers()
                     << " stream was not ok, StreamPool deleting";
        stream = nullptr;
      }
    }
  }

  if (!stream) {
    // Create a new stream.
    stream = std::make_unique<se::Stream>(executor);
    stream->Init();
    ITEX_VLOG(1) << stream->DebugStreamPointers()
                 << " StreamPool created new stream";
  }

  // Return the stream wrapped in Ptr, which has our special deleter semantics.
  PtrDeleter deleter = {this};
  return Ptr(stream.release(), deleter);
}

void StreamPool::ReturnStream(se::Stream* stream) {
  if (stream->ok()) {
    ITEX_VLOG(1) << stream->DebugStreamPointers()
                 << " StreamPool returning ok stream";
    absl::MutexLock lock(&mu_);
    streams_.emplace_back(stream);
  } else {
    // If the stream has encountered any errors, all subsequent operations on it
    // will fail. So just delete the stream, and rely on new streams to be
    // created in the future.
    ITEX_VLOG(1) << stream->DebugStreamPointers()
                 << " StreamPool deleting !ok stream";
    delete stream;
  }
}

}  // namespace itex_xla
