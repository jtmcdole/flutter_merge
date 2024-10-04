// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/toolkit/interop/color_source.h"

namespace impeller::interop {

ScopedObject<ColorSource> ColorSource::MakeLinearGradient(
    const Point& start_point,
    const Point& end_point,
    const std::vector<flutter::DlColor>& colors,
    const std::vector<Scalar>& stops,
    flutter::DlTileMode tile_mode,
    const Matrix& transformation) {
  const auto sk_transformation = ToSkMatrix(transformation);
  auto dl_filter =
      flutter::DlColorSource::MakeLinear(ToSkiaType(start_point),  //
                                         ToSkiaType(end_point),    //
                                         stops.size(),             //
                                         colors.data(),            //
                                         stops.data(),             //
                                         tile_mode,                //
                                         &sk_transformation        //
      );
  if (!dl_filter) {
    return nullptr;
  }
  return Create<ColorSource>(std::move(dl_filter));
}

ScopedObject<ColorSource> ColorSource::MakeRadialGradient(
    const Point& center,
    Scalar radius,
    const std::vector<flutter::DlColor>& colors,
    const std::vector<Scalar>& stops,
    flutter::DlTileMode tile_mode,
    const Matrix& transformation) {
  const auto sk_transformation = ToSkMatrix(transformation);
  auto dl_filter = flutter::DlColorSource::MakeRadial(ToSkiaType(center),  //
                                                      radius,              //
                                                      stops.size(),        //
                                                      colors.data(),       //
                                                      stops.data(),        //
                                                      tile_mode,           //
                                                      &sk_transformation   //
  );
  if (!dl_filter) {
    return nullptr;
  }
  return Create<ColorSource>(std::move(dl_filter));
}

ScopedObject<ColorSource> ColorSource::MakeConicalGradient(
    const Point& start_center,
    Scalar start_radius,
    const Point& end_center,
    Scalar end_radius,
    const std::vector<flutter::DlColor>& colors,
    const std::vector<Scalar>& stops,
    flutter::DlTileMode tile_mode,
    const Matrix& transformation) {
  const auto sk_transformation = ToSkMatrix(transformation);
  auto dl_filter =
      flutter::DlColorSource::MakeConical(ToSkiaType(start_center),  //
                                          start_radius,              //
                                          ToSkiaType(end_center),    //
                                          end_radius,                //
                                          stops.size(),              //
                                          colors.data(),             //
                                          stops.data(),              //
                                          tile_mode,                 //
                                          &sk_transformation         //
      );
  if (!dl_filter) {
    return nullptr;
  }
  return Create<ColorSource>(std::move(dl_filter));
}

ScopedObject<ColorSource> ColorSource::MakeSweepGradient(
    const Point& center,
    Scalar start,
    Scalar end,
    const std::vector<flutter::DlColor>& colors,
    const std::vector<Scalar>& stops,
    flutter::DlTileMode tile_mode,
    const Matrix& transformation) {
  const auto sk_transformation = ToSkMatrix(transformation);
  auto dl_filter = flutter::DlColorSource::MakeSweep(ToSkiaType(center),  //
                                                     start,               //
                                                     end,                 //
                                                     stops.size(),        //
                                                     colors.data(),       //
                                                     stops.data(),        //
                                                     tile_mode,           //
                                                     &sk_transformation   //
  );
  if (!dl_filter) {
    return nullptr;
  }
  return Create<ColorSource>(std::move(dl_filter));
}

ColorSource::ColorSource(std::shared_ptr<flutter::DlColorSource> source)
    : color_source_(std::move(source)) {}

ColorSource::~ColorSource() = default;

bool ColorSource::IsValid() const {
  return !!color_source_;
}

const std::shared_ptr<flutter::DlColorSource>& ColorSource::GetColorSource()
    const {
  return color_source_;
}

}  // namespace impeller::interop
