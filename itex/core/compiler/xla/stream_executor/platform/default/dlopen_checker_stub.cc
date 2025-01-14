/* Copyright (c) 2023 Intel Corporation

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
#include "itex/core/compiler/xla/stream_executor/platform/default/dso_loader.h"
#include "itex/core/compiler/xla/stream_executor/platform/logging.h"
#include "itex/core/compiler/xla/stream_executor/platform/port.h"

namespace stream_executor {
namespace internal {
namespace DsoLoader {

// Skip check when GPU libraries are statically linked.
port::Status MaybeTryDlopenGPULibraries() {
  LOG(INFO) << "GPU libraries are statically linked, skip dlopen check.";
  return itex::Status::OK();
}
}  // namespace DsoLoader
}  // namespace internal
}  // namespace stream_executor
