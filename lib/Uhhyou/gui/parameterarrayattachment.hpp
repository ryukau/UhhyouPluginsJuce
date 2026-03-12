// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <utility>

namespace Uhhyou {

/**
This is basically `ParameterAttachment`, but can handle an array of parameters instead of
a single parameter.

I thought taking `AudioProcessorParameterGroup` (APPG) at constructor is better, but APPG
doesn't have method to return `RangedAudioParameter` in JUCE 7. Maybe rewrite this when
something like `RangedAudioParameterGroup` is added to JUCE.

Advantage of using APPG is easier parameter ID management. Only a group ID is required,
instead of all parameter IDs in a group. Also template can be omitted. Disadvantage is
dynamic allocation.
*/
template<int nParameter>
class ParameterArrayAttachment : private juce::AudioProcessorParameter::Listener,
                                 private juce::AsyncUpdater {
public:
  ParameterArrayAttachment(std::array<juce::RangedAudioParameter*, nParameter> parameter,
                           std::function<void(int, float)> parameterChangedCallback,
                           juce::UndoManager* undoManager = nullptr)
      : parameter_(parameter), undoManager_(undoManager),
        parameterChangedCallback_(std::move(parameterChangedCallback)) {
    isEditing_.fill(false);

    for (auto& x : parameter_) { x->addListener(this); }

    for (int i = 0; i < nParameter; ++i) {
      indexMap_.emplace(std::make_pair(parameter_[static_cast<size_t>(i)]->getParameterIndex(), i));
    }
  }

  virtual ~ParameterArrayAttachment() override {
    for (auto& x : parameter_) { x->removeListener(this); }
    cancelPendingUpdate();
  }

  void sendInitialUpdate() {
    for (int i = 0; i < nParameter; ++i) {
      auto idx = static_cast<size_t>(i);
      parameterValueChanged(parameter_[idx]->getParameterIndex(), parameter_[idx]->getValue());
    }
  }

  void setValueAsCompleteGesture(int index, float newRawValue) {
    callIfParameterValueChanged(index, newRawValue, [this](int i, float v) {
      auto idx = static_cast<size_t>(i);
      parameter_[idx]->beginChangeGesture();
      parameter_[idx]->setValueNotifyingHost(v);
      parameter_[idx]->endChangeGesture();
    });
  }

  void setValueAsCompleteGesture(const std::array<float, nParameter>& newRawValue) {
    beginGesture();
    for (int index = 0; index < nParameter; ++index) {
      callIfParameterValueChanged(
        index, newRawValue[static_cast<size_t>(index)],
        [this](int i, float v) { parameter_[static_cast<size_t>(i)]->setValueNotifyingHost(v); });
    }
    endGesture();
  }

  void beginGesture() {
    if (undoManager_ != nullptr) { undoManager_->beginNewTransaction(); }
    for (int index = 0; index < nParameter; ++index) {
      auto idx = static_cast<size_t>(index);
      if (!isEditing_[idx]) { parameter_[idx]->beginChangeGesture(); }
    }
    isEditing_.fill(true);
  }

  void beginGesture(int index) {
    auto idx = static_cast<size_t>(index);
    if (isEditing_[idx]) { return; }
    if (undoManager_ != nullptr) { undoManager_->beginNewTransaction(); }
    parameter_[idx]->beginChangeGesture();
    isEditing_[idx] = true;
  }

  void setValueAsPartOfGesture(int index, float newRawValue) {
    callIfParameterValueChanged(index, newRawValue, [this](int i, float v) {
      parameter_[static_cast<size_t>(i)]->setValueNotifyingHost(v);
    });
  }

  void setValueAsPartOfGesture(const std::array<float, nParameter>& newRawValue) {
    for (int index = 0; index < nParameter; ++index) {
      callIfParameterValueChanged(
        index, newRawValue[static_cast<size_t>(index)],
        [this](int i, float v) { parameter_[static_cast<size_t>(i)]->setValueNotifyingHost(v); });
    }
  }

  void endGesture() {
    for (int index = 0; index < nParameter; ++index) {
      auto idx = static_cast<size_t>(index);
      if (isEditing_[idx]) { parameter_[idx]->endChangeGesture(); }
    }
    isEditing_.fill(false);
  }

  void endGesture(int index) {
    auto idx = static_cast<size_t>(index);
    parameter_[idx]->endChangeGesture();
    isEditing_[idx] = false;
  }

private:
  template<typename Callback>
  void callIfParameterValueChanged(int index, float newRawValue, Callback&& callback) {
    auto idx = static_cast<size_t>(index);
    const auto newValue = parameter_[idx]->convertTo0to1(newRawValue);
    if (parameter_[idx]->getValue() != newValue) { callback(index, newValue); }
  }

  // `parameterIndex` is a value from `AudioParameter::getParameterIndex()`.
  void parameterValueChanged(int parameterIndex, float newValue) override {
    if (!indexMap_.contains(parameterIndex)) { return; }
    auto index = indexMap_[parameterIndex];
    lastValue_[static_cast<size_t>(index)] = newValue;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      cancelPendingUpdate();
      handleAsyncUpdate(index);
    } else {
      triggerAsyncUpdate();
    }
  }

  void parameterGestureChanged(int, bool) override {}

  void handleAsyncUpdate() override {
    if (parameterChangedCallback_ != nullptr) {
      for (int i = 0; i < nParameter; ++i) {
        auto idx = static_cast<size_t>(i);
        parameterChangedCallback_(i, parameter_[idx]->convertFrom0to1(lastValue_[idx]));
      }
    }
  }

  void handleAsyncUpdate(int index) {
    if (parameterChangedCallback_ != nullptr) {
      auto idx = static_cast<size_t>(index);
      parameterChangedCallback_(index, parameter_[idx]->convertFrom0to1(lastValue_[idx]));
    }
  }

  // JUCE 7 holds parameters in a single `juce::Array`. `indexMap_` is used to map the index of
  // `juce::Array` to the internal index used in this class.
  std::unordered_map<int, int> indexMap_;

  std::array<juce::RangedAudioParameter*, nParameter> parameter_;
  std::array<bool, nParameter> isEditing_{};
  std::array<std::atomic<float>, nParameter> lastValue_{};
  juce::UndoManager* undoManager_ = nullptr;
  std::function<void(int, float)> parameterChangedCallback_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterArrayAttachment)
};

} // namespace Uhhyou
