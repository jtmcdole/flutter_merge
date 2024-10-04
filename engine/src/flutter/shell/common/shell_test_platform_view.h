// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_COMMON_SHELL_TEST_PLATFORM_VIEW_H_
#define FLUTTER_SHELL_COMMON_SHELL_TEST_PLATFORM_VIEW_H_

#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/common/shell_test_external_view_embedder.h"
#include "flutter/shell/common/vsync_waiters_test.h"

namespace flutter {
namespace testing {

class ShellTestPlatformView : public PlatformView {
 public:
  enum class BackendType {
    kDefaultBackend = 0,
    kGLBackend,
    kVulkanBackend,
    kMetalBackend,
  };

  static std::unique_ptr<ShellTestPlatformView> Create(
      PlatformView::Delegate& delegate,
      const TaskRunners& task_runners,
      const std::shared_ptr<ShellTestVsyncClock>& vsync_clock,
      const CreateVsyncWaiter& create_vsync_waiter,
      BackendType backend,
      const std::shared_ptr<ShellTestExternalViewEmbedder>&
          shell_test_external_view_embedder,
      const std::shared_ptr<const fml::SyncSwitch>&
          is_gpu_disabled_sync_switch);

  virtual void SimulateVSync() = 0;

 protected:
  ShellTestPlatformView(PlatformView::Delegate& delegate,
                        const TaskRunners& task_runners)
      : PlatformView(delegate, task_runners) {}

  FML_DISALLOW_COPY_AND_ASSIGN(ShellTestPlatformView);
};

// Create a ShellTestPlatformView from configuration struct.
class ShellTestPlatformViewBuilder {
 public:
  struct Config {
    bool simulate_vsync = false;
    std::shared_ptr<ShellTestExternalViewEmbedder>
        shell_test_external_view_embedder = nullptr;
    ShellTestPlatformView::BackendType rendering_backend =
        ShellTestPlatformView::BackendType::kDefaultBackend;
  };

  explicit ShellTestPlatformViewBuilder(Config config);
  ~ShellTestPlatformViewBuilder() = default;

  // Override operator () to make this class assignable to std::function.
  std::unique_ptr<PlatformView> operator()(Shell& shell);

 private:
  Config config_;
};

}  // namespace testing
}  // namespace flutter

#endif  // FLUTTER_SHELL_COMMON_SHELL_TEST_PLATFORM_VIEW_H_
