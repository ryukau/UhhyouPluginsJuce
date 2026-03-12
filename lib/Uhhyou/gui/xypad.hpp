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
#include <format>
#include <optional>
#include <ranges>
#include <vector>

namespace Uhhyou {

class XYPad : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)

  class XYPadValueInterface : public juce::AccessibilityValueInterface {
  public:
    explicit XYPadValueInterface(XYPad& padToWrap) : pad_(padToWrap) {}

    bool isReadOnly() const override { return false; }

    double getCurrentValue() const override {
      double offset = pad_.lastActiveAxis_ == 1 ? 0.000001 : 0.0;
      return double(pad_.value_[pad_.lastActiveAxis_]) + offset;
    }

    void setValue(double newValue) override {
      pad_.attachment_.setValueAsCompleteGesture(
        static_cast<int>(pad_.lastActiveAxis_),
        pad_.parameter_[pad_.lastActiveAxis_]->convertFrom0to1(float(newValue)));
    }

    juce::String getCurrentValueAsString() const override {
      return std::format("X: {} %, Y: {} %",
                         std::format("{:.{}f}", 100.0f * pad_.parameter_[0]->getValue(), 1),
                         std::format("{:.{}f}", 100.0f * pad_.parameter_[1]->getValue(), 1));
    }

    void setValueAsString(const juce::String&) override {}

    AccessibleValueRange getRange() const override { return {{0.0, 1.0}, 0.0}; }

  private:
    XYPad& pad_;
  };

  class XYPadAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    explicit XYPadAccessibilityHandler(XYPad& xypad, juce::AccessibilityActions a)
        : juce::AccessibilityHandler(xypad, juce::AccessibilityRole::slider, std::move(a),
                                     Interfaces{std::make_unique<XYPadValueInterface>(xypad)}),
          pad_(xypad) {}

    juce::String getTitle() const override {
      return std::format("XY Pad, {} versus {}", pad_.parameter_[0]->getName(64).toRawUTF8(),
                         pad_.parameter_[1]->getName(64).toRawUTF8());
    }

    juce::String getHelp() const override {
      return "Use Left and Right arrow keys to adjust the X axis. Use Up and Down arrow keys to "
             "adjust the Y axis.";
    }

  private:
    XYPad& pad_;
  };

  constexpr static int8_t nGrid = 8;
  constexpr static float wheelSensitivity = float(0.0001);

  juce::AudioProcessorEditor& editor_;
  std::array<juce::RangedAudioParameter*, 2> parameter_;
  Palette& pal_;
  StatusBar& statusBar_;
  ParameterArrayAttachment<2> attachment_;

  std::array<float, 2> value_{float(0), float(0)};
  juce::Point<float> mousePosition_{float(-1), float(-1)};
  bool isMouseEntered_ = false;
  bool isEditing_ = false;
  size_t lastActiveAxis_ = 0; // Tracks whether X (0) or Y (1) was last updated

  std::array<std::vector<float>, 2> snaps_;
  std::array<bool, 2> isSnapping_{false, false};
  std::array<float, 2> snapDiffAccumulator_{float(0), float(0)};
  juce::Point<float> lastMousePos_;
  float snapDistancePixel_ = float(24);
  bool snappingEnabled_ = true;

  enum class AxisLock { none, x, y };

  AxisLock getLockState(const juce::ModifierKeys& mods) {
    if (mods.isMiddleButtonDown()) { return mods.isShiftDown() ? AxisLock::x : AxisLock::y; }
    return AxisLock::none;
  }

  void showHostMenuNative(size_t index, juce::Point<int> position) {
    if (auto* hostContext = editor_.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter_[index])) {
        hostContextMenu->showNativeMenu(position);
      }
    }
  }

  void showHostMenuJuce(size_t index) {
    if (auto* hostContext = editor_.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter_[index])) {
        hostContextMenu->getEquivalentPopupMenu().showMenuAsync(
          juce::PopupMenu::Options().withTargetComponent(this));
      }
    }
  }

  void updateStatusBar() {
    statusBar_.update(
      std::format("X: {}, Y: {} = {}, {}", parameter_[0]->getName(64).toRawUTF8(),
                  parameter_[1]->getName(64).toRawUTF8(),
                  parameter_[0]->getText(parameter_[0]->getValue(), 5).toRawUTF8(),
                  parameter_[1]->getText(parameter_[1]->getValue(), 5).toRawUTF8()));
  }

  void setInternalValue(size_t index, float newValue) {
    if (value_[index] == newValue) { return; }
    value_[index] = newValue;
    lastActiveAxis_ = index;
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
    }
  }

  void updateValueFromPos(const juce::Point<float>& pos, AxisLock lock = AxisLock::none) {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    if (w <= 0 || h <= 0) { return; }

    bool isUpdated{false};

    if (lock != AxisLock::x) {
      float normX = std::clamp(pos.x, float(0), w) / w;
      if (value_[0] != normX) {
        setInternalValue(0, normX);
        attachment_.setValueAsPartOfGesture(0, parameter_[0]->convertFrom0to1(value_[0]));
        isUpdated = true;
      }
    }

    if (lock != AxisLock::y) {
      float normY = std::clamp(h - pos.y, float(0), h) / h;
      if (value_[1] != normY) {
        setInternalValue(1, normY);
        attachment_.setValueAsPartOfGesture(1, parameter_[1]->convertFrom0to1(value_[1]));
        isUpdated = true;
      }
    }

    if (isUpdated) { updateStatusBar(); }
  }

  void moveValueWithSnap(size_t axis, float deltaPixel, float rangePixels) {
    if (rangePixels <= 0) { return; }

    if (snappingEnabled_ && isSnapping_[axis]) {
      snapDiffAccumulator_[axis] += deltaPixel;
      if (std::abs(snapDiffAccumulator_[axis]) > snapDistancePixel_) {
        isSnapping_[axis] = false;
        snapDiffAccumulator_[axis] = float(0);
      }
      return;
    }

    float currentVal = value_[axis];
    float deltaNorm = deltaPixel / rangePixels;
    float nextVal = currentVal + deltaNorm;

    auto findSnapPoint = [&]() -> std::optional<float> {
      if (!snappingEnabled_ || snaps_[axis].empty() || deltaNorm == float(0)) {
        return std::nullopt;
      }

      const auto& axisSnaps = snaps_[axis];
      if (deltaNorm > float(0)) {
        auto it = std::ranges::upper_bound(axisSnaps, currentVal);
        if (it != axisSnaps.end() && nextVal >= *it) { return *it; }
      } else {
        auto it = std::ranges::lower_bound(axisSnaps, currentVal);
        if (it != axisSnaps.begin()) {
          if (float prev = *std::prev(it); nextVal <= prev) { return prev; }
        }
      }
      return std::nullopt;
    };

    if (auto snapTarget = findSnapPoint()) {
      setInternalValue(axis, *snapTarget);
      isSnapping_[axis] = true;
      snapDiffAccumulator_[axis] = float(0);
    } else {
      setInternalValue(axis, std::clamp(nextVal, float(0), float(1)));
    }
  }

  void resetValue(AxisLock lock = AxisLock::none) {
    isSnapping_ = {false, false};

    for (size_t i = 0; i < 2; ++i) {
      if ((i == 0 && lock == AxisLock::x) || (i == 1 && lock == AxisLock::y)) { continue; }

      float defaultNorm = parameter_[i]->getDefaultValue();
      setInternalValue(i, defaultNorm);
      attachment_.setValueAsCompleteGesture(static_cast<int>(i),
                                            parameter_[i]->convertFrom0to1(defaultNorm));
    }
    updateStatusBar();
  }

  void cycleUpValue(size_t index) {
    float v = value_[index];
    if (v >= float(1)) {
      setInternalValue(index, float(0));
      return;
    }

    const auto& snp = snaps_[index];
    auto it = std::ranges::upper_bound(snp, v);
    setInternalValue(index, (it != snp.end()) ? *it : float(1));
  }

  void cycleDownValue(size_t index) {
    float v = value_[index];
    if (v <= 0) {
      setInternalValue(index, float(1));
      return;
    }

    const auto& snp = snaps_[index];
    auto it = std::ranges::lower_bound(snp, v);
    setInternalValue(index, (it != snp.begin()) ? *std::prev(it) : float(0));
  }

