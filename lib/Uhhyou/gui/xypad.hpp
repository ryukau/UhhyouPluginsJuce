// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "numbereditor.hpp"
#include "parameterarrayattachment.hpp"
#include "parameterlock.hpp"
#include "style.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <ranges>
#include <vector>

namespace Uhhyou {

class XYPad : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)

  constexpr static int8_t nGrid = 8;
  constexpr static float wheelSensitivity = float(0.0001);

  juce::AudioProcessorEditor &editor;
  std::array<juce::RangedAudioParameter *, 2> parameter;
  Palette &pal;
  StatusBar &statusBar;
  ParameterArrayAttachment<2> attachment;

  std::array<float, 2> value{float(0), float(0)};
  juce::Point<float> mousePosition{float(-1), float(-1)};
  bool isMouseEntered = false;
  bool isEditing = false;

  std::array<std::vector<float>, 2> snaps;
  std::array<bool, 2> isSnapping{false, false};
  std::array<float, 2> snapDiffAccumulator{float(0), float(0)};
  juce::Point<float> lastMousePos;
  float snapDistancePixel = float(24);
  bool snappingEnabled = true;

  enum class AxisLock { none, x, y };

  AxisLock getLockState(const juce::ModifierKeys &mods)
  {
    if (mods.isMiddleButtonDown()) return mods.isShiftDown() ? AxisLock::x : AxisLock::y;
    return AxisLock::none;
  }

  void updateStatusBar()
  {
    juce::String text{"("};
    text += parameter[0]->getName(64);
    text += ", ";
    text += parameter[1]->getName(64);
    text += ") = (";
    text += parameter[0]->getText(parameter[0]->getValue(), 5);
    text += ", ";
    text += parameter[1]->getText(parameter[1]->getValue(), 5);
    text += ")";
    statusBar.update(text);
  }

  void updateValueFromPos(const juce::Point<float> &pos, AxisLock lock = AxisLock::none)
  {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    if (w <= 0 || h <= 0) return;

    bool isUpdated{false};

    if (lock != AxisLock::x) {
      float normX = std::clamp(pos.x, float(0), w) / w;
      if (value[0] != normX) {
        value[0] = normX;
        attachment.setValueAsPartOfGesture(0, parameter[0]->convertFrom0to1(value[0]));
        isUpdated = true;
      }
    }

    if (lock != AxisLock::y) {
      float normY = std::clamp(h - pos.y, float(0), h) / h;
      if (value[1] != normY) {
        value[1] = normY;
        attachment.setValueAsPartOfGesture(1, parameter[1]->convertFrom0to1(value[1]));
        isUpdated = true;
      }
    }

    if (isUpdated) updateStatusBar();
  }

  void moveValueWithSnap(int axis, float deltaPixel, float rangePixels)
  {
    if (rangePixels <= 0) return;

    if (snappingEnabled && isSnapping[axis]) {
      snapDiffAccumulator[axis] += deltaPixel;
      if (std::abs(snapDiffAccumulator[axis]) > snapDistancePixel) {
        isSnapping[axis] = false;
        snapDiffAccumulator[axis] = float(0);
      }
      return;
    }

    float currentVal = value[axis];
    float deltaNorm = deltaPixel / rangePixels;
    float nextVal = currentVal + deltaNorm;

    auto findSnapPoint = [&]() -> std::optional<float>
    {
      if (!snappingEnabled || snaps[axis].empty() || deltaNorm == float(0)) {
        return std::nullopt;
      }

      const auto &axisSnaps = snaps[axis];
      if (deltaNorm > float(0)) {
        // Moving Positive
        auto it = std::ranges::upper_bound(axisSnaps, currentVal);
        if (it != axisSnaps.end() && nextVal >= *it) return *it;
      } else {
        // Moving Negative
        auto it = std::ranges::lower_bound(axisSnaps, currentVal);
        if (it != axisSnaps.begin()) {
          if (float prev = *std::prev(it); nextVal <= prev) return prev;
        }
      }
      return std::nullopt;
    };

    if (auto snapTarget = findSnapPoint()) {
      value[axis] = *snapTarget;
      isSnapping[axis] = true;
      snapDiffAccumulator[axis] = float(0);
    } else {
      value[axis] = std::clamp(nextVal, float(0), float(1));
    }
  }

  void resetValue(AxisLock lock = AxisLock::none)
  {
    isSnapping = {false, false};

    for (int i = 0; i < 2; ++i) {
      if ((i == 0 && lock == AxisLock::x) || (i == 1 && lock == AxisLock::y)) continue;

      float defaultNorm = parameter[i]->getDefaultValue();
      value[i] = defaultNorm;
      attachment.setValueAsCompleteGesture(i, parameter[i]->convertFrom0to1(defaultNorm));
    }
    updateStatusBar();
    repaint();
  }

  void cycleUpValue(size_t index)
  {
    auto &v = value[index];

    if (v >= float(1)) {
      v = float(0);
      return;
    }

    const auto &snp = snaps[index];
    auto it = std::ranges::upper_bound(snp, v);
    v = (it != snp.end()) ? *it : float(1);
  }

  void cycleDownValue(size_t index)
  {
    auto &v = value[index];

    if (v <= 0) {
      v = float(1);
      return;
    }

    const auto &snp = snaps[index];
    auto it = std::ranges::lower_bound(snp, v);
    v = (it != snp.begin()) ? *std::prev(it) : float(0);
  }

