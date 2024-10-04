// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

precision highp float;

#include <impeller/gaussian.glsl>
#include <impeller/types.glsl>

uniform FragInfo {
  f16vec4 color;
  vec2 rect_size;
  float blur_sigma;
  vec2 corner_radii;
}
frag_info;

in vec2 v_position;

out f16vec4 frag_color;

const int kSampleCount = 4;

/// Closed form unidirectional rounded rect blur mask solution using the
/// analytical Gaussian integral (with approximated erf).
vec4 RRectBlurX(float sample_position_x,
                vec4 sample_position_y,
                vec2 half_size) {
  // The vertical edge of the rrect consists of a flat portion and a curved
  // portion, the two of which vary in size depending on the size of the
  // corner radii, both adding up to half_size.y.
  // half_size.y - corner_radii.y is the size of the vertical flat
  // portion of the rrect.
  // subtracting the absolute value of the Y sample_position will be
  // negative (and then clamped to 0) for positions that are located
  // vertically in the flat part of the rrect, and will be the relative
  // distance from the center of curvature otherwise.
  vec4 space_y = min(vec4(0.0), half_size.y - frag_info.corner_radii.y -
                                    abs(sample_position_y));
  // space is now in the range [0.0, corner_radii.y]. If the y sample was
  // in the flat portion of the rrect, it will be 0.0

  // We will now calculate rrect_distance as the distance from the centerline
  // of the rrect towards the near side of the rrect.
  // half_size.x - frag_info.corner_radii.x is the size of the horizontal
  // flat portion of the rrect.
  // We add to that the X size (space_x) of the curved corner measured at
  // the indicated Y coordinate we calculated as space_y, such that:
  //   (space_y / corner_radii.y)^2 + (space_x / corner_radii.x)^2 == 1.0
  // Since we want the space_x, we rearrange the equation as:
  //   space_x = corner_radii.x * sqrt(1.0 - (space_y / corner_radii.y)^2)
  // We need to prevent negative values inside the sqrt which can occur
  // when the Y sample was beyond the vertical size of the rrect and thus
  // space_y was larger than corner_radii.y.
  // The calling function RRectBlur will never provide a Y sample outside
  // of that range, though, so the max(0.0) is mostly a precaution.
  vec4 unit_space_y = space_y / frag_info.corner_radii.y;
  vec4 unit_space_x = sqrt(max(vec4(0.0), 1.0 - unit_space_y * unit_space_y));
  vec4 rrect_distance =
      half_size.x - frag_info.corner_radii.x * (1.0 - unit_space_x);

  vec4 result;
  // Now we integrate the Gaussian over the range of the relative positions
  // of the left and right sides of the rrect relative to the sampling
  // X coordinate.
  vec4 integral = IPVec4FastGaussianIntegral(
      float(sample_position_x) + vec4(-rrect_distance[0], rrect_distance[0],
                                      -rrect_distance[1], rrect_distance[1]),
      float(frag_info.blur_sigma));
  // integral.y contains the evaluation of the indefinite gaussian integral
  // function at (X + rrect_distance) and integral.x contains the evaluation
  // of it at (X - rrect_distance). Subtracting the two produces the
  // integral result over the range from one to the other.
  result.xy = integral.yw - integral.xz;
  integral = IPVec4FastGaussianIntegral(
      float(sample_position_x) + vec4(-rrect_distance[2], rrect_distance[2],
                                      -rrect_distance[3], rrect_distance[3]),
      float(frag_info.blur_sigma));
  result.zw = integral.yw - integral.xz;

  return result;
}

float RRectBlur(vec2 sample_position, vec2 half_size) {
  // Limit the sampling range to 3 standard deviations in the Y direction from
  // the kernel center to incorporate 99.7% of the color contribution.
  float half_sampling_range = frag_info.blur_sigma * 3.0;

  // We want to cover the range [Y - half_range, Y + half_range], but we
  // don't want to sample beyond the edge of the rrect (where the RRectBlurX
  // function produces bad information and where the real answer at those
  // locations will be 0.0 anyway).
  float begin_y = max(-half_sampling_range, sample_position.y - half_size.y);
  float end_y = min(half_sampling_range, sample_position.y + half_size.y);
  float interval = (end_y - begin_y) / kSampleCount;

  // Sample the X blur kSampleCount times, weighted by the Gaussian function.
  vec4 ys = vec4(0.5, 1.5, 2.5, 3.5) * interval + begin_y;
  vec4 sample_ys = sample_position.y - ys;
  vec4 blurx = RRectBlurX(sample_position.x, sample_ys, half_size);
  vec4 gaussian_y = IPGaussian(ys, float(frag_info.blur_sigma));
  return dot(blurx, gaussian_y * interval);
}

void main() {
  frag_color = frag_info.color;

  vec2 half_size = frag_info.rect_size * 0.5;
  vec2 sample_position = v_position - half_size;

  frag_color *= float16_t(RRectBlur(sample_position, half_size));
}
