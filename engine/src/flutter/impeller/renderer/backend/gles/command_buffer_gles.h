// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_GLES_COMMAND_BUFFER_GLES_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_GLES_COMMAND_BUFFER_GLES_H_

#include "impeller/renderer/backend/gles/reactor_gles.h"
#include "impeller/renderer/command_buffer.h"
#include "impeller/renderer/render_target.h"

namespace impeller {

class CommandBufferGLES final : public CommandBuffer {
 public:
  // |CommandBuffer|
  ~CommandBufferGLES() override;

 private:
  friend class ContextGLES;

  ReactorGLES::Ref reactor_;
  bool is_valid_ = false;

  CommandBufferGLES(std::weak_ptr<const Context> context,
                    ReactorGLES::Ref reactor);

  // |CommandBuffer|
  void SetLabel(const std::string& label) const override;

  // |CommandBuffer|
  bool IsValid() const override;

  // |CommandBuffer|
  bool OnSubmitCommands(CompletionCallback callback) override;

  // |CommandBuffer|
  void OnWaitUntilScheduled() override;

  // |CommandBuffer|
  std::shared_ptr<RenderPass> OnCreateRenderPass(RenderTarget target) override;

  // |CommandBuffer|
  std::shared_ptr<BlitPass> OnCreateBlitPass() override;

  // |CommandBuffer|
  std::shared_ptr<ComputePass> OnCreateComputePass() override;

  CommandBufferGLES(const CommandBufferGLES&) = delete;

  CommandBufferGLES& operator=(const CommandBufferGLES&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_GLES_COMMAND_BUFFER_GLES_H_
