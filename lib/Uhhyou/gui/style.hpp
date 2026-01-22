// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_graphics/juce_graphics.h>

#include <memory>
#include <unordered_map>

namespace Uhhyou {

enum class Style { common, accent, warning };
enum class FontType { ui, monospace };

class Palette {
private:
  using FontMap = std::unordered_map<unsigned, juce::Font>;

public:
  Palette() { load(); }
  void load();

  const juce::Font &getFont(float size, FontType fontType = FontType::ui)
  {
    auto findFont = [&](
                      FontMap &fmap, const juce::String &name, const juce::String &style,
                      std::vector<juce::String> fallbacks) -> juce::Font &
    {
      auto key = unsigned(fontMapKeyScaling * size);
      auto found = fmap.find(key);
      if (found != fmap.end()) return found->second;

      const auto height = key * scalingFactor / fontMapKeyScaling;
      auto inserted = fmap.emplace(
        key, juce::Font(juce::FontOptions{name, style, height}.withFallbacks(fallbacks)));

      return inserted.first->second;
    };

    if (fontType == FontType::monospace) {
      return findFont(
        fontMonoMap, fontMonoName(), fontMonoStyle(),
        {"Consolas", "Menlo", "DejaVu Sans Mono", "mono"});
    }
    return findFont(
      fontUiMap, fontUiName(), fontUiStyle(),
      {"Segoe UI", ".AppleSystemUIFont", "DejaVu Sans", "Verdana", "sans-serif"});
  }

  void resize(float scale)
  {
    scalingFactor = scale;

    auto reconstruct
      = [&](FontMap &fontMap, const juce::String &name, const juce::String &style)
    {
      fontMap.clear();
      std::vector<unsigned> sizes{
        unsigned(textSizeSmall() * fontMapKeyScaling),
        unsigned(textSizeUi() * fontMapKeyScaling),
        unsigned(textSizeBig() * fontMapKeyScaling),
      };
      for (const auto &key : sizes) {
        const auto height = key * scalingFactor / fontMapKeyScaling;
        fontMap.emplace(key, juce::Font(juce::FontOptions{name, style, height}));
      }
    };

    reconstruct(fontUiMap, fontUiName(), fontUiStyle());
    reconstruct(fontMonoMap, fontMonoName(), fontMonoStyle());

    _borderThinScaled = scalingFactor * _borderThin;
    _borderThickScaled = scalingFactor * _borderThick;
  }

  const juce::String &fontUiName() { return _fontUiName; }
  const juce::String &fontUiStyle() { return _fontUiStyle; }
  const juce::String &fontMonoName() { return _fontMonoName; }
  const juce::String &fontMonoStyle() { return _fontMonoStyle; }
  const float textSizeSmall() { return 10; }
  const float textSizeUi() { return 14; }
  const float textSizeBig() { return 20; }
  const float borderThin() { return _borderThinScaled; }
  const float borderThick() { return _borderThickScaled; }
  const juce::Colour &foreground() { return _foreground; }
  const juce::Colour &foregroundButtonOn() { return _foregroundButtonOn; }
  const juce::Colour &foregroundInactive() { return _foregroundInactive; }
  const juce::Colour &background() { return _background; }
  const juce::Colour &boxBackground() { return _boxBackground; }
  const juce::Colour &border() { return _border; }
  const juce::Colour &borderCheckbox() { return _borderCheckbox; }
  const juce::Colour &borderLabel() { return _borderLabel; }
  const juce::Colour &unfocused() { return _unfocused; }
  const juce::Colour &highlightMain() { return _highlightMain; }
  const juce::Colour &highlightAccent() { return _highlightAccent; }
  const juce::Colour &highlightButton() { return _highlightButton; }
  const juce::Colour &highlightWarning() { return _highlightWarning; }
  const juce::Colour &overlay() { return _overlay; }
  const juce::Colour &overlayHighlight() { return _overlayHighlight; }
  const juce::Colour &overlayFaint() { return _overlayFaint; }

private:
  static constexpr unsigned fontMapKeyScaling = 10;
  float scalingFactor = float(1);
  FontMap fontUiMap;
  FontMap fontMonoMap;

  float _borderThin = 1;
  float _borderThick = 8;
  float _borderThinScaled = _borderThin;
  float _borderThickScaled = _borderThick;

  juce::String _fontUiName{"Ubuntu"};
  juce::String _fontUiStyle{"Regular"};
  juce::String _fontMonoName{"Ubuntu Mono"};
  juce::String _fontMonoStyle{"Regular"};
  juce::Colour _foreground{juce::uint32{0xff000000}};
  juce::Colour _foregroundButtonOn{juce::uint32{0xff000000}};
  juce::Colour _foregroundInactive{juce::uint32{0xff8a8a8a}};
  juce::Colour _background{juce::uint32{0xffffffff}};
  juce::Colour _boxBackground{juce::uint32{0xffffffff}};
  juce::Colour _border{juce::uint32{0xff000000}};
  juce::Colour _borderCheckbox{juce::uint32{0xff000000}};
  juce::Colour _borderLabel{juce::uint32{0xff000000}};
  juce::Colour _unfocused{juce::uint32{0xffdddddd}};
  juce::Colour _highlightMain{juce::uint32{0xff0ba4f1}};
  juce::Colour _highlightAccent{juce::uint32{0xff13c136}};
  juce::Colour _highlightButton{juce::uint32{0xfffcc04f}};
  juce::Colour _highlightWarning{juce::uint32{0xfffc8080}};
  juce::Colour _overlay{juce::uint32{0x88000000}};
  juce::Colour _overlayHighlight{juce::uint32{0x3300ff00}};
  juce::Colour _overlayFaint{juce::uint32{0x0b000000}};
};

} // namespace Uhhyou
