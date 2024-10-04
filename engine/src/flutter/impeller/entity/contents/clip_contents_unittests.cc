// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <optional>

#include "gtest/gtest.h"

#include "impeller/entity/contents/clip_contents.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/contents.h"
#include "impeller/entity/contents/test/recording_render_pass.h"
#include "impeller/entity/entity.h"
#include "impeller/entity/entity_playground.h"
#include "impeller/renderer/render_target.h"

namespace impeller {
namespace testing {

using EntityTest = EntityPlayground;

TEST_P(EntityTest, ClipContentsOptimizesFullScreenIntersectClips) {
  // Set up mock environment.

  auto content_context = GetContentContext();
  auto buffer = content_context->GetContext()->CreateCommandBuffer();
  auto render_target =
      GetContentContext()->GetRenderTargetCache()->CreateOffscreenMSAA(
          *content_context->GetContext(), {100, 100},
          /*mip_count=*/1);
  auto render_pass = buffer->CreateRenderPass(render_target);
  auto recording_pass = std::make_shared<RecordingRenderPass>(
      render_pass, GetContext(), render_target);

  // Set up clip contents.

  auto contents = std::make_shared<ClipContents>();
  contents->SetClipOperation(Entity::ClipOperation::kIntersect);
  contents->SetGeometry(Geometry::MakeCover());

  Entity entity;
  entity.SetContents(std::move(contents));

  // Render the clip contents.

  ASSERT_TRUE(recording_pass->GetCommands().empty());
  ASSERT_TRUE(entity.Render(*content_context, *recording_pass));
  ASSERT_FALSE(recording_pass->GetCommands().empty());
}

}  // namespace testing
}  // namespace impeller
