/* Copyright (c) 2023 Intel Corporation

Copyright 2016 The TensorFlow Authors. All Rights Reserved.

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

// Class method definitions for HostStream, the Stream implementation for
// the HostExecutor implementation.
#include "itex/core/compiler/xla/stream_executor/host/host_stream.h"

#include <utility>

#include "absl/synchronization/notification.h"
#include "itex/core/utils/denormal.h"
#include "itex/core/utils/setround.h"

namespace stream_executor {
namespace host {

namespace {

port::ThreadOptions GetThreadOptions(size_t stack_size_in_bytes) {
  port::ThreadOptions options;
  options.stack_size = stack_size_in_bytes;
  return options;
}

}  // namespace

HostStream::HostStream(size_t stack_size_in_bytes)
    : thread_(port::Env::Default()->StartThread(
          GetThreadOptions(stack_size_in_bytes), "host_executor",
          [this]() { WorkLoop(); })) {}

HostStream::~HostStream() {
  {
    absl::MutexLock lock(&mu_);
    work_queue_.push(nullptr);
  }
  // thread_'s destructor blocks until the thread finishes running.
  thread_.reset();
}

bool HostStream::EnqueueTask(std::function<void()> task) {
  return EnqueueTaskWithStatus([task = std::move(task)]() {
    task();
    return ::itex::OkStatus();
  });
}

bool HostStream::EnqueueTaskWithStatus(std::function<port::Status()> task) {
  ITEX_CHECK(task != nullptr);
  absl::MutexLock lock(&mu_);
  work_queue_.push(std::move(task));
  return true;
}

bool HostStream::WorkAvailable() { return !work_queue_.empty(); }

void HostStream::WorkLoop() {
  // Set denormal and rounding behavior to match the default TF ThreadPool
  // behavior.
  // TODO(phawkins, jlebar): it's not clear this is the best place to set this.
  itex::port::ScopedFlushDenormal flush;
  itex::port::ScopedSetRound round(FE_TONEAREST);
  while (true) {
    std::queue<std::function<port::Status()>> queue;
    {
      absl::MutexLock lock(&mu_);
      mu_.Await(absl::Condition(this, &HostStream::WorkAvailable));
      std::swap(queue, work_queue_);
    }
    while (!queue.empty()) {
      std::function<port::Status()>& fn = queue.front();
      if (!fn) {
        return;
      }
      status_.Update(fn());
      queue.pop();
    }
  }
}

port::Status HostStream::BlockUntilDone() {
  absl::Notification done;
  port::Status status;
  EnqueueTask([&done, &status, this]() {
    // This task is always executed synchronously before 'status_' is updated
    // with the result of the task (always OK() in this case), so we don't need
    // to worry about locking access to 'status_'.
    status = status_;
    status_ = ::itex::OkStatus();
    done.Notify();
  });
  done.WaitForNotification();
  return status;
}

}  // namespace host

}  // namespace stream_executor
