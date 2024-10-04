// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_RENDER_PASS_BUILDER_VK_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_RENDER_PASS_BUILDER_VK_H_

#include <map>
#include <optional>

#include "impeller/core/formats.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/vk.h"

namespace impeller {

class RenderPassBuilderVK {
 public:
  RenderPassBuilderVK();

  ~RenderPassBuilderVK();

  RenderPassBuilderVK(const RenderPassBuilderVK&) = delete;

  RenderPassBuilderVK& operator=(const RenderPassBuilderVK&) = delete;

  RenderPassBuilderVK& SetColorAttachment(size_t index,
                                          PixelFormat format,
                                          SampleCount sample_count,
                                          LoadAction load_action,
                                          StoreAction store_action);

  RenderPassBuilderVK& SetDepthStencilAttachment(PixelFormat format,
                                                 SampleCount sample_count,
                                                 LoadAction load_action,
                                                 StoreAction store_action);

  RenderPassBuilderVK& SetStencilAttachment(PixelFormat format,
                                            SampleCount sample_count,
                                            LoadAction load_action,
                                            StoreAction store_action);

  vk::UniqueRenderPass Build(const vk::Device& device) const;

  // Visible for testing.
  const std::map<size_t, vk::AttachmentDescription>& GetColorAttachments()
      const;

  // Visible for testing.
  const std::map<size_t, vk::AttachmentDescription>& GetResolves() const;

  // Visible for testing.
  const std::optional<vk::AttachmentDescription>& GetDepthStencil() const;

 private:
  std::map<size_t, vk::AttachmentDescription> colors_;
  std::map<size_t, vk::AttachmentDescription> resolves_;
  std::optional<vk::AttachmentDescription> depth_stencil_;
};

//------------------------------------------------------------------------------
/// @brief      Inserts the appropriate barriers to ensure that subsequent
///             commands can read from the specified image (itself a framebuffer
///             attachment) as an input attachment.
///
///             Unlike most barriers, this barrier may only be inserted within a
///             Vulkan render-pass.
///
///             The type of barrier inserted depends on the subpass setup and
///             self-dependencies. Only use this utility method for inserting
///             barriers in render passes created by `RenderPassBuilderVK`.
///
/// @param[in]  buffer  The buffer
/// @param[in]  image   The image
///
void InsertBarrierForInputAttachmentRead(const vk::CommandBuffer& buffer,
                                         const vk::Image& image);

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_RENDER_PASS_BUILDER_VK_H_
