#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "numbereditor.hpp"
#include "parameterarrayattachment.hpp"
#include "style.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace Uhhyou {

class XYPad : public juce::Component {
private:
  constexpr static int8_t nGrid = 8;
  constexpr static float borderWidth = 2.0f;
  constexpr static float wheelSensitivity = 0.0001f;

  juce::AudioProcessorEditor &editor;
  std::array<juce::RangedAudioParameter *, 2> parameter;
  Palette &pal;
  StatusBar &statusBar;
  ParameterArrayAttachment<2> attachment;

  std::array<float, 2> value{0.0f, 0.0f}; // Internal normalized values (0.0 - 1.0)
  juce::Point<float> mousePosition{-1.0f, -1.0f};
  bool isMouseEntered = false;
  bool isEditing = false;

  std::array<std::vector<float>, 2> snaps;
  std::array<bool, 2> isSnapping{false, false};
  std::array<float, 2> snapDiffAccumulator{0.0f, 0.0f};
  juce::Point<float> lastMousePos;
  float snapDistancePixel = 24.0f;

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
      float normX = std::clamp(pos.x, 0.0f, w) / w;
      if (value[0] != normX) {
        value[0] = normX;
        attachment.setValueAsPartOfGesture(0, parameter[0]->convertFrom0to1(value[0]));
        isUpdated = true;
      }
    }

    if (lock != AxisLock::y) {
      float normY = std::clamp(h - pos.y, 0.0f, h) / h;
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

    // 1. If currently snapped, handle hysteresis (stickiness)
    if (isSnapping[axis]) {
      snapDiffAccumulator[axis] += deltaPixel;

      // If we pulled far enough, break the snap
      if (std::abs(snapDiffAccumulator[axis]) > snapDistancePixel) {
        isSnapping[axis] = false;
        snapDiffAccumulator[axis] = 0.0f;
        // We do NOT apply the delta this frame.
        // This effectively "resets" the relative motion anchor to the current mouse
        // position, preventing the value from jumping to catch up with the mouse cursor.
      }
      return;
    }

    // 2. Normal movement
    float currentVal = value[axis];
    float deltaNorm = deltaPixel / rangePixels;
    float nextVal = currentVal + deltaNorm;

    // 3. Check for new snaps
    const auto &axisSnaps = snaps[axis];
    if (!axisSnaps.empty()) {
      if (deltaNorm > 0) {
        // Moving Positive
        auto it = std::upper_bound(axisSnaps.begin(), axisSnaps.end(), currentVal);
        if (it != axisSnaps.end()) {
          // If we crossed or hit a snap point
          if (nextVal >= *it) {
            value[axis] = *it;
            isSnapping[axis] = true;
            snapDiffAccumulator[axis] = 0.0f;
            return;
          }
        }
      } else if (deltaNorm < 0) {
        // Moving Negative
        auto it = std::lower_bound(axisSnaps.begin(), axisSnaps.end(), currentVal);
        if (it != axisSnaps.begin()) {
          auto prev = std::prev(it);
          // If we crossed or hit a snap point
          if (nextVal <= *prev) {
            value[axis] = *prev;
            isSnapping[axis] = true;
            snapDiffAccumulator[axis] = 0.0f;
            return;
          }
        }
      }
    }

    // Apply value if no snap occurred
    value[axis] = std::clamp(nextVal, 0.0f, 1.0f);
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

    for (size_t i = 0; i < 2; ++i) {
      if (!parameter[i]) continue;
      value[i] = parameter[i]->convertTo0to1(parameter[i]->getValue());
    }
    attachment.sendInitialUpdate();
  }

  virtual ~XYPad() override {}

  void setSnaps(const std::vector<float> &snapsX_, const std::vector<float> &snapsY_)
  {
    snaps[0] = snapsX_;
    if (!snaps[0].empty()) std::sort(snaps[0].begin(), snaps[0].end());

    snaps[1] = snapsY_;
    if (!snaps[1].empty()) std::sort(snaps[1].begin(), snaps[1].end());
  }

  void setSnapDistance(float pixels) { snapDistancePixel = pixels; }

  void paint(juce::Graphics &g) override
  {
    const float width = (float)getWidth();
    const float height = (float)getHeight();

    // Background.
    g.setColour(pal.boxBackground());
    g.fillAll();

    // Grid.
    constexpr float dotRadius = 2.0f;
    g.setColour(pal.foregroundInactive());

    for (size_t ix = 1; ix < nGrid; ++ix) {
      for (size_t iy = 1; iy < nGrid; ++iy) {
        auto cx = std::floor(ix * width / nGrid);
        auto cy = std::floor(iy * height / nGrid);
        g.fillEllipse(cx - dotRadius, cy - dotRadius, dotRadius * 2, dotRadius * 2);
      }
    }

    // Mouse Cursor Crosshair.
    if (isMouseEntered) {
      g.setColour(pal.highlightMain());
      g.drawLine(0, mousePosition.y, width, mousePosition.y, 1.0f);
      g.drawLine(mousePosition.x, 0, mousePosition.x, height, 1.0f);
    }

    // Value Indicator.
    auto valueX = std::floor(value[0] * width);
    auto valueY = std::floor((1.0f - value[1]) * height);
    constexpr float valR = 8.0f;

    g.setColour(pal.foreground());
    g.drawEllipse(valueX - valR, valueY - valR, valR * 2, valR * 2, 2.0f);

    g.drawLine(0, valueY, width, valueY, 1.0f);
    g.drawLine(valueX, 0, valueX, height, 1.0f);

    // Border.
    g.setColour((isMouseEntered || isEditing) ? pal.highlightMain() : pal.border());
    g.drawRect(0.0f, 0.0f, width, height, borderWidth);
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

      // Left half = X Parameter. Right half = Y Parameter.
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
      snapDiffAccumulator = {0.0f, 0.0f};
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

      mousePosition = {value[0] * getWidth(), (1.0f - value[1]) * getHeight()};

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

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

} // namespace Uhhyou
