// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_GLES_BLIT_PASS_GLES_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_GLES_BLIT_PASS_GLES_H_

#include <memory>

#include "flutter/impeller/base/config.h"
#include "flutter/impeller/renderer/backend/gles/reactor_gles.h"
#include "flutter/impeller/renderer/blit_pass.h"
#include "impeller/renderer/backend/gles/blit_command_gles.h"

namespace impeller {

class BlitPassGLES final : public BlitPass,
                           public std::enable_shared_from_this<BlitPassGLES> {
 public:
  // |BlitPass|
  ~BlitPassGLES() override;

 private:
  friend class CommandBufferGLES;

  std::vector<std::unique_ptr<BlitEncodeGLES>> commands_;
  ReactorGLES::Ref reactor_;
  std::string label_;
  bool is_valid_ = false;

  explicit BlitPassGLES(ReactorGLES::Ref reactor);

  // |BlitPass|
  bool IsValid() const override;

  // |BlitPass|
  void OnSetLabel(std::string label) override;

  // |BlitPass|
  bool EncodeCommands(
      const std::shared_ptr<Allocator>& transients_allocator) const override;

  // |BlitPass|
  bool ResizeTexture(const std::shared_ptr<Texture>& source,
                     const std::shared_ptr<Texture>& destination) override;

  // |BlitPass|
  bool OnCopyTextureToTextureCommand(std::shared_ptr<Texture> source,
                                     std::shared_ptr<Texture> destination,
                                     IRect source_region,
                                     IPoint destination_origin,
                                     std::string label) override;

  // |BlitPass|
  bool OnCopyTextureToBufferCommand(std::shared_ptr<Texture> source,
                                    std::shared_ptr<DeviceBuffer> destination,
                                    IRect source_region,
                                    size_t destination_offset,
                                    std::string label) override;

  // |BlitPass|
  bool OnCopyBufferToTextureCommand(BufferView source,
                                    std::shared_ptr<Texture> destination,
                                    IRect destination_region,
                                    std::string label,
                                    uint32_t slice,
                                    bool convert_to_read) override;

  // |BlitPass|
  bool OnGenerateMipmapCommand(std::shared_ptr<Texture> texture,
                               std::string label) override;

  BlitPassGLES(const BlitPassGLES&) = delete;

  BlitPassGLES& operator=(const BlitPassGLES&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_GLES_BLIT_PASS_GLES_H_