public:
  XYPad(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    std::array<juce::RangedAudioParameter *, 2> parameters,
    StatusBar &statusBar)
    : editor(editor)
    , parameter(parameters)
    , pal(palette)
    , statusBar(statusBar)
    , attachment(
        parameters,
        [&](int index, float rawValue)
        {
          if (index < 0 || index >= 2) return;
          float normalized = parameter[index]->convertTo0to1(rawValue);
          if (value[index] != normalized) {
            value[index] = normalized;
            repaint();
          }
        },
        undoManager)
  {
    editor.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setWantsKeyboardFocus(true);

    for (size_t i = 0; i < 2; ++i) {
      if (!parameter[i]) continue;
      value[i] = parameter[i]->convertTo0to1(parameter[i]->getValue());
    }
    attachment.sendInitialUpdate();
  }

  void setSnappingEnabled(bool shouldSnap) { snappingEnabled = shouldSnap; }

  void setSnaps(const std::vector<float> &snapsX_, const std::vector<float> &snapsY_)
  {
    snaps[0] = snapsX_;
    if (!snaps[0].empty()) std::sort(snaps[0].begin(), snaps[0].end());

    snaps[1] = snapsY_;
    if (!snaps[1].empty()) std::sort(snaps[1].begin(), snaps[1].end());
  }

  void setSnapDistance(float pixels) { snapDistancePixel = pixels; }

  void paint(juce::Graphics &ctx) override
  {
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());

    // Background.
    ctx.setColour(pal.boxBackground());
    ctx.fillAll();

    // Grid.
    const float dotRadius = 2 * pal.borderThin();
    ctx.setColour(pal.foregroundInactive());
    for (size_t ix = 1; ix < nGrid; ++ix) {
      for (size_t iy = 1; iy < nGrid; ++iy) {
        auto cx = std::floor(ix * width / nGrid);
        auto cy = std::floor(iy * height / nGrid);
        ctx.fillEllipse(cx - dotRadius, cy - dotRadius, dotRadius * 2, dotRadius * 2);
      }
    }

    // Mouse Cursor Crosshair.
    if (isMouseEntered) {
      ctx.setColour(pal.highlightMain());
      ctx.drawLine(float(0), mousePosition.y, width, mousePosition.y, pal.borderThin());
      ctx.drawLine(mousePosition.x, float(0), mousePosition.x, height, pal.borderThin());
    }

    // Value Indicator.
    auto valueX = std::floor(value[0] * width);
    auto valueY = std::floor((float(1) - value[1]) * height);
    const float valR = 8 * pal.borderThin();
    ctx.setColour(pal.foreground());

    ctx.drawEllipse(valueX - valR, valueY - valR, valR * 2, valR * 2, float(2));

    ctx.drawLine(float(0), valueY, width, valueY, pal.borderThin());
    ctx.drawLine(valueX, float(0), valueX, height, pal.borderThin());

    // Border.
    ctx.setColour((isMouseEntered || isEditing) ? pal.highlightMain() : pal.border());
    ctx.drawRect(float(0), float(0), width, height, pal.borderThin());
  }

  void mouseEnter(const juce::MouseEvent &event) override
  {
    isMouseEntered = true;
    mousePosition = event.position;
    repaint();
  }

  void mouseExit(const juce::MouseEvent &event) override
  {
    isMouseEntered = false;
    mousePosition = event.position;
    repaint();
  }

  void mouseMove(const juce::MouseEvent &event) override
  {
    if (!isMouseEntered) return;
    mousePosition = event.position;
    repaint();
  }

  void mouseDown(const juce::MouseEvent &event) override
  {
    mousePosition = event.position;

    if (event.mods.isRightButtonDown()) {
      auto hostContext = editor.getHostContext();
      if (hostContext == nullptr) return;

      size_t index = event.position.x < getWidth() / float(2) ? 0 : 1;

      auto hostContextMenu = hostContext->getContextMenuForParameter(parameter[index]);
      if (hostContextMenu != nullptr) {
        hostContextMenu->showNativeMenu(event.getScreenPosition());
      }
      return;
    }

    if (event.mods.isCommandDown()) {
      resetValue(getLockState(event.mods));
    } else {
      isEditing = true;
      isSnapping = {false, false};
      snapDiffAccumulator = {float(0), float(0)};
      updateValueFromPos(mousePosition, getLockState(event.mods));
      event.source.enableUnboundedMouseMovement(true);
      lastMousePos = event.position;
      attachment.beginGesture(0);
      attachment.beginGesture(1);
    }

    repaint();
  }

  void mouseDrag(const juce::MouseEvent &event) override
  {
    if (isEditing && !event.mods.isRightButtonDown()) {
      float deltaX = event.position.x - lastMousePos.x;
      float deltaY = event.position.y - lastMousePos.y;
      lastMousePos = event.position;

      AxisLock lock = getLockState(event.mods);
      bool changed = false;

      if (lock != AxisLock::x) {
        float oldX = value[0];
        moveValueWithSnap(0, deltaX, float(getWidth()));
        if (value[0] != oldX) {
          attachment.setValueAsPartOfGesture(0, parameter[0]->convertFrom0to1(value[0]));
          changed = true;
        }
      }

      if (lock != AxisLock::y) {
        float oldY = value[1];
        moveValueWithSnap(1, -deltaY, float(getHeight()));
        if (value[1] != oldY) {
          attachment.setValueAsPartOfGesture(1, parameter[1]->convertFrom0to1(value[1]));
          changed = true;
        }
      }

      mousePosition = {value[0] * getWidth(), (float(1) - value[1]) * getHeight()};

      if (changed) updateStatusBar();
      repaint();
    }
  }

  void mouseUp(const juce::MouseEvent &event) override
  {
    if (!isEditing) return;

    event.source.enableUnboundedMouseMovement(false);
    mousePosition = {value[0] * getWidth(), (float(1) - value[1]) * getHeight()};

    attachment.endGesture(0);
    attachment.endGesture(1);
    isEditing = false;
    isSnapping = {false, false};

    repaint();
  }

  void mouseWheelMove(
    const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override
  {
    if (wheel.deltaY == 0) return;

    int index = event.mods.isShiftDown() ? 1 : 0;

    float updated = value[index] + float(wheel.deltaY * wheelSensitivity);
    updated = std::clamp(updated, float(0), float(1));
    if (value[index] == updated) return;

    value[index] = updated;
    attachment.setValueAsCompleteGesture(
      index, parameter[index]->convertFrom0to1(value[index]));

    updateStatusBar();
    repaint();
  }

  bool keyPressed(const juce::KeyPress &key) override
  {
    using KP = juce::KeyPress;
    const auto &mods = key.getModifiers();

    auto updateValue = [&](int index)
    {
      attachment.setValueAsCompleteGesture(
        index, parameter[index]->convertFrom0to1(value[index]));
      updateStatusBar();
      repaint();
      return true;
    };

    auto increaseValue = [&](int index, float delta)
    { value[index] = std::clamp(value[index] + delta, float(0), float(1)); };

    auto handleMove = [&](
                        int index, const int increamentKey, const int decrementKey,
                        const int cycleUpKey, const int cycleDownKey)
    {
      constexpr float sensitivity = float(0.0078125);

      if (
        key.isKeyCode(cycleUpKey)
        || (key.isKeyCode(increamentKey) && mods.isCommandDown()))
      {
        cycleUpValue(index);
        return updateValue(index);
      }
      if (
        key.isKeyCode(cycleDownKey)
        || (key.isKeyCode(decrementKey) && mods.isCommandDown()))
      {
        cycleDownValue(index);
        return updateValue(index);
      }
      if (key.isKeyCode(increamentKey)) {
        increaseValue(index, sensitivity);
        return updateValue(index);
      }
      if (key.isKeyCode(decrementKey)) {
        increaseValue(index, -sensitivity);
        return updateValue(index);
      }

      return false;
    };

    // X.
    if (handleMove(0, KP::rightKey, KP::leftKey, KP::pageUpKey, KP::pageDownKey)) {
      return true;
    }

    // Y
    if (handleMove(1, KP::upKey, KP::downKey, KP::homeKey, KP::endKey)) {
      return true;
    }

    return false;
  }
};

