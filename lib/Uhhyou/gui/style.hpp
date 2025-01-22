// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_graphics/juce_graphics.h>

#include <memory>
#include <unordered_map>

namespace Uhhyou {

enum class Style { common, accent, warning };

class Palette {
public:
  Palette() { load(); }
  void load();

  const juce::Font &getFont(float size)
  {
    auto key = size_t(fontMapKeyScaling * size);
    auto found = fontMap.find(key);
    if (found != fontMap.end()) return found->second;
    auto inserted = fontMap.emplace(
      key,
      juce::Font(juce::FontOptions{
        fontName(), fontFace(), key * scalingFactor / fontMapKeyScaling}));
    return inserted.first->second;
  }

  void resize(float scale)
  {
    scalingFactor = scale;

    fontMap.clear();
    std::vector<size_t> sizes{
      size_t(textSizeSmall() * fontMapKeyScaling),
      size_t(textSizeUi() * fontMapKeyScaling),
      size_t(textSizeBig() * fontMapKeyScaling),
    };

    for (const auto &key : sizes) {
      fontMap.emplace(
        key,
        juce::Font(juce::FontOptions{
          fontName(), fontFace(), key * scalingFactor / fontMapKeyScaling}));
    }

    _borderThinScaled = scalingFactor * _borderThin;
    _borderThickScaled = scalingFactor * _borderThick;
  }

  const juce::String &fontName() { return _fontName; }
  const juce::String &fontFace() { return _fontFace; }
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
  std::unordered_map<size_t, juce::Font> fontMap;

  float _borderThin = 1;
  float _borderThick = 8;
  float _borderThinScaled = _borderThin;
  float _borderThickScaled = _borderThick;

  juce::String _fontName{"Tinos"};
  juce::String _fontFace{"Bold Italic"};
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
