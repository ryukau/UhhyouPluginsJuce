// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "scale.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cwctype>
#include <format>
#include <functional>
#include <limits>
#include <string>

namespace Uhhyou {

enum class ParameterTextRepresentation {
  normalized,
  raw,
  display,
};

inline auto formatNumber(float value, int precision) {
  return std::format("{:.{}f}", value, precision);
}

template<typename Scale> class ScaledParameter : public juce::RangedAudioParameter {
private:
  float defaultNormalized_{};
  std::atomic<float> raw_{};
  Scale& scale_; // Scale might output int, uint or double.
  juce::NormalisableRange<float> range_;

  ParameterTextRepresentation textRep_ = ParameterTextRepresentation::display;

  using ToTextFn = std::function<juce::String(float)>;
  using FromTextFn = std::function<float(const juce::String&)>;
  ToTextFn toTextFunc_;
  FromTextFn fromTextFunc_;

public:
  ScaledParameter(float defaultNormalized, Scale& scale, const juce::String& parameterId,
                  const juce::String& parameterName,
                  juce::AudioProcessorParameter::Category parameterCategory, int versionHint,
                  const juce::String& unitLabel = "",
                  ParameterTextRepresentation textRep = ParameterTextRepresentation::display,
                  ToTextFn toText = nullptr, FromTextFn fromText = nullptr)
      : RangedAudioParameter(juce::ParameterID(parameterId, versionHint), parameterName,
                             juce::AudioProcessorParameterWithIDAttributes()
                               .withCategory(parameterCategory)
                               .withLabel(unitLabel)),
        defaultNormalized_(defaultNormalized), raw_(float(scale.map(defaultNormalized))),
        scale_(scale),
        range_(
          float(scale.getMin()), float(scale.getMax()),
          [&](float, float, float normalized) { return normalizedToRaw(normalized); },
          [&](float, float, float rawValue) { return rawToNormalized(rawValue); },
          [&](float, float, float v) { return v; }),
        textRep_(textRep), toTextFunc_(std::move(toText)), fromTextFunc_(std::move(fromText)) {
    assert((toTextFunc_ == nullptr && fromTextFunc_ == nullptr)
           || (toTextFunc_ != nullptr && fromTextFunc_ != nullptr));
  }

  const juce::NormalisableRange<float>& getNormalisableRange() const override { return range_; }

  std::atomic<float>* getAtomicRaw() { return &raw_; }
  Scale& getScale() { return scale_; }
  ParameterTextRepresentation getTextRepresentation() const { return textRep_; }

  float rawToNormalized(float rawValue) const {
    return std::clamp(float(scale_.invmap(rawValue)), float(0), float(1));
  }

  float normalizedToRaw(float normalized) const {
    return float(scale_.map(std::clamp(normalized, float(0), float(1))));
  }

  float getDefaultValue() const override { return defaultNormalized_; }

private:
  float getValue() const override { return rawToNormalized(raw_); }

  void setValue(float newValue) override {
    raw_ = float(scale_.map(std::clamp(newValue, float(0), float(1))));
  }

  static constexpr int decimalDigitsF32 = std::numeric_limits<float>::digits10 + 1;

  juce::String getText(float normalized, int precision = decimalDigitsF32) const override {
    // Argument `precision` is a hack that might break in future. The original name of
    // argument is `maximumStringLength`. However, `maximumStringLength` is not documented
    // in JUCE 7.
    if (precision >= decimalDigitsF32) { precision = decimalDigitsF32; }

    if (toTextFunc_ != nullptr) { return toTextFunc_(normalized); }

    if (textRep_ == ParameterTextRepresentation::display) {
      return formatNumber(float(scale_.toDisplay(normalized)), precision);
    } else if (textRep_ == ParameterTextRepresentation::raw) {
      return formatNumber(float(scale_.map(normalized)), precision);
    }
    // `ParameterTextRepresentation::normalized` case.
    return formatNumber(normalized, precision);
  }

  float getValueForText(const juce::String& text) const override {
    if (fromTextFunc_ != nullptr) { return std::clamp(fromTextFunc_(text), float(0), float(1)); }

    auto value = text.getFloatValue();
    if (textRep_ == ParameterTextRepresentation::display) {
      value = float(scale_.fromDisplay(value));
    } else if (textRep_ == ParameterTextRepresentation::raw) {
      value = rawToNormalized(value);
    }
    // `ParameterTextRepresentation::normalized` case.
    return std::clamp(value, float(0), float(1));
  }
};

} // namespace Uhhyou
