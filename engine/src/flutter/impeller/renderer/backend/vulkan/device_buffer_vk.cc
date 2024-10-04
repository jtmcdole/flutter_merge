// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/vulkan/device_buffer_vk.h"

#include "flutter/flutter_vma/flutter_vma.h"
#include "flutter/fml/trace_event.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "vulkan/vulkan_core.h"

namespace impeller {

DeviceBufferVK::DeviceBufferVK(DeviceBufferDescriptor desc,
                               std::weak_ptr<Context> context,
                               UniqueBufferVMA buffer,
                               VmaAllocationInfo info)
    : DeviceBuffer(desc),
      context_(std::move(context)),
      resource_(ContextVK::Cast(*context_.lock().get()).GetResourceManager(),
                BufferResource{
                    std::move(buffer),  //
                    info                //
                }) {}

DeviceBufferVK::~DeviceBufferVK() = default;

uint8_t* DeviceBufferVK::OnGetContents() const {
  return static_cast<uint8_t*>(resource_->info.pMappedData);
}

bool DeviceBufferVK::OnCopyHostBuffer(const uint8_t* source,
                                      Range source_range,
                                      size_t offset) {
  uint8_t* dest = OnGetContents();

  if (!dest) {
    return false;
  }

  if (source) {
    ::memmove(dest + offset, source + source_range.offset, source_range.length);
  }
  ::vmaFlushAllocation(resource_->buffer.get().allocator,
                       resource_->buffer.get().allocation, offset,
                       source_range.length);

  return true;
}

bool DeviceBufferVK::SetLabel(const std::string& label) {
  auto context = context_.lock();
  if (!context || !resource_->buffer.is_valid()) {
    // The context could have died at this point.
    return false;
  }

  ::vmaSetAllocationName(resource_->buffer.get().allocator,   //
                         resource_->buffer.get().allocation,  //
                         label.c_str()                        //
  );

  return ContextVK::Cast(*context).SetDebugName(resource_->buffer.get().buffer,
                                                label);
}

void DeviceBufferVK::Flush(std::optional<Range> range) const {
  auto flush_range = range.value_or(Range{0, GetDeviceBufferDescriptor().size});
  ::vmaFlushAllocation(resource_->buffer.get().allocator,
                       resource_->buffer.get().allocation, flush_range.offset,
                       flush_range.length);
}

void DeviceBufferVK::Invalidate(std::optional<Range> range) const {
  auto flush_range = range.value_or(Range{0, GetDeviceBufferDescriptor().size});
  ::vmaInvalidateAllocation(resource_->buffer.get().allocator,
                            resource_->buffer.get().allocation,
                            flush_range.offset, flush_range.length);
}

bool DeviceBufferVK::SetLabel(const std::string& label, Range range) {
  // We do not have the ability to name ranges. Just name the whole thing.
  return SetLabel(label);
}

vk::Buffer DeviceBufferVK::GetBuffer() const {
  return resource_->buffer.get().buffer;
}

}  // namespace impeller
