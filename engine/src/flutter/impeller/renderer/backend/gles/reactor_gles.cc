// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/reactor_gles.h"

#include <algorithm>

#include "flutter/fml/trace_event.h"
#include "fml/logging.h"
#include "impeller/base/validation.h"

namespace impeller {

ReactorGLES::ReactorGLES(std::unique_ptr<ProcTableGLES> gl)
    : proc_table_(std::move(gl)) {
  if (!proc_table_ || !proc_table_->IsValid()) {
    VALIDATION_LOG << "Proc table was invalid.";
    return;
  }
  can_set_debug_labels_ = proc_table_->GetDescription()->HasDebugExtension();
  is_valid_ = true;
}

ReactorGLES::~ReactorGLES() = default;

bool ReactorGLES::IsValid() const {
  return is_valid_;
}

ReactorGLES::WorkerID ReactorGLES::AddWorker(std::weak_ptr<Worker> worker) {
  Lock lock(workers_mutex_);
  auto id = WorkerID{};
  workers_[id] = std::move(worker);
  return id;
}

bool ReactorGLES::RemoveWorker(WorkerID worker) {
  Lock lock(workers_mutex_);
  return workers_.erase(worker) == 1;
}

bool ReactorGLES::HasPendingOperations() const {
  Lock ops_lock(ops_mutex_);
  return !ops_.empty();
}

const ProcTableGLES& ReactorGLES::GetProcTable() const {
  FML_DCHECK(IsValid());
  return *proc_table_;
}

std::optional<GLuint> ReactorGLES::GetGLHandle(const HandleGLES& handle) const {
  ReaderLock handles_lock(handles_mutex_);
  if (auto found = handles_.find(handle); found != handles_.end()) {
    if (found->second.pending_collection) {
      VALIDATION_LOG
          << "Attempted to acquire a handle that was pending collection.";
      return std::nullopt;
    }
    if (!found->second.name.has_value()) {
      VALIDATION_LOG << "Attempt to acquire a handle outside of an operation.";
      return std::nullopt;
    }
    return found->second.name;
  }
  VALIDATION_LOG << "Attempted to acquire an invalid GL handle.";
  return std::nullopt;
}

bool ReactorGLES::AddOperation(Operation operation) {
  if (!operation) {
    return false;
  }
  {
    Lock ops_lock(ops_mutex_);
    ops_.emplace_back(std::move(operation));
  }
  // Attempt a reaction if able but it is not an error if this isn't possible.
  [[maybe_unused]] auto result = React();
  return true;
}

static std::optional<GLuint> CreateGLHandle(const ProcTableGLES& gl,
                                            HandleType type) {
  GLuint handle = GL_NONE;
  switch (type) {
    case HandleType::kUnknown:
      return std::nullopt;
    case HandleType::kTexture:
      gl.GenTextures(1u, &handle);
      return handle;
    case HandleType::kBuffer:
      gl.GenBuffers(1u, &handle);
      return handle;
    case HandleType::kProgram:
      return gl.CreateProgram();
    case HandleType::kRenderBuffer:
      gl.GenRenderbuffers(1u, &handle);
      return handle;
    case HandleType::kFrameBuffer:
      gl.GenFramebuffers(1u, &handle);
      return handle;
  }
  return std::nullopt;
}

static bool CollectGLHandle(const ProcTableGLES& gl,
                            HandleType type,
                            GLuint handle) {
  switch (type) {
    case HandleType::kUnknown:
      return false;
    case HandleType::kTexture:
      gl.DeleteTextures(1u, &handle);
      return true;
    case HandleType::kBuffer:
      gl.DeleteBuffers(1u, &handle);
      return true;
    case HandleType::kProgram:
      gl.DeleteProgram(handle);
      return true;
    case HandleType::kRenderBuffer:
      gl.DeleteRenderbuffers(1u, &handle);
      return true;
    case HandleType::kFrameBuffer:
      gl.DeleteFramebuffers(1u, &handle);
      return true;
  }
  return false;
}

HandleGLES ReactorGLES::CreateHandle(HandleType type) {
  if (type == HandleType::kUnknown) {
    return HandleGLES::DeadHandle();
  }
  auto new_handle = HandleGLES::Create(type);
  if (new_handle.IsDead()) {
    return HandleGLES::DeadHandle();
  }
  WriterLock handles_lock(handles_mutex_);
  auto gl_handle = CanReactOnCurrentThread()
                       ? CreateGLHandle(GetProcTable(), type)
                       : std::nullopt;
  handles_[new_handle] = LiveHandle{gl_handle};
  return new_handle;
}

void ReactorGLES::CollectHandle(HandleGLES handle) {
  WriterLock handles_lock(handles_mutex_);
  if (auto found = handles_.find(handle); found != handles_.end()) {
    found->second.pending_collection = true;
  }
}

bool ReactorGLES::React() {
  if (!CanReactOnCurrentThread()) {
    return false;
  }
  TRACE_EVENT0("impeller", "ReactorGLES::React");
  while (HasPendingOperations()) {
    // Both the raster thread and the IO thread can flush queued operations.
    // Ensure that execution of the ops is serialized.
    Lock execution_lock(ops_execution_mutex_);

    if (!ReactOnce()) {
      return false;
    }
  }
  return true;
}

static DebugResourceType ToDebugResourceType(HandleType type) {
  switch (type) {
    case HandleType::kUnknown:
      FML_UNREACHABLE();
    case HandleType::kTexture:
      return DebugResourceType::kTexture;
    case HandleType::kBuffer:
      return DebugResourceType::kBuffer;
    case HandleType::kProgram:
      return DebugResourceType::kProgram;
    case HandleType::kRenderBuffer:
      return DebugResourceType::kRenderBuffer;
    case HandleType::kFrameBuffer:
      return DebugResourceType::kFrameBuffer;
  }
  FML_UNREACHABLE();
}

bool ReactorGLES::ReactOnce() {
  if (!IsValid()) {
    return false;
  }
  TRACE_EVENT0("impeller", __FUNCTION__);
  return ConsolidateHandles() && FlushOps();
}

bool ReactorGLES::ConsolidateHandles() {
  TRACE_EVENT0("impeller", __FUNCTION__);
  const auto& gl = GetProcTable();
  WriterLock handles_lock(handles_mutex_);
  std::vector<HandleGLES> handles_to_delete;
  for (auto& handle : handles_) {
    // Collect dead handles.
    if (handle.second.pending_collection) {
      // This could be false if the handle was created and collected without
      // use. We still need to get rid of map entry.
      if (handle.second.name.has_value()) {
        CollectGLHandle(gl, handle.first.type, handle.second.name.value());
      }
      handles_to_delete.push_back(handle.first);
      continue;
    }
    // Create live handles.
    if (!handle.second.name.has_value()) {
      auto gl_handle = CreateGLHandle(gl, handle.first.type);
      if (!gl_handle) {
        VALIDATION_LOG << "Could not create GL handle.";
        return false;
      }
      handle.second.name = gl_handle;
    }
    // Set pending debug labels.
    if (handle.second.pending_debug_label.has_value()) {
      if (gl.SetDebugLabel(ToDebugResourceType(handle.first.type),
                           handle.second.name.value(),
                           handle.second.pending_debug_label.value())) {
        handle.second.pending_debug_label = std::nullopt;
      }
    }
  }
  for (const auto& handle_to_delete : handles_to_delete) {
    handles_.erase(handle_to_delete);
  }
  return true;
}

bool ReactorGLES::FlushOps() {
  TRACE_EVENT0("impeller", __FUNCTION__);

#ifdef IMPELLER_DEBUG
  // glDebugMessageControl sometimes must be called before glPushDebugGroup:
  // https://github.com/flutter/flutter/issues/135715#issuecomment-1740153506
  SetupDebugGroups();
#endif

  // Do NOT hold the ops or handles locks while performing operations in case
  // the ops enqueue more ops.
  decltype(ops_) ops;
  {
    Lock ops_lock(ops_mutex_);
    std::swap(ops_, ops);
  }
  for (const auto& op : ops) {
    TRACE_EVENT0("impeller", "ReactorGLES::Operation");
    op(*this);
  }
  return true;
}

void ReactorGLES::SetupDebugGroups() {
  // Setup of a default active debug group: Filter everything in.
  if (proc_table_->DebugMessageControlKHR.IsAvailable()) {
    proc_table_->DebugMessageControlKHR(GL_DONT_CARE,  // source
                                        GL_DONT_CARE,  // type
                                        GL_DONT_CARE,  // severity
                                        0,             // count
                                        nullptr,       // ids
                                        GL_TRUE);      // enabled
  }
}

void ReactorGLES::SetDebugLabel(const HandleGLES& handle, std::string label) {
  if (!can_set_debug_labels_) {
    return;
  }
  if (handle.IsDead()) {
    return;
  }
  WriterLock handles_lock(handles_mutex_);
  if (auto found = handles_.find(handle); found != handles_.end()) {
    found->second.pending_debug_label = std::move(label);
  }
}

bool ReactorGLES::CanReactOnCurrentThread() const {
  std::vector<WorkerID> dead_workers;
  Lock lock(workers_mutex_);
  for (const auto& worker : workers_) {
    auto worker_ptr = worker.second.lock();
    if (!worker_ptr) {
      dead_workers.push_back(worker.first);
      continue;
    }
    if (worker_ptr->CanReactorReactOnCurrentThreadNow(*this)) {
      return true;
    }
  }
  for (const auto& worker_id : dead_workers) {
    workers_.erase(worker_id);
  }
  return false;
}

}  // namespace impeller
