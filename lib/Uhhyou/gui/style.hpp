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
    return fontType == FontType::monospace ? fontMonoArray_[static_cast<size_t>(size)]
                                           : fontUiArray_[static_cast<size_t>(size)];
  }

  float getFontHeight(TextSize size, FontType fontType = FontType::ui) const {
    return getFont(size, fontType).getHeight();
  }

  void resize(float scale) {
    scalingFactor_ = scale;
    const float sizes[] = {10.0f, 14.0f, 20.0f};

    for (size_t i = 0; i < static_cast<size_t>(TextSize::_count); ++i) {
      float h = sizes[i] * scalingFactor_;
      fontUiArray_[i] = juce::Font(juce::FontOptions{fontUiName(), fontUiStyle(), h}.withFallbacks(
        {"Segoe UI", ".AppleSystemUIFont", "DejaVu Sans", "Verdana", "sans-serif"}));
      fontMonoArray_[i]
        = juce::Font(juce::FontOptions{fontMonoName(), fontMonoStyle(), h}.withFallbacks(
          {"Consolas", "Menlo", "DejaVu Sans Mono", "mono"}));
    }

    borderWidthScaled_ = scalingFactor_ * borderWidth_;
  }

  // Global GUI Settings.
  bool keyboardFocusEnabled() const { return keyboardFocusEnabled_; }
  float windowScale() const { return windowScale_; }

  // Look and feel.
  const juce::String& fontUiName() const { return fontUiName_; }
  const juce::String& fontUiStyle() const { return fontUiStyle_; }
  const juce::String& fontMonoName() const { return fontMonoName_; }
  const juce::String& fontMonoStyle() const { return fontMonoStyle_; }
  float borderWidth() const { return borderWidthScaled_; }
  const juce::Colour& foreground() const { return foreground_; }
  const juce::Colour& background() const { return background_; }
  const juce::Colour& surface() const { return surface_; }
  const juce::Colour& border() const { return border_; }
  const juce::Colour& main() const { return main_; }
  const juce::Colour& accent() const { return accent_; }
  const juce::Colour& warning() const { return warning_; }

private:
  static constexpr unsigned fontMapKeyScaling = 10;
  float scalingFactor_ = float(1);
  std::array<juce::Font, static_cast<size_t>(TextSize::_count)> fontUiArray_;
  std::array<juce::Font, static_cast<size_t>(TextSize::_count)> fontMonoArray_;

  float borderWidth_ = 1;
  float borderWidthScaled_ = borderWidth_;

  bool keyboardFocusEnabled_ = true;
  float windowScale_ = 1.0f;

  juce::String fontUiName_{"Ubuntu"};
  juce::String fontUiStyle_{"Regular"};
  juce::String fontMonoName_{"Ubuntu Mono"};
  juce::String fontMonoStyle_{"Regular"};
  juce::Colour foreground_{juce::uint32{0xff000000}};
  juce::Colour background_{juce::uint32{0xffffffff}};
  juce::Colour surface_{juce::uint32{0xfff8f8f8}};
  juce::Colour border_{juce::uint32{0xff888888}};
  juce::Colour main_{juce::uint32{0xfffcc04f}};
  juce::Colour accent_{juce::uint32{0xff13c136}};
  juce::Colour warning_{juce::uint32{0xfffc8080}};
};

} // namespace Uhhyou
