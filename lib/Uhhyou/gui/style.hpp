// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

// Note that changing this source leads to slow compilation due to nlohmann/json.hpp.

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>

namespace Uhhyou {

enum class Style { common, accent, warning };
enum class FontType { ui, monospace };
enum class TextSize { small, normal, large, _count };

class Palette {
public:
  Palette() { load(); }
  void load();
  void updateSetting(const std::string& key, bool value);
  void updateSetting(const std::string& key, float value);

  const juce::Font& getFont(TextSize size, FontType fontType = FontType::ui) const {
    return fontType == FontType::monospace ? fontMonoArray[static_cast<size_t>(size)]
                                           : fontUiArray[static_cast<size_t>(size)];
  }

  float getFontHeight(TextSize size, FontType fontType = FontType::ui) const {
    return getFont(size, fontType).getHeight();
  }

  void resize(float scale) {
    scalingFactor = scale;
    const float sizes[] = {10.0f, 14.0f, 20.0f};

    for (size_t i = 0; i < static_cast<size_t>(TextSize::_count); ++i) {
      float h = sizes[i] * scalingFactor;
      fontUiArray[i] = juce::Font(juce::FontOptions{fontUiName(), fontUiStyle(), h}.withFallbacks(
        {"Segoe UI", ".AppleSystemUIFont", "DejaVu Sans", "Verdana", "sans-serif"}));
      fontMonoArray[i]
        = juce::Font(juce::FontOptions{fontMonoName(), fontMonoStyle(), h}.withFallbacks(
          {"Consolas", "Menlo", "DejaVu Sans Mono", "mono"}));
    }

    _borderWidthScaled = scalingFactor * _borderWidth;
  }

  // Global GUI Settings.
  bool keyboardFocusEnabled() const { return _keyboardFocusEnabled; }
  float windowScale() const { return _windowScale; }

  // Look and feel.
  const juce::String& fontUiName() const { return _fontUiName; }
  const juce::String& fontUiStyle() const { return _fontUiStyle; }
  const juce::String& fontMonoName() const { return _fontMonoName; }
  const juce::String& fontMonoStyle() const { return _fontMonoStyle; }
  float borderWidth() const { return _borderWidthScaled; }
  const juce::Colour& foreground() const { return _foreground; }
  const juce::Colour& background() const { return _background; }
  const juce::Colour& surface() const { return _surface; }
  const juce::Colour& border() const { return _border; }
  const juce::Colour& main() const { return _main; }
  const juce::Colour& accent() const { return _accent; }
  const juce::Colour& warning() const { return _warning; }

private:
  static constexpr unsigned fontMapKeyScaling = 10;
  float scalingFactor = float(1);
  std::array<juce::Font, static_cast<size_t>(TextSize::_count)> fontUiArray;
  std::array<juce::Font, static_cast<size_t>(TextSize::_count)> fontMonoArray;

  float _borderWidth = 1;
  float _borderWidthScaled = _borderWidth;

  bool _keyboardFocusEnabled = true;
  float _windowScale = 1.0f;

  juce::String _fontUiName{"Ubuntu"};
  juce::String _fontUiStyle{"Regular"};
  juce::String _fontMonoName{"Ubuntu Mono"};
  juce::String _fontMonoStyle{"Regular"};
  juce::Colour _foreground{juce::uint32{0xff000000}};
  juce::Colour _background{juce::uint32{0xffffffff}};
  juce::Colour _surface{juce::uint32{0xfff8f8f8}};
  juce::Colour _border{juce::uint32{0xff888888}};
  juce::Colour _main{juce::uint32{0xfffcc04f}};
  juce::Colour _accent{juce::uint32{0xff13c136}};
  juce::Colour _warning{juce::uint32{0xfffc8080}};
};

} // namespace Uhhyou
