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
template<size_t nParameter>
class ParameterArrayAttachment : private juce::AudioProcessorParameter::Listener,
                                 private juce::AsyncUpdater {
public:
  ParameterArrayAttachment(
    std::array<juce::RangedAudioParameter *, nParameter> parameter,
    std::function<void(int, float)> parameterChangedCallback,
    juce::UndoManager *undoManager = nullptr)
    : parameter(parameter)
    , undoManager(undoManager)
    , parameterChangedCallback(std::move(parameterChangedCallback))
  {
    isEditing.fill(false);

    for (auto &x : parameter) x->addListener(this);

    for (int i = 0; i < nParameter; ++i)
      indexMap.emplace(std::make_pair(parameter[i]->getParameterIndex(), i));
  }

  virtual ~ParameterArrayAttachment() override
  {
    for (auto &x : parameter) x->removeListener(this);
    cancelPendingUpdate();
  }

  void sendInitialUpdate()
  {
    for (int i = 0; i < nParameter; ++i)
      parameterValueChanged(parameter[i]->getParameterIndex(), parameter[i]->getValue());
  }

  void setValueAsCompleteGesture(int index, float newRawValue)
  {
    callIfParameterValueChanged(index, newRawValue, [this](int i, float v) {
      parameter[i]->beginChangeGesture();
      parameter[i]->setValueNotifyingHost(v);
      parameter[i]->endChangeGesture();
    });
  }

  void setValueAsCompleteGesture(const std::array<float, nParameter> &newRawValue)
  {
    beginGesture();
    for (int index = 0; index < nParameter; ++index) {
      callIfParameterValueChanged(index, newRawValue[index], [this](int i, float v) {
        parameter[i]->setValueNotifyingHost(v);
      });
    }
    endGesture();
  }

  void beginGesture()
  {
    if (undoManager != nullptr) undoManager->beginNewTransaction();
    for (int index = 0; index < nParameter; ++index) {
      if (!isEditing[index]) parameter[index]->beginChangeGesture();
    }
    isEditing.fill(true);
  }

  void beginGesture(int index)
  {
    if (isEditing[index]) return;
    if (undoManager != nullptr) undoManager->beginNewTransaction();
    parameter[index]->beginChangeGesture();
    isEditing[index] = true;
  }

  void setValueAsPartOfGesture(int index, float newRawValue)
  {
    callIfParameterValueChanged(index, newRawValue, [this](int i, float v) {
      parameter[i]->setValueNotifyingHost(v);
    });
  }

  void setValueAsPartOfGesture(const std::array<float, nParameter> &newRawValue)
  {
    for (int index = 0; index < nParameter; ++index) {
      callIfParameterValueChanged(index, newRawValue[index], [this](int i, float v) {
        parameter[i]->setValueNotifyingHost(v);
      });
    }
  }

  void endGesture()
  {
    for (int index = 0; index < nParameter; ++index) {
      if (isEditing[index]) parameter[index]->endChangeGesture();
    }
    isEditing.fill(false);
  }

  void endGesture(int index)
  {
    parameter[index]->endChangeGesture();
    isEditing[index] = false;
  }

private:
  template<typename Callback>
  void callIfParameterValueChanged(int index, float newRawValue, Callback &&callback)
  {
    const auto newValue = parameter[index]->convertTo0to1(newRawValue);
    if (parameter[index]->getValue() != newValue) callback(index, newValue);
  }

  // `parameterIndex` is a value from `AudioParameter::getParameterIndex()`.
  void parameterValueChanged(int parameterIndex, float newValue) override
  {
    if (!indexMap.contains(parameterIndex)) return;
    auto index = indexMap[parameterIndex];
    lastValue[index] = newValue;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      cancelPendingUpdate();
      handleAsyncUpdate(index);
    } else {
      triggerAsyncUpdate();
    }
  }

  void parameterGestureChanged(int, bool) override {}

  void handleAsyncUpdate() override
  {
    if (parameterChangedCallback != nullptr)
      for (int i = 0; i < nParameter; ++i)
        parameterChangedCallback(i, parameter[i]->convertFrom0to1(lastValue[i]));
  }

  void handleAsyncUpdate(int index)
  {
    if (parameterChangedCallback != nullptr) {
      parameterChangedCallback(
        index, parameter[index]->convertFrom0to1(lastValue[index]));
    }
  }

  // JUCE 7 holds parameters in a single `juce::Array`. `indexMap` is used to map the
  // index of `juce::Array` to the internal index used in this class.
  std::unordered_map<int, int> indexMap;

  std::array<juce::RangedAudioParameter *, nParameter> parameter;
  std::array<bool, nParameter> isEditing{};
  std::array<std::atomic<float>, nParameter> lastValue{};
  juce::UndoManager *undoManager = nullptr;
  std::function<void(int, float)> parameterChangedCallback;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterArrayAttachment)
};

} // namespace Uhhyou
