// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_METAL_RENDER_PASS_MTL_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_METAL_RENDER_PASS_MTL_H_

#include <Metal/Metal.h>

#include "impeller/renderer/backend/metal/pass_bindings_cache_mtl.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/render_target.h"

namespace impeller {

class RenderPassMTL final : public RenderPass {
 public:
  // |RenderPass|
  ~RenderPassMTL() override;

 private:
  friend class CommandBufferMTL;

  id<MTLCommandBuffer> buffer_ = nil;
  id<MTLRenderCommandEncoder> encoder_ = nil;
  MTLRenderPassDescriptor* desc_ = nil;
  std::string label_;
  bool is_metal_trace_active_ = false;
  bool is_valid_ = false;
  // Many parts of the codebase will start writing to a render pass but
  // never submit them. This boolean is used to track if a submit happened
  // so that in the dtor we can always ensure the render pass is finished.
  mutable bool did_finish_encoding_ = false;

  PassBindingsCacheMTL pass_bindings_;

  // Per-command state
  size_t instance_count_ = 1u;
  size_t base_vertex_ = 0u;
  size_t vertex_count_ = 0u;
  bool has_valid_pipeline_ = false;
  bool has_label_ = false;
  BufferView index_buffer_ = {};
  PrimitiveType primitive_type_ = {};
  MTLIndexType index_type_ = {};

  RenderPassMTL(std::shared_ptr<const Context> context,
                const RenderTarget& target,
                id<MTLCommandBuffer> buffer);

  // |RenderPass|
  void ReserveCommands(size_t command_count) override {}

  // |RenderPass|
  bool IsValid() const override;

  // |RenderPass|
  void OnSetLabel(std::string label) override;

  // |RenderPass|
  bool OnEncodeCommands(const Context& context) const override;

  // |RenderPass|
  void SetPipeline(
      const std::shared_ptr<Pipeline<PipelineDescriptor>>& pipeline) override;

  // |RenderPass|
  void SetCommandLabel(std::string_view label) override;

  // |RenderPass|
  void SetStencilReference(uint32_t value) override;

  // |RenderPass|
  void SetBaseVertex(uint64_t value) override;

  // |RenderPass|
  void SetViewport(Viewport viewport) override;

  // |RenderPass|
  void SetScissor(IRect scissor) override;

  // |RenderPass|
  void SetInstanceCount(size_t count) override;

  // |RenderPass|
  bool SetVertexBuffer(VertexBuffer buffer) override;

  // |RenderPass|
  fml::Status Draw() override;

  // |RenderPass|
  bool BindResource(ShaderStage stage,
                    DescriptorType type,
                    const ShaderUniformSlot& slot,
                    const ShaderMetadata& metadata,
                    BufferView view) override;

  // |RenderPass|
  bool BindResource(ShaderStage stage,
                    DescriptorType type,
                    const ShaderUniformSlot& slot,
                    const std::shared_ptr<const ShaderMetadata>& metadata,
                    BufferView view) override;

  // |RenderPass|
  bool BindResource(ShaderStage stage,
                    DescriptorType type,
                    const SampledImageSlot& slot,
                    const ShaderMetadata& metadata,
                    std::shared_ptr<const Texture> texture,
                    const std::unique_ptr<const Sampler>& sampler) override;

  RenderPassMTL(const RenderPassMTL&) = delete;

  RenderPassMTL& operator=(const RenderPassMTL&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_METAL_RENDER_PASS_MTL_H_