class LabeledXYPad : public juce::Component {
private:
  class SnapToggle : public juce::Component {
  private:
    Palette &pal;
    StatusBar &statusBar;
    bool state = true;
    std::function<void(bool)> onStateChange = nullptr;

  public:
    SnapToggle(
      Palette &palette, StatusBar &s, bool initialState, decltype(onStateChange) onChange)
      : pal(palette), statusBar(s), state(initialState), onStateChange(onChange)
    {
      setWantsKeyboardFocus(true);
    }

    void paint(juce::Graphics &ctx) override
    {
      auto bounds = getLocalBounds().toFloat();
      ctx.setColour(pal.border());
      ctx.drawRect(bounds, pal.borderThin());

      if (state) {
        ctx.setColour(pal.highlightButton());
        ctx.fillRect(bounds.reduced(4 * pal.borderThin()));
      }

      if (hasKeyboardFocus(false)) {
        ctx.setColour(pal.highlightMain().withAlpha(0.5f));
        ctx.drawRect(bounds, 2.0f);
      }
    }

    void mouseEnter(const juce::MouseEvent &) override
    {
      setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent &) override
    {
      setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent &) override { toggle(); }

    bool keyPressed(const juce::KeyPress &key) override
    {
      using KP = juce::KeyPress;
      if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
        toggle();
        return true;
      }
      return false;
    }

