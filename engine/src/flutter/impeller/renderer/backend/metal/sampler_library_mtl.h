// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_METAL_SAMPLER_LIBRARY_MTL_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_METAL_SAMPLER_LIBRARY_MTL_H_

#include <Metal/Metal.h>

#include <memory>

#include "impeller/base/backend_cast.h"
#include "impeller/base/comparable.h"
#include "impeller/core/sampler_descriptor.h"
#include "impeller/renderer/sampler_library.h"

namespace impeller {

class SamplerLibraryMTL final
    : public SamplerLibrary,
      public BackendCast<SamplerLibraryMTL, SamplerLibrary> {
 public:
  // |SamplerLibrary|
  ~SamplerLibraryMTL() override;

 private:
  friend class ContextMTL;

  id<MTLDevice> device_ = nullptr;
  SamplerMap samplers_;

  explicit SamplerLibraryMTL(id<MTLDevice> device);

  // |SamplerLibrary|
  const std::unique_ptr<const Sampler>& GetSampler(
      SamplerDescriptor descriptor) override;

  SamplerLibraryMTL(const SamplerLibraryMTL&) = delete;

  SamplerLibraryMTL& operator=(const SamplerLibraryMTL&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_METAL_SAMPLER_LIBRARY_MTL_H_
