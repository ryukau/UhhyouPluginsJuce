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
  float defaultNormalized{};
  std::atomic<float> raw{};
  Scale& scale; // Scale might output int, uint or double.
  juce::NormalisableRange<float> range;

  ParameterTextRepresentation textRep = ParameterTextRepresentation::display;

  using ToTextFn = std::function<juce::String(float)>;
  using FromTextFn = std::function<float(const juce::String&)>;
  ToTextFn toTextFunc;
  FromTextFn fromTextFunc;

public:
  ScaledParameter(float defaultNormalized, Scale& scale, const juce::String& parameterId,
                  const juce::String& parameterName,
                  juce::AudioProcessorParameter::Category category, int versionHint,
                  const juce::String& unitLabel = "",
                  ParameterTextRepresentation textRep = ParameterTextRepresentation::display,
                  ToTextFn toText = nullptr, FromTextFn fromText = nullptr)
      : RangedAudioParameter(
          juce::ParameterID(parameterId, versionHint), parameterName,
          juce::AudioProcessorParameterWithIDAttributes().withCategory(category).withLabel(
            unitLabel)),
        defaultNormalized(defaultNormalized), raw(float(scale.map(defaultNormalized))),
        scale(scale), range(
                        float(scale.getMin()), float(scale.getMax()),
                        [&](float, float, float normalized) { return normalizedToRaw(normalized); },
                        [&](float, float, float rawValue) { return rawToNormalized(rawValue); },
                        [&](float, float, float v) { return v; }),
        textRep(textRep), toTextFunc(std::move(toText)), fromTextFunc(std::move(fromText)) {
    assert((toTextFunc == nullptr && fromTextFunc == nullptr)
           || (toTextFunc != nullptr && fromTextFunc != nullptr));
  }

  const juce::NormalisableRange<float>& getNormalisableRange() const override { return range; }

  std::atomic<float>* getAtomicRaw() { return &raw; }
  Scale& getScale() { return scale; }
  ParameterTextRepresentation getTextRepresentation() const { return textRep; }

  float rawToNormalized(float rawValue) const {
    return std::clamp(float(scale.invmap(rawValue)), float(0), float(1));
  }

  float normalizedToRaw(float normalized) const {
    return float(scale.map(std::clamp(normalized, float(0), float(1))));
  }

  float getDefaultValue() const override { return defaultNormalized; }

private:
  float getValue() const override { return rawToNormalized(raw); }

  void setValue(float newValue) override {
    raw = float(scale.map(std::clamp(newValue, float(0), float(1))));
  }

  static constexpr int decimalDigitsF32 = std::numeric_limits<float>::digits10 + 1;

  juce::String getText(float normalized, int precision = decimalDigitsF32) const override {
    // Argument `precision` is a hack that might break in future. The original name of
    // argument is `maximumStringLength`. However, `maximumStringLength` is not documented
    // in JUCE 7.
    if (precision >= decimalDigitsF32) { precision = decimalDigitsF32; }

    if (toTextFunc != nullptr) { return toTextFunc(normalized); }

    if (textRep == ParameterTextRepresentation::display) {
      return formatNumber(float(scale.toDisplay(normalized)), precision);
    } else if (textRep == ParameterTextRepresentation::raw) {
      return formatNumber(float(scale.map(normalized)), precision);
    }
    // `ParameterTextRepresentation::normalized` case.
    return formatNumber(normalized, precision);
  }

  float getValueForText(const juce::String& text) const override {
    if (fromTextFunc != nullptr) { return std::clamp(fromTextFunc(text), float(0), float(1)); }

    auto value = text.getFloatValue();
    if (textRep == ParameterTextRepresentation::display) {
      value = float(scale.fromDisplay(value));
    } else if (textRep == ParameterTextRepresentation::raw) {
      value = rawToNormalized(value);
    }
    // `ParameterTextRepresentation::normalized` case.
    return std::clamp(value, float(0), float(1));
  }
};

} // namespace Uhhyou
