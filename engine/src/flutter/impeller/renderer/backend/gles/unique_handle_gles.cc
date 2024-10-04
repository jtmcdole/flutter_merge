// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/unique_handle_gles.h"

#include <utility>

namespace impeller {

UniqueHandleGLES::UniqueHandleGLES(ReactorGLES::Ref reactor, HandleType type)
    : reactor_(std::move(reactor)) {
  if (reactor_) {
    handle_ = reactor_->CreateHandle(type);
  }
}

UniqueHandleGLES::UniqueHandleGLES(ReactorGLES::Ref reactor, HandleGLES handle)
    : reactor_(std::move(reactor)), handle_(handle) {}

UniqueHandleGLES::~UniqueHandleGLES() {
  if (!handle_.IsDead() && reactor_) {
    reactor_->CollectHandle(handle_);
  }
}

const HandleGLES& UniqueHandleGLES::Get() const {
  return handle_;
}

bool UniqueHandleGLES::IsValid() const {
  return !handle_.IsDead();
}

UniqueHandleGLES::UniqueHandleGLES(UniqueHandleGLES&& other) {
  std::swap(reactor_, other.reactor_);
  std::swap(handle_, other.handle_);
}

}  // namespace impeller