    void focusGained(juce::Component::FocusChangeType) override { repaint(); }
    void focusLost(juce::Component::FocusChangeType) override { repaint(); }

    void toggle()
    {
      state = !state;
      if (onStateChange) {
        onStateChange(state);
        statusBar.update(state ? "XYPad Snap: ON" : "XYPad Snap OFF");
      }
      repaint();
    }
  };

  Palette &pal;
  XYPad &pad;
  SnapToggle toggle;

  LockableLabel labelXComp;
  LockableLabel labelYComp;

public:
  LabeledXYPad(
    ParameterLockRegistry &locks,
    Palette &palette,
    StatusBar &statusBar,
    XYPad &pad,
    const juce::String &labelX,
    const juce::String &labelY,
    const juce::AudioProcessorParameter *const parameterX,
    const juce::AudioProcessorParameter *const parameterY,
    bool initialSnapState = true,
    std::function<void(bool)> onSnapChange = nullptr)
    : pal(palette)
    , pad(pad)
    , toggle(
        palette,
        statusBar,
        initialSnapState,
        [this, onSnapChange](bool state)
        {
          this->pad.setSnappingEnabled(state);
          if (onSnapChange) onSnapChange(state);
        })
    , labelXComp(
        locks,
        palette,
        labelX,
        parameterX,
        juce::Justification::centred,
        LockableLabel::Orientation::horizontal)
    , labelYComp(
        locks,
        palette,
        labelY,
        parameterY,
        juce::Justification::centred,
        LockableLabel::Orientation::vertical)
  {
    pad.setSnappingEnabled(initialSnapState);

    addAndMakeVisible(pad);
    addAndMakeVisible(toggle);
    addAndMakeVisible(labelXComp);
    addAndMakeVisible(labelYComp);
  }

  void resized() override
  {
    int stripSize
      = static_cast<int>(pal.getFont(pal.textSizeUi()).getHeight() * float(1.5));
    auto r = getLocalBounds();
    toggle.setBounds(0, 0, stripSize, stripSize);
    labelXComp.setBounds(stripSize, 0, r.getWidth() - stripSize, stripSize);
    labelYComp.setBounds(0, stripSize, stripSize, r.getHeight() - stripSize);
    pad.setBounds(
      stripSize, stripSize, r.getWidth() - stripSize, r.getHeight() - stripSize);
  }

  juce::Component &getToggle() { return toggle; }
  juce::Component &getLabelX() { return labelXComp; }
  juce::Component &getLabelY() { return labelYComp; }
};

} // namespace Uhhyou
