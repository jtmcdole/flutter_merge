// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_TYPOGRAPHER_GLYPH_ATLAS_H_
#define FLUTTER_IMPELLER_TYPOGRAPHER_GLYPH_ATLAS_H_

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include "impeller/core/texture.h"
#include "impeller/geometry/rect.h"
#include "impeller/typographer/font_glyph_pair.h"
#include "impeller/typographer/rectangle_packer.h"

namespace impeller {

class FontGlyphAtlas;

//------------------------------------------------------------------------------
/// @brief      A texture containing the bitmap representation of glyphs in
///             different fonts along with the ability to query the location of
///             specific font glyphs within the texture.
///
class GlyphAtlas {
 public:
  //----------------------------------------------------------------------------
  /// @brief      Describes how the glyphs are represented in the texture.
  enum class Type {
    //--------------------------------------------------------------------------
    /// The glyphs are reprsented at their requested size using only an 8-bit
    /// color channel.
    ///
    /// This might be backed by a grey or red single channel texture, depending
    /// on the backend capabilities.
    kAlphaBitmap,

    //--------------------------------------------------------------------------
    /// The glyphs are reprsented at their requested size using N32 premul
    /// colors.
    ///
    kColorBitmap,
  };

  //----------------------------------------------------------------------------
  /// @brief      Create an empty glyph atlas.
  ///
  /// @param[in]  type  How the glyphs are represented in the texture.
  ///
  explicit GlyphAtlas(Type type);

  ~GlyphAtlas();

  bool IsValid() const;

  //----------------------------------------------------------------------------
  /// @brief      Describes how the glyphs are represented in the texture.
  ///
  Type GetType() const;

  //----------------------------------------------------------------------------
  /// @brief      Set the texture for the glyph atlas.
  ///
  /// @param[in]  texture  The texture
  ///
  void SetTexture(std::shared_ptr<Texture> texture);

  //----------------------------------------------------------------------------
  /// @brief      Get the texture for the glyph atlas.
  ///
  /// @return     The texture.
  ///
  const std::shared_ptr<Texture>& GetTexture() const;

  //----------------------------------------------------------------------------
  /// @brief      Record the location of a specific font-glyph pair within the
  ///             atlas.
  ///
  /// @param[in]  pair  The font-glyph pair
  /// @param[in]  rect  The position in the atlas
  /// @param[in]  bounds The bounds of the glyph at scale
  ///
  void AddTypefaceGlyphPositionAndBounds(const FontGlyphPair& pair,
                                         Rect position,
                                         Rect bounds);

  //----------------------------------------------------------------------------
  /// @brief      Get the number of unique font-glyph pairs in this atlas.
  ///
  /// @return     The glyph count.
  ///
  size_t GetGlyphCount() const;

  //----------------------------------------------------------------------------
  /// @brief      Iterate of all the glyphs along with their locations in the
  ///             atlas.
  ///
  /// @param[in]  iterator  The iterator. Return `false` from the iterator to
  ///                       stop iterating.
  ///
  /// @return     The number of glyphs iterated over.
  ///
  size_t IterateGlyphs(
      const std::function<bool(const ScaledFont& scaled_font,
                               const SubpixelGlyph& glyph,
                               const Rect& rect)>& iterator) const;

  //----------------------------------------------------------------------------
  /// @brief      Find the location of a specific font-glyph pair in the atlas.
  ///
  /// @param[in]  pair  The font-glyph pair
  ///
  /// @return     The location of the font-glyph pair in the atlas.
  ///             `std::nullopt` if the pair is not in the atlas.
  ///
  std::optional<std::pair<Rect, Rect>> FindFontGlyphBounds(
      const FontGlyphPair& pair) const;

  //----------------------------------------------------------------------------
  /// @brief      Obtain an interface for querying the location of glyphs in the
  ///             atlas for the given font and scale.  This provides a more
  ///             efficient way to look up a run of glyphs in the same font.
  ///
  /// @param[in]  font  The font
  /// @param[in]  scale The scale
  ///
  /// @return     A pointer to a FontGlyphAtlas, or nullptr if the font and
  ///             scale are not available in the atlas.  The pointer is only
  ///             valid for the lifetime of the GlyphAtlas.
  ///
  const FontGlyphAtlas* GetFontGlyphAtlas(const Font& font, Scalar scale) const;

 private:
  const Type type_;
  std::shared_ptr<Texture> texture_;

  std::unordered_map<ScaledFont,
                     FontGlyphAtlas,
                     ScaledFont::Hash,
                     ScaledFont::Equal>
      font_atlas_map_;

  GlyphAtlas(const GlyphAtlas&) = delete;

  GlyphAtlas& operator=(const GlyphAtlas&) = delete;
};

//------------------------------------------------------------------------------
/// @brief      A container for caching a glyph atlas across frames.
///
class GlyphAtlasContext {
 public:
  explicit GlyphAtlasContext(GlyphAtlas::Type type);

  virtual ~GlyphAtlasContext();

  //----------------------------------------------------------------------------
  /// @brief      Retrieve the current glyph atlas.
  std::shared_ptr<GlyphAtlas> GetGlyphAtlas() const;

  //----------------------------------------------------------------------------
  /// @brief      Retrieve the size of the current glyph atlas.
  const ISize& GetAtlasSize() const;

  //----------------------------------------------------------------------------
  /// @brief      Retrieve the previous (if any) rect packer.
  std::shared_ptr<RectanglePacker> GetRectPacker() const;

  //----------------------------------------------------------------------------
  /// @brief      A y-coordinate shift that must be applied to glyphs appended
  /// to
  ///             the atlas.
  ///
  ///             The rectangle packer is only initialized for unfilled regions
  ///             of the atlas. The area the rectangle packer covers is offset
  ///             from the origin by this height adjustment.
  int64_t GetHeightAdjustment() const;

  //----------------------------------------------------------------------------
  /// @brief      Update the context with a newly constructed glyph atlas.
  void UpdateGlyphAtlas(std::shared_ptr<GlyphAtlas> atlas,
                        ISize size,
                        int64_t height_adjustment_);

  void UpdateRectPacker(std::shared_ptr<RectanglePacker> rect_packer);

 private:
  std::shared_ptr<GlyphAtlas> atlas_;
  ISize atlas_size_;
  std::shared_ptr<RectanglePacker> rect_packer_;
  int64_t height_adjustment_;

  GlyphAtlasContext(const GlyphAtlasContext&) = delete;

  GlyphAtlasContext& operator=(const GlyphAtlasContext&) = delete;
};

//------------------------------------------------------------------------------
/// @brief      An object that can look up glyph locations within the GlyphAtlas
///             for a particular typeface.
///
class FontGlyphAtlas {
 public:
  FontGlyphAtlas() = default;

  //----------------------------------------------------------------------------
  /// @brief      Find the location of a glyph in the atlas.
  ///
  /// @param[in]  glyph The glyph
  ///
  /// @return     The location of the glyph in the atlas.
  ///             `std::nullopt` if the glyph is not in the atlas.
  ///
  std::optional<std::pair<Rect, Rect>> FindGlyphBounds(
      const SubpixelGlyph& glyph) const;

 private:
  friend class GlyphAtlas;
  std::unordered_map<SubpixelGlyph,
                     std::pair<Rect, Rect>,
                     SubpixelGlyph::Hash,
                     SubpixelGlyph::Equal>
      positions_;

  FontGlyphAtlas(const FontGlyphAtlas&) = delete;

  FontGlyphAtlas& operator=(const FontGlyphAtlas&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_TYPOGRAPHER_GLYPH_ATLAS_H_
