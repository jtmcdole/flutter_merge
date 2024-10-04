// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/geometry/geometry.h"

#include <memory>
#include <optional>

#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/geometry/circle_geometry.h"
#include "impeller/entity/geometry/cover_geometry.h"
#include "impeller/entity/geometry/ellipse_geometry.h"
#include "impeller/entity/geometry/fill_path_geometry.h"
#include "impeller/entity/geometry/line_geometry.h"
#include "impeller/entity/geometry/point_field_geometry.h"
#include "impeller/entity/geometry/rect_geometry.h"
#include "impeller/entity/geometry/round_rect_geometry.h"
#include "impeller/entity/geometry/stroke_path_geometry.h"
#include "impeller/geometry/rect.h"

namespace impeller {

GeometryResult Geometry::ComputePositionGeometry(
    const ContentContext& renderer,
    const Tessellator::VertexGenerator& generator,
    const Entity& entity,
    RenderPass& pass) {
  using VT = SolidFillVertexShader::PerVertexData;

  size_t count = generator.GetVertexCount();

  return GeometryResult{
      .type = generator.GetTriangleType(),
      .vertex_buffer =
          {
              .vertex_buffer = renderer.GetTransientsBuffer().Emplace(
                  count * sizeof(VT), alignof(VT),
                  [&generator](uint8_t* buffer) {
                    auto vertices = reinterpret_cast<VT*>(buffer);
                    generator.GenerateVertices([&vertices](const Point& p) {
                      *vertices++ = {
                          .position = p,
                      };
                    });
                    FML_DCHECK(vertices == reinterpret_cast<VT*>(buffer) +
                                               generator.GetVertexCount());
                  }),
              .vertex_count = count,
              .index_type = IndexType::kNone,
          },
      .transform = entity.GetShaderTransform(pass),
  };
}

GeometryResult::Mode Geometry::GetResultMode() const {
  return GeometryResult::Mode::kNormal;
}

std::shared_ptr<Geometry> Geometry::MakeFillPath(
    const Path& path,
    std::optional<Rect> inner_rect) {
  return std::make_shared<FillPathGeometry>(path, inner_rect);
}

std::shared_ptr<Geometry> Geometry::MakePointField(std::vector<Point> points,
                                                   Scalar radius,
                                                   bool round) {
  return std::make_shared<PointFieldGeometry>(std::move(points), radius, round);
}

std::shared_ptr<Geometry> Geometry::MakeStrokePath(const Path& path,
                                                   Scalar stroke_width,
                                                   Scalar miter_limit,
                                                   Cap stroke_cap,
                                                   Join stroke_join) {
  // Skia behaves like this.
  if (miter_limit < 0) {
    miter_limit = 4.0;
  }
  return std::make_shared<StrokePathGeometry>(path, stroke_width, miter_limit,
                                              stroke_cap, stroke_join);
}

std::shared_ptr<Geometry> Geometry::MakeCover() {
  return std::make_shared<CoverGeometry>();
}

std::shared_ptr<Geometry> Geometry::MakeRect(const Rect& rect) {
  return std::make_shared<RectGeometry>(rect);
}

std::shared_ptr<Geometry> Geometry::MakeOval(const Rect& rect) {
  return std::make_shared<EllipseGeometry>(rect);
}

std::shared_ptr<Geometry> Geometry::MakeLine(const Point& p0,
                                             const Point& p1,
                                             Scalar width,
                                             Cap cap) {
  return std::make_shared<LineGeometry>(p0, p1, width, cap);
}

std::shared_ptr<Geometry> Geometry::MakeCircle(const Point& center,
                                               Scalar radius) {
  return std::make_shared<CircleGeometry>(center, radius);
}

std::shared_ptr<Geometry> Geometry::MakeStrokedCircle(const Point& center,
                                                      Scalar radius,
                                                      Scalar stroke_width) {
  return std::make_shared<CircleGeometry>(center, radius, stroke_width);
}

std::shared_ptr<Geometry> Geometry::MakeRoundRect(const Rect& rect,
                                                  const Size& radii) {
  return std::make_shared<RoundRectGeometry>(rect, radii);
}

bool Geometry::CoversArea(const Matrix& transform, const Rect& rect) const {
  return false;
}

bool Geometry::IsAxisAlignedRect() const {
  return false;
}

bool Geometry::CanApplyMaskFilter() const {
  return true;
}

// static
Scalar Geometry::ComputeStrokeAlphaCoverage(const Matrix& transform,
                                            Scalar stroke_width) {
  Scalar scaled_stroke_width = transform.GetMaxBasisLengthXY() * stroke_width;
  // If the stroke width is 0 or greater than kMinStrokeSizeMSAA, don't apply
  // any additional alpha. This is intended to match Skia behavior.
  if (scaled_stroke_width == 0.0 || scaled_stroke_width >= kMinStrokeSizeMSAA) {
    return 1.0;
  }
  // This scalling is eyeballed from Skia.
  return std::clamp(scaled_stroke_width * 2.0f, 0.f, 1.f);
}

}  // namespace impeller