public:
  XYPad(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
        std::array<juce::RangedAudioParameter*, 2> parameters, StatusBar& statusBar)
      : editor_(editor), parameter_(parameters), pal_(palette), statusBar_(statusBar),
        attachment_(
          parameters,
          [&](int index, float rawValue) {
            if (index < 0 || index >= 2) { return; }
            size_t idx = static_cast<size_t>(index);
            float normalized = parameter_[idx]->convertTo0to1(rawValue);
            setInternalValue(idx, normalized);
          },
          undoManager) {
    editor_.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    for (size_t i = 0; i < 2; ++i) {
      if (!parameter_[i]) { continue; }
      value_[i] = parameter_[i]->convertTo0to1(parameter_[i]->getValue());
    }
    attachment_.sendInitialUpdate();
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::showMenu, [this]() {
      juce::PopupMenu menu;
      menu.addItem(1, "X: " + parameter_[0]->getName(64));
      menu.addItem(2, "Y: " + parameter_[1]->getName(64));
      menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this](int result) {
        if (result == 1) {
          showHostMenuJuce(0);
        } else if (result == 2) {
          showHostMenuJuce(1);
        }
      });
    });
    return std::make_unique<XYPadAccessibilityHandler>(*this, std::move(actions));
  }

  void setSnappingEnabled(bool shouldSnap) { snappingEnabled_ = shouldSnap; }

  void setSnaps(const std::vector<float>& snapsX, const std::vector<float>& snapsY) {
    snaps_[0] = snapsX;
    if (!snaps_[0].empty()) { std::sort(snaps_[0].begin(), snaps_[0].end()); }

    snaps_[1] = snapsY;
    if (!snaps_[1].empty()) { std::sort(snaps_[1].begin(), snaps_[1].end()); }
  }

  void setSnapDistance(float pixels) { snapDistancePixel_ = pixels; }

  void paint(juce::Graphics& ctx) override {
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());

    // Background.
    ctx.setColour(pal_.surface());
    ctx.fillAll();

    // Grid.
    const float dotRadius = 2 * pal_.borderWidth();
    ctx.setColour(pal_.foreground().withAlpha(0.5f));
    for (size_t ix = 1; ix < nGrid; ++ix) {
      for (size_t iy = 1; iy < nGrid; ++iy) {
        auto cx = std::floor(static_cast<float>(ix) * width / nGrid);
        auto cy = std::floor(static_cast<float>(iy) * height / nGrid);
        ctx.fillEllipse(cx - dotRadius, cy - dotRadius, dotRadius * 2, dotRadius * 2);
      }
    }

    // Mouse Cursor Crosshair.
    if (isMouseEntered_) {
      ctx.setColour(pal_.main());
      ctx.drawLine(float(0), mousePosition_.y, width, mousePosition_.y, pal_.borderWidth());
      ctx.drawLine(mousePosition_.x, float(0), mousePosition_.x, height, pal_.borderWidth());
    }

    // Value Indicator.
    auto valueX = std::floor(value_[0] * width);
    auto valueY = std::floor((float(1) - value_[1]) * height);
    const float valR = 8 * pal_.borderWidth();
    ctx.setColour(pal_.foreground());

    ctx.drawEllipse(valueX - valR, valueY - valR, valR * 2, valR * 2, float(2));

    ctx.drawLine(float(0), valueY, width, valueY, pal_.borderWidth());
    ctx.drawLine(valueX, float(0), valueX, height, pal_.borderWidth());

    // Border.
    ctx.setColour((isMouseEntered_ || isEditing_) ? pal_.main() : pal_.border());
    ctx.drawRect(float(0), float(0), width, height, pal_.borderWidth());
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    isMouseEntered_ = true;
    mousePosition_ = event.position;
    updateStatusBar();
    repaint();
  }

  void mouseExit(const juce::MouseEvent& event) override {
    isMouseEntered_ = false;
    mousePosition_ = event.position;
    statusBar_.clear();
    repaint();
  }

  void mouseMove(const juce::MouseEvent& event) override {
    if (!isMouseEntered_) { return; }
    mousePosition_ = event.position;
    repaint();
  }

  void mouseDown(const juce::MouseEvent& event) override {
    mousePosition_ = event.position;

    if (event.mods.isRightButtonDown()) {
      size_t index = event.position.x < getWidth() / float(2) ? 0 : 1;
      showHostMenuNative(index, editor_.getMouseXYRelative());
      return;
    }

    if (event.mods.isCommandDown()) {
      resetValue(getLockState(event.mods));
    } else {
      isEditing_ = true;
      isSnapping_ = {false, false};
      snapDiffAccumulator_ = {float(0), float(0)};
      updateValueFromPos(mousePosition_, getLockState(event.mods));
      event.source.enableUnboundedMouseMovement(true);
      lastMousePos_ = event.position;
      attachment_.beginGesture(0);
      attachment_.beginGesture(1);
    }

    repaint();
  }

  void mouseDrag(const juce::MouseEvent& event) override {
    if (isEditing_ && !event.mods.isRightButtonDown()) {
      float deltaX = event.position.x - lastMousePos_.x;
      float deltaY = event.position.y - lastMousePos_.y;
      lastMousePos_ = event.position;

      AxisLock lock = getLockState(event.mods);
      bool changed = false;

      if (lock != AxisLock::x) {
        float oldX = value_[0];
        moveValueWithSnap(0, deltaX, float(getWidth()));
        if (value_[0] != oldX) {
          attachment_.setValueAsPartOfGesture(0, parameter_[0]->convertFrom0to1(value_[0]));
          changed = true;
        }
      }

      if (lock != AxisLock::y) {
        float oldY = value_[1];
        moveValueWithSnap(1, -deltaY, float(getHeight()));
        if (value_[1] != oldY) {
          attachment_.setValueAsPartOfGesture(1, parameter_[1]->convertFrom0to1(value_[1]));
          changed = true;
        }
      }

      mousePosition_ = {value_[0] * getWidth(), (float(1) - value_[1]) * getHeight()};

      if (changed) { updateStatusBar(); }
      repaint();
    }
  }

  void mouseUp(const juce::MouseEvent& event) override {
    if (!isEditing_) { return; }

    event.source.enableUnboundedMouseMovement(false);
    mousePosition_ = {value_[0] * getWidth(), (float(1) - value_[1]) * getHeight()};

    attachment_.endGesture(0);
    attachment_.endGesture(1);
    isEditing_ = false;
    isSnapping_ = {false, false};

    repaint();
  }

  void mouseWheelMove(const juce::MouseEvent& event,
                      const juce::MouseWheelDetails& wheel) override {
    if (std::abs(wheel.deltaY) <= std::numeric_limits<float>::epsilon()) { return; }

    size_t index = event.mods.isShiftDown() ? 1 : 0;

    float updated = value_[index] + float(wheel.deltaY * wheelSensitivity);
    updated = std::clamp(updated, float(0), float(1));
    if (value_[index] == updated) { return; }

    setInternalValue(index, updated);
    attachment_.setValueAsCompleteGesture(static_cast<int>(index),
                                          parameter_[index]->convertFrom0to1(value_[index]));

    updateStatusBar();
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    const auto& mods = key.getModifiers();

    if (key.isKeyCode(KP::F10Key) && mods.isShiftDown()) {
      if (mods.isCommandDown()) {
        showHostMenuJuce(1);
      } else {
        showHostMenuJuce(0);
      }
      return true;
    }

    if (key.isKeyCode(KP::backspaceKey) || key.isKeyCode(KP::deleteKey)) {
      if (mods.isCommandDown()) {
        resetValue(AxisLock::none);
        return true;
      }
    }

    auto updateValue = [&](size_t index) {
      attachment_.setValueAsCompleteGesture(static_cast<int>(index),
                                            parameter_[index]->convertFrom0to1(value_[index]));
      updateStatusBar();
      return true;
    };

    auto increaseValue = [&](size_t index, float delta) {
      setInternalValue(index, std::clamp(value_[index] + delta, float(0), float(1)));
    };

    auto handleMove = [&](size_t index, const int increamentKey, const int decrementKey,
                          const int cycleUpKey, const int cycleDownKey) {
      const float sensitivity = mods.isShiftDown() ? float(0.0078125 / 5) : float(0.0078125);

      if (key.isKeyCode(cycleUpKey) || (key.isKeyCode(increamentKey) && mods.isCommandDown())) {
        cycleUpValue(index);
        return updateValue(index);
      }
      if (key.isKeyCode(cycleDownKey) || (key.isKeyCode(decrementKey) && mods.isCommandDown())) {
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

    // 0: X, 1: Y.
    if (handleMove(0, KP::rightKey, KP::leftKey, KP::pageUpKey, KP::pageDownKey)) { return true; }
    if (handleMove(1, KP::upKey, KP::downKey, KP::homeKey, KP::endKey)) { return true; }

    return false;
  }
};

class LabeledXYPad : public juce::Component {
private:
  class SnapToggle : public juce::Component {
  private:
    class SnapToggleAccessibilityHandler : public juce::AccessibilityHandler {
    public:
      SnapToggleAccessibilityHandler(SnapToggle& st, juce::AccessibilityActions act)
          : juce::AccessibilityHandler(st, juce::AccessibilityRole::toggleButton, std::move(act)),
            toggle_(st) {}

      juce::AccessibleState getCurrentState() const override {
        auto accessibleState = juce::AccessibilityHandler::getCurrentState().withCheckable();
        if (toggle_.getState()) { accessibleState = accessibleState.withChecked(); }
        return accessibleState;
      }

      juce::String getTitle() const override {
        return "XY Pad Snap " + juce::String(toggle_.getState() ? "On" : "Off");
      }

    private:
      SnapToggle& toggle_;
    };

    Palette& pal_;
    StatusBar& statusBar_;
    bool state_ = true;
    std::function<void(bool)> onStateChange_ = nullptr;

    void updateStatusBar() { statusBar_.update(state_ ? "XY Pad: Snap ON" : "XY Pad: Snap OFF"); }

  public:
    SnapToggle(Palette& palette, StatusBar& s, bool initialState, decltype(onStateChange_) onChange)
        : pal_(palette), statusBar_(s), state_(initialState), onStateChange_(onChange) {
      setWantsKeyboardFocus(true);
      setMouseClickGrabsKeyboardFocus(true);
      setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
      juce::AccessibilityActions actions;
      actions.addAction(juce::AccessibilityActionType::toggle, [this]() { toggle(); });
      return std::make_unique<SnapToggleAccessibilityHandler>(*this, std::move(actions));
    }

    bool getState() const { return state_; }

    void paint(juce::Graphics& ctx) override {
      auto bounds = getLocalBounds().toFloat();
      ctx.setColour(pal_.border());
      ctx.drawRect(bounds, pal_.borderWidth());

      if (state_) {
        ctx.setColour(pal_.main());
        ctx.fillRect(bounds.reduced(4 * pal_.borderWidth()));
      }

      if (hasKeyboardFocus(false)) {
        ctx.setColour(pal_.main().withAlpha(0.5f));
        ctx.drawRect(bounds, 2.0f);
      }
    }

    void mouseEnter(const juce::MouseEvent&) override { updateStatusBar(); }
    void mouseExit(const juce::MouseEvent&) override { statusBar_.clear(); }

    void mouseDown(const juce::MouseEvent&) override { toggle(); }

    bool keyPressed(const juce::KeyPress& key) override {
      using KP = juce::KeyPress;
      if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
        toggle();
        return true;
      }
      return false;
    }

    void focusGained(juce::Component::FocusChangeType) override { repaint(); }
    void focusLost(juce::Component::FocusChangeType) override { repaint(); }

    void toggle() {
      state_ = !state_;
      if (onStateChange_) {
        onStateChange_(state_);
        updateStatusBar();
      }
      repaint();

      if (auto* handler = getAccessibilityHandler()) {
        handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
      }
    }
  };

  Palette& pal_;
  XYPad& pad_;
  SnapToggle toggle_;

  LockableLabel labelXComp_;
  LockableLabel labelYComp_;

public:
  LabeledXYPad(ParameterLockRegistry& locks, Palette& palette, StatusBar& statusBar, XYPad& pad,
               const juce::String& labelX, const juce::String& labelY,
               const juce::RangedAudioParameter* const parameterX,
               const juce::RangedAudioParameter* const parameterY, bool initialSnapState = true,
               std::function<void(bool)> onSnapChange = nullptr)
      : pal_(palette), pad_(pad), toggle_(palette, statusBar, initialSnapState,
                                          [this, onSnapChange](bool state) {
                                            pad_.setSnappingEnabled(state);
                                            if (onSnapChange) { onSnapChange(state); }
                                          }),
        labelXComp_(locks, palette, statusBar, labelX, parameterX, juce::Justification::centred,
                    LockableLabel::Orientation::horizontal),
        labelYComp_(locks, palette, statusBar, labelY, parameterY, juce::Justification::centred,
                    LockableLabel::Orientation::vertical) {
    pad_.setSnappingEnabled(initialSnapState);

    addAndMakeVisible(pad_);
    addAndMakeVisible(toggle_);
    addAndMakeVisible(labelXComp_);
    addAndMakeVisible(labelYComp_);
  }

  void resized() override {
    int stripSize = static_cast<int>(pal_.getFontHeight(TextSize::normal) * float(1.5));
    auto r = getLocalBounds();
    toggle_.setBounds(0, 0, stripSize, stripSize);
    labelXComp_.setBounds(stripSize, 0, r.getWidth() - stripSize, stripSize);
    labelYComp_.setBounds(0, stripSize, stripSize, r.getHeight() - stripSize);
    pad_.setBounds(stripSize, stripSize, r.getWidth() - stripSize, r.getHeight() - stripSize);
  }

  juce::Component& getToggle() { return toggle_; }
  juce::Component& getLabelX() { return labelXComp_; }
  juce::Component& getLabelY() { return labelYComp_; }
};

} // namespace Uhhyou
