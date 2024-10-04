// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_COMMAND_BUFFER_VK_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_COMMAND_BUFFER_VK_H_

#include "impeller/base/backend_cast.h"
#include "impeller/renderer/backend/vulkan/vk.h"
#include "impeller/renderer/command_buffer.h"

namespace impeller {

class ContextVK;
class CommandEncoderFactoryVK;
class CommandEncoderVK;

class CommandBufferVK final
    : public CommandBuffer,
      public BackendCast<CommandBufferVK, CommandBuffer>,
      public std::enable_shared_from_this<CommandBufferVK> {
 public:
  // |CommandBuffer|
  ~CommandBufferVK() override;

  const std::shared_ptr<CommandEncoderVK>& GetEncoder();

 private:
  friend class ContextVK;

  std::shared_ptr<CommandEncoderVK> encoder_;
  std::shared_ptr<CommandEncoderFactoryVK> encoder_factory_;

  CommandBufferVK(std::weak_ptr<const Context> context,
                  std::shared_ptr<CommandEncoderFactoryVK> encoder_factory);

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

  CommandBufferVK(const CommandBufferVK&) = delete;

  CommandBufferVK& operator=(const CommandBufferVK&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_COMMAND_BUFFER_VK_H_
