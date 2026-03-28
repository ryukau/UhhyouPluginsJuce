// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

// Note that changing this source leads to slow compilation due to nlohmann/json.hpp.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>

namespace Uhhyou {

enum class Style { main, accent, warning, background, surface, border, _count };
enum class FontType { ui, monospace };
enum class TextSize { small, normal, large, _count };

class StyleNotifier : public juce::ChangeBroadcaster {
public:
  static StyleNotifier& getInstance() {
    static StyleNotifier instance;
    return instance;
  }
};

class Palette {
public:
  Palette() { load(); }
  void load();
  void updateSetting(const std::string& key, bool value);
  void updateSetting(const std::string& key, float value);

  // This must be called anytime a color property changes.
  void updateCaches() {
    foregroundLightLuminance_ = getRelativeLuminance(foregroundLight_);
    foregroundDarkLuminance_ = getRelativeLuminance(foregroundDark_);

    auto calcFg = [this](const juce::Colour& bg) { return getForeground(bg); };

    styleFgCache_[static_cast<size_t>(Style::main)] = calcFg(main_);
    styleFgCache_[static_cast<size_t>(Style::accent)] = calcFg(accent_);
    styleFgCache_[static_cast<size_t>(Style::warning)] = calcFg(warning_);
    styleFgCache_[static_cast<size_t>(Style::background)] = calcFg(background_);
    styleFgCache_[static_cast<size_t>(Style::surface)] = calcFg(surface_);
    styleFgCache_[static_cast<size_t>(Style::border)] = calcFg(border_);
  }

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

  const juce::Colour& foregroundLight() const { return foregroundLight_; }
  const juce::Colour& foregroundDark() const { return foregroundDark_; }
  const juce::Colour& background() const { return background_; }
  const juce::Colour& surface() const { return surface_; }
  const juce::Colour& border() const { return border_; }
  const juce::Colour& main() const { return main_; }
  const juce::Colour& accent() const { return accent_; }
  const juce::Colour& warning() const { return warning_; }

  template<Style style> const juce::Colour& getColor() const {
    static_assert(style != Style::_count, "Invalid color style");
    if constexpr (style == Style::main) {
      return main_;
    } else if constexpr (style == Style::accent) {
      return accent_;
    } else if constexpr (style == Style::warning) {
      return warning_;
    } else if constexpr (style == Style::background) {
      return background_;
    } else if constexpr (style == Style::surface) {
      return surface_;
    } else if constexpr (style == Style::border) {
      return border_;
    } else {
      return main_;
    }
  }

  template<Style style> const juce::Colour& getForeground() const {
    static_assert(style != Style::_count, "Invalid foreground style");
    return styleFgCache_[static_cast<size_t>(style)];
  }

  const juce::Colour& getForeground(const juce::Colour& bgColour) const {
    float bgLum = getRelativeLuminance(bgColour);
    float contrastLight = (std::max(bgLum, foregroundLightLuminance_) + 0.05f)
      / (std::min(bgLum, foregroundLightLuminance_) + 0.05f);
    float contrastDark = (std::max(bgLum, foregroundDarkLuminance_) + 0.05f)
      / (std::min(bgLum, foregroundDarkLuminance_) + 0.05f);
    return contrastLight > contrastDark ? foregroundLight_ : foregroundDark_;
  }

  juce::File getStyleDirectory() const;
  void loadStyleFromFile(const juce::File& file);

private:
  void saveStyleJson();

  template<typename Visitor> void visitProperties(Visitor&& visitor) {
    visitor("keyboardFocusEnabled", keyboardFocusEnabled_);
    visitor("windowScale", windowScale_);
    visitor("fontUiName", fontUiName_);
    visitor("fontUiStyle", fontUiStyle_);
    visitor("fontMonoName", fontMonoName_);
    visitor("fontMonoStyle", fontMonoStyle_);
    visitor("foregroundLight", foregroundLight_);
    visitor("foregroundDark", foregroundDark_);
    visitor("background", background_);
    visitor("surface", surface_);
    visitor("border", border_);
    visitor("main", main_);
    visitor("accent", accent_);
    visitor("warning", warning_);
  }

  static float getRelativeLuminance(const juce::Colour& c) {
    static const std::array<float, 256> lut = []() {
      std::array<float, 256> l{};
      for (size_t i = 0; i < 256; ++i) {
        float v = static_cast<float>(i) / 255.0f;
        l[i] = v <= 0.03928f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
      }
      return l;
    }();
    return 0.2126f * lut[c.getRed()] + 0.7152f * lut[c.getGreen()] + 0.0722f * lut[c.getBlue()];
  }

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

  juce::Colour foregroundLight_{juce::uint32{0xffffffff}};
  juce::Colour foregroundDark_{juce::uint32{0xff000000}};
  juce::Colour background_{juce::uint32{0xffffffff}};
  juce::Colour surface_{juce::uint32{0xfff8f8f8}};
  juce::Colour border_{juce::uint32{0xff888888}};
  juce::Colour main_{juce::uint32{0xfffcc04f}};
  juce::Colour accent_{juce::uint32{0xff13c136}};
  juce::Colour warning_{juce::uint32{0xfffc8080}};

  // --- Caching ---
  float foregroundLightLuminance_ = 1.0f;
  float foregroundDarkLuminance_ = 0.0f;
  std::array<juce::Colour, static_cast<size_t>(Style::_count)> styleFgCache_;
};

} // namespace Uhhyou
