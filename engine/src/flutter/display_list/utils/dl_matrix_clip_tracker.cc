// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/display_list/utils/dl_matrix_clip_tracker.h"

#include "flutter/display_list/dl_builder.h"
#include "flutter/fml/logging.h"

namespace flutter {

bool DisplayListMatrixClipState::is_3x3(const SkM44& m) {
  // clang-format off
  return (                                      m.rc(0, 2) == 0 &&
                                                m.rc(1, 2) == 0 &&
          m.rc(2, 0) == 0 && m.rc(2, 1) == 0 && m.rc(2, 2) == 1 && m.rc(2, 3) == 0 &&
                                                m.rc(3, 2) == 0);
  // clang-format on
}

static constexpr DlRect kEmpty = DlRect();

static const DlRect& ProtectEmpty(const SkRect& rect) {
  // isEmpty protects us against NaN while we normalize any empty cull rects
  return rect.isEmpty() ? kEmpty : ToDlRect(rect);
}

static const DlRect& ProtectEmpty(const DlRect& rect) {
  // isEmpty protects us against NaN while we normalize any empty cull rects
  return rect.IsEmpty() ? kEmpty : rect;
}

DisplayListMatrixClipState::DisplayListMatrixClipState(const DlRect& cull_rect,
                                                       const DlMatrix& matrix)
    : cull_rect_(ProtectEmpty(cull_rect)), matrix_(matrix) {}

DisplayListMatrixClipState::DisplayListMatrixClipState(const SkRect& cull_rect)
    : cull_rect_(ProtectEmpty(cull_rect)), matrix_(DlMatrix()) {}

DisplayListMatrixClipState::DisplayListMatrixClipState(const SkRect& cull_rect,
                                                       const SkMatrix& matrix)
    : cull_rect_(ProtectEmpty(cull_rect)), matrix_(ToDlMatrix(matrix)) {}

DisplayListMatrixClipState::DisplayListMatrixClipState(const SkRect& cull_rect,
                                                       const SkM44& matrix)
    : cull_rect_(ProtectEmpty(cull_rect)), matrix_(ToDlMatrix(matrix)) {}

bool DisplayListMatrixClipState::inverseTransform(
    const DisplayListMatrixClipState& tracker) {
  if (tracker.is_matrix_invertable()) {
    matrix_ = matrix_ * tracker.matrix_.Invert();
    return true;
  }
  return false;
}

bool DisplayListMatrixClipState::mapAndClipRect(const SkRect& src,
                                                SkRect* mapped) const {
  DlRect dl_mapped = ToDlRect(src).TransformAndClipBounds(matrix_);
  auto dl_intersected = dl_mapped.Intersection(cull_rect_);
  if (dl_intersected.has_value()) {
    *mapped = ToSkRect(dl_intersected.value());
    return true;
  }
  mapped->setEmpty();
  return false;
}

void DisplayListMatrixClipState::clipRect(const DlRect& rect,
                                          ClipOp op,
                                          bool is_aa) {
  if (rect.IsFinite()) {
    adjustCullRect(rect, op, is_aa);
  }
}

void DisplayListMatrixClipState::clipOval(const DlRect& bounds,
                                          ClipOp op,
                                          bool is_aa) {
  if (!bounds.IsFinite()) {
    return;
  }
  switch (op) {
    case DlCanvas::ClipOp::kIntersect:
      adjustCullRect(bounds, op, is_aa);
      break;
    case DlCanvas::ClipOp::kDifference:
      if (oval_covers_cull(bounds)) {
        cull_rect_ = DlRect();
      }
      break;
  }
}

void DisplayListMatrixClipState::clipRRect(const SkRRect& rrect,
                                           ClipOp op,
                                           bool is_aa) {
  DlRect bounds = ToDlRect(rrect.getBounds());
  if (rrect.isRect()) {
    return clipRect(bounds, op, is_aa);
  }
  switch (op) {
    case ClipOp::kIntersect:
      adjustCullRect(bounds, op, is_aa);
      break;
    case ClipOp::kDifference: {
      if (rrect_covers_cull(rrect)) {
        cull_rect_ = DlRect();
        return;
      }
      auto upper_left = rrect.radii(SkRRect::kUpperLeft_Corner);
      auto upper_right = rrect.radii(SkRRect::kUpperRight_Corner);
      auto lower_left = rrect.radii(SkRRect::kLowerLeft_Corner);
      auto lower_right = rrect.radii(SkRRect::kLowerRight_Corner);
      DlRect safe = bounds.Expand(-std::max(upper_left.fX, lower_left.fX), 0,
                                  -std::max(upper_right.fX, lower_right.fX), 0);
      adjustCullRect(safe, op, is_aa);
      safe = bounds.Expand(0, -std::max(upper_left.fY, upper_right.fY),  //
                           0, -std::max(lower_left.fY, lower_right.fY));
      adjustCullRect(safe, op, is_aa);
      break;
    }
  }
}

void DisplayListMatrixClipState::clipPath(const SkPath& path,
                                          ClipOp op,
                                          bool is_aa) {
  // Map "kDifference of inverse path" to "kIntersect of the original path" and
  // map "kIntersect of inverse path" to "kDifference of the original path"
  if (path.isInverseFillType()) {
    switch (op) {
      case ClipOp::kIntersect:
        op = ClipOp::kDifference;
        break;
      case ClipOp::kDifference:
        op = ClipOp::kIntersect;
        break;
    }
  }

  DlRect bounds = ToDlRect(path.getBounds());
  if (path.isRect(nullptr)) {
    return clipRect(bounds, op, is_aa);
  }
  switch (op) {
    case ClipOp::kIntersect:
      adjustCullRect(bounds, op, is_aa);
      break;
    case ClipOp::kDifference:
      break;
  }
}

bool DisplayListMatrixClipState::content_culled(
    const DlRect& content_bounds) const {
  if (cull_rect_.IsEmpty() || content_bounds.IsEmpty()) {
    return true;
  }
  if (!is_matrix_invertable()) {
    return true;
  }
  if (has_perspective()) {
    return false;
  }
  DlRect mapped;
  mapRect(content_bounds, &mapped);
  return !mapped.IntersectsWithRect(cull_rect_);
}

void DisplayListMatrixClipState::resetDeviceCullRect(const DlRect& cull_rect) {
  if (cull_rect.IsEmpty()) {
    cull_rect_ = DlRect();
  } else {
    cull_rect_ = cull_rect;
  }
}

void DisplayListMatrixClipState::resetLocalCullRect(const DlRect& cull_rect) {
  if (!cull_rect.IsEmpty()) {
    mapRect(cull_rect, &cull_rect_);
    if (!cull_rect_.IsEmpty()) {
      return;
    }
  }
  cull_rect_ = DlRect();
}

void DisplayListMatrixClipState::adjustCullRect(const DlRect& clip,
                                                ClipOp op,
                                                bool is_aa) {
  if (cull_rect_.IsEmpty()) {
    // No point in constraining further.
    return;
  }
  if (matrix_.HasPerspective()) {
    // We can conservatively ignore this clip.
    return;
  }
  switch (op) {
    case ClipOp::kIntersect: {
      if (clip.IsEmpty()) {
        cull_rect_ = DlRect();
        break;
      }
      DlRect rect;
      mapRect(clip, &rect);
      if (is_aa) {
        rect = DlRect::RoundOut(rect);
      }
      cull_rect_ = cull_rect_.Intersection(rect).value_or(DlRect());
      break;
    }
    case ClipOp::kDifference: {
      if (clip.IsEmpty()) {
        break;
      }
      DlRect rect;
      if (mapRect(clip, &rect)) {
        // This technique only works if the transform is rect -> rect
        if (is_aa) {
          rect = DlRect::Round(rect);
          if (rect.IsEmpty()) {
            break;
          }
        }
        cull_rect_ = cull_rect_.CutoutOrEmpty(rect);
      }
      break;
    }
  }
}

SkRect DisplayListMatrixClipState::local_cull_rect() const {
  if (cull_rect_.IsEmpty()) {
    return SkRect::MakeEmpty();
  }
  if (!is_matrix_invertable()) {
    return SkRect::MakeEmpty();
  }
  if (matrix_.HasPerspective2D()) {
    // We could do a 4-point long-form conversion, but since this is
    // only used for culling, let's just return a non-constricting
    // cull rect.
    return DisplayListBuilder::kMaxCullRect;
  }
  DlMatrix inverse = matrix_.Invert();
  // We eliminated perspective above so we can use the cheaper non-clipping
  // bounds transform method.
  return ToSkRect(cull_rect_.TransformBounds(inverse));
}

bool DisplayListMatrixClipState::rect_covers_cull(const DlRect& content) const {
  if (content.IsEmpty()) {
    return false;
  }
  if (cull_rect_.IsEmpty()) {
    return true;
  }
  if (matrix_.IsAligned2D()) {
    // This transform-to-device calculation is faster and more accurate
    // for rect-to-rect aligned transformations, but not accurate under
    // (non-quadrant) rotations and skews.
    return content.TransformAndClipBounds(matrix_).Contains(cull_rect_);
  }
  DlPoint corners[4];
  if (!getLocalCullCorners(corners)) {
    return false;
  }
  for (auto corner : corners) {
    if (!content.ContainsInclusive(corner)) {
      return false;
    }
  }
  return true;
}

bool DisplayListMatrixClipState::oval_covers_cull(const DlRect& bounds) const {
  if (bounds.IsEmpty()) {
    return false;
  }
  if (cull_rect_.IsEmpty()) {
    return true;
  }
  DlPoint corners[4];
  if (!getLocalCullCorners(corners)) {
    return false;
  }
  DlPoint center = bounds.GetCenter();
  DlSize scale = 2.0 / bounds.GetSize();
  for (auto corner : corners) {
    if (!bounds.Contains(corner)) {
      return false;
    }
    if (((corner - center) * scale).GetLengthSquared() >= 1.0) {
      return false;
    }
  }
  return true;
}

bool DisplayListMatrixClipState::rrect_covers_cull(
    const SkRRect& content) const {
  if (content.isEmpty()) {
    return false;
  }
  if (cull_rect_.IsEmpty()) {
    return true;
  }
  if (content.isRect()) {
    return rect_covers_cull(content.getBounds());
  }
  if (content.isOval()) {
    return oval_covers_cull(content.getBounds());
  }
  if (!content.isSimple()) {
    return false;
  }
  DlPoint corners[4];
  if (!getLocalCullCorners(corners)) {
    return false;
  }
  auto outer = content.getBounds();
  DlScalar x_center = outer.centerX();
  DlScalar y_center = outer.centerY();
  auto radii = content.getSimpleRadii();
  DlScalar inner_x = outer.width() * 0.5f - radii.fX;
  DlScalar inner_y = outer.height() * 0.5f - radii.fY;
  DlScalar scale_x = 1.0 / radii.fX;
  DlScalar scale_y = 1.0 / radii.fY;
  for (auto corner : corners) {
    if (!outer.contains(corner.x, corner.y)) {
      return false;
    }
    DlScalar x_rel = std::abs(corner.x - x_center) - inner_x;
    DlScalar y_rel = std::abs(corner.y - y_center) - inner_y;
    if (x_rel > 0.0f && y_rel > 0.0f) {
      x_rel *= scale_x;
      y_rel *= scale_y;
      if (x_rel * x_rel + y_rel * y_rel >= 1.0f) {
        return false;
      }
    }
  }
  return true;
}

bool DisplayListMatrixClipState::getLocalCullCorners(DlPoint corners[4]) const {
  if (!is_matrix_invertable()) {
    return false;
  }
  DlMatrix inverse = matrix_.Invert();
  corners[0] = inverse * cull_rect_.GetLeftTop();
  corners[1] = inverse * cull_rect_.GetRightTop();
  corners[2] = inverse * cull_rect_.GetRightBottom();
  corners[3] = inverse * cull_rect_.GetLeftBottom();
  return true;
}

}  // namespace flutter
