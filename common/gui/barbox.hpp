// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "parameterarrayattachment.hpp"
#include "style.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <string>

namespace Uhhyou {

template<typename Scale, size_t nParameter> class BarBox : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarBox)

  enum class BarState : uint8_t { active, lock };

  juce::AudioProcessorEditor &editor;
  std::array<juce::RangedAudioParameter *, nParameter> parameter;
  Scale &scale;
  Uhhyou::Palette &pal;
  ParameterArrayAttachment<nParameter> attachment;

  bool isMouseEntered = false;
  juce::Point<float> mousePosition{float(-1), float(-1)};
  juce::Point<float> anchor{float(-1), float(-1)};
  BarState anchorState = BarState::active;
  int indexL = 0;
  int indexR = 0;
  int indexRange = 0;
  float sliderWidth = float(1);
  float barMargin = float(1);

  std::string name;
  std::array<std::string, nParameter> barIndices;
  std::array<BarState, nParameter> barState{};
  std::vector<float> active, locked; // Used to store temporary values.

  std::array<float, nParameter> value{};
  std::array<float, nParameter> defaultValue{};
  std::vector<std::array<float, nParameter>> undoValue;

  juce::Font indexFont;
  juce::Font nameFont;

  auto constructBarIndices()
  {
    std::array<std::string, nParameter> indices;
    for (size_t i = 0; i < indices.size(); ++i) indices[i] = std::to_string(i);
    return indices;
  }

  auto
  constructDefaultValue(std::array<juce::RangedAudioParameter *, nParameter> &parmeter)
  {
    std::array<float, nParameter> val;
    for (size_t i = 0; i < val.size(); ++i) val[i] = parmeter[i]->getDefaultValue();
    return val;
  }

  void updateValue()
  {
    for (int i = 0; i < value.size(); ++i)
      attachment.setValueAsPartOfGesture(i, scale.map(value[i]));
  }

  void editAndUpdateValue()
  {
    for (int i = 0; i < nParameter; ++i)
      attachment.setValueAsCompleteGesture(i, scale.map(value[i]));
  }

public:
  float sliderZero = 0;
  int32_t indexOffset = 0;
  bool liveUpdateLineEdit = true; // Set this false when line edit is slow.
  float scrollSensitivity = float(0.01);
  float altScrollSensitivity = float(0.001);
  std::array<float, nParameter> snapValue;

  BarBox(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    std::array<juce::RangedAudioParameter *, nParameter> parameter,
    Scale &scale,
    std::string name)
    : editor(editor)
    , parameter(parameter)
    , scale(scale)
    , pal(palette)
    , attachment(
        parameter,
        [&](int index, float rawValue) {
          if (index < 0 && index >= value.size()) return;
          auto normalized = scale.invmap(rawValue);
          if (value[index] != normalized) {
            value[index] = normalized;
            repaint();
          }
        },
        undoManager)
    , name(name)
    , barIndices(constructBarIndices())
    , defaultValue(constructDefaultValue(parameter))
    , indexFont(palette.getFont(palette.textSizeSmall()))
    , nameFont(palette.getFont(palette.textSizeBig()))
  {
    setWantsKeyboardFocus(true);

    setViewRange(0, 1);

    for (size_t i = 0; i < 4; ++i) undoValue.emplace_back(defaultValue);

    barState.fill(BarState::active);
    active.reserve(nParameter);
    locked.reserve(nParameter);

    attachment.sendInitialUpdate();

    editor.addAndMakeVisible(*this, 0);
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    using namespace juce;
    using RectF = Rectangle<float>;

    const float lw1 = pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float indexTextSize = pal.textSizeSmall() * float(1.25);
    const float width = float(getWidth());
    const float height = float(getHeight());

    // Background.
    ctx.setColour(pal.boxBackground());
    ctx.fillRoundedRectangle(0.0f, 0.0f, width, height, lw2);

    // Value bar.
    float sliderZeroHeight = height * (float(1) - sliderZero);
    for (int i = indexL; i < indexR; ++i) {
      auto left = (i - indexL) * sliderWidth;
      auto barWidth = sliderWidth - barMargin;
      auto top = height - value[i] * height;
      auto bottom = sliderZeroHeight;
      if (top > bottom) std::swap(top, bottom);
      ctx.setColour(
        barState[i] == BarState::active ? pal.highlightMain() : pal.foregroundInactive());
      ctx.fillRect(left, top, barWidth, bottom - top);
    }

    // Index text.
    ctx.setFont(indexFont);
    ctx.setColour(pal.foreground());
    if (sliderWidth >= indexTextSize) {
      for (int i = indexL; i < indexR; ++i) {
        auto left = (i - indexL) * sliderWidth;
        auto barWidth = sliderWidth - barMargin;
        ctx.drawText(
          barIndices[i].c_str(),
          RectF(left, height - indexTextSize, barWidth, indexTextSize),
          Justification::centred, false);
        if (barState[i] != BarState::active) {
          ctx.drawText(
            "L", RectF(left, 0, barWidth, indexTextSize), Justification::centred, false);
        }
      }
    }

    // Additional index text for zoom in.
    if (value.size() != size_t(indexRange)) {
      ctx.setColour(pal.overlay());
      std::string str = "<- #" + std::to_string(indexL);
      ctx.drawText(
        str.c_str(), RectF(2, 2, 10 * indexTextSize, 2 * indexTextSize),
        Justification::centredLeft);
    }

    // Border.
    ctx.setColour(pal.foreground());
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Highlight.
    if (isMouseEntered) {
      size_t index = size_t(indexL + indexRange * mousePosition.x / width);
      if (index < value.size()) {
        ctx.setColour(pal.overlayHighlight());
        ctx.fillRect((index - indexL) * sliderWidth, 0.0f, sliderWidth, height);

        // Index text.
        ctx.setFont(nameFont);
        ctx.setColour(pal.overlay());
        std::ostringstream os;
        os << "#" << std::to_string(index + indexOffset) << ": "
           << std::to_string(scale.map(value[index]));
        auto indexText = os.str();
        ctx.drawText(
          indexText.c_str(), RectF(0, 0, width, height), Justification::centred);

        if (barState[index] != BarState::active) {
          ctx.setFont(indexFont);
          ctx.drawText(
            "Locked", RectF(0, indexTextSize, width, 2 * indexTextSize),
            Justification::centred);
        }
      }
    } else {
      // Title.
      ctx.setFont(nameFont);
      ctx.setColour(pal.overlay());
      ctx.drawText(name.c_str(), RectF(0, 0, width, height), Justification::centred);
    }

    // Zero line.
    auto zeroLineHeight = height - sliderZero * height;
    ctx.setColour(pal.overlay());
    ctx.fillRect(0.0f, zeroLineHeight - lw1 / 2, width, lw1);
  }

  void resized() override
  {
    indexFont = pal.getFont(pal.textSizeSmall());
    nameFont = pal.getFont(pal.textSizeBig());

    refreshSliderWidth(float(getWidth()));
  }

  void mouseMove(const juce::MouseEvent &event) override
  {
    mousePosition = event.position;
    repaint();
  }

  void mouseEnter(const juce::MouseEvent &) override
  {
    isMouseEntered = true;
    repaint();
  }

  void mouseExit(const juce::MouseEvent &) override
  {
    giveAwayKeyboardFocus();

    isMouseEntered = false;
    repaint();
  }

  void mouseDown(const juce::MouseEvent &event) override
  {
    if (event.mods.isRightButtonDown()) {
      auto hostContext = editor.getHostContext();
      if (hostContext == nullptr) return;

      mousePosition = event.position;
      size_t index = calcIndex(mousePosition);
      if (index >= parameter.size()) return;

      auto hostContextMenu = hostContext->getContextMenuForParameter(parameter[index]);
      if (hostContextMenu == nullptr) return;

      hostContextMenu->showNativeMenu(editor.getMouseXYRelative());
      return;
    }

    grabKeyboardFocus();

    mousePosition = event.position;
    anchor = mousePosition;

    if (
      event.mods.isMiddleButtonDown() && event.mods.isCommandDown()
      && event.mods.isShiftDown())
    {
      anchorState = setStateFromPosition(mousePosition, BarState::lock);
    } else {
      setValueFromPosition(mousePosition, event.mods);
    }
    repaint();
  }

  void mouseUp(const juce::MouseEvent &) override
  {
    attachment.endGesture();
    pushUndoValue();
  }

  void mouseDrag(const juce::MouseEvent &event) override
  {
    mousePosition = event.position;
    if (event.mods.isLeftButtonDown()) {
      if (event.mods.isCommandDown() && event.mods.isShiftDown()) {
        setValueFromPosition(mousePosition, event.mods);
      } else {
        setValueFromLine(anchor, mousePosition, event.mods);
      }
      anchor = mousePosition;
    } else if (event.mods.isMiddleButtonDown()) {
      if (event.mods.isCommandDown() && event.mods.isShiftDown()) {
        setStateFromLine(anchor, mousePosition, anchorState);
      } else if (event.mods.isShiftDown()) {
        mousePosition.x = anchor.x;
        setValueFromPosition(mousePosition, false, false);
      } else {
        setValueFromLine(anchor, mousePosition, event.mods);
      }
    }
    repaint(); // Required to refresh highlighting position.
  }

  void mouseWheelMove(
    const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override
  {
    if (wheel.deltaY == 0) return;

    grabKeyboardFocus();

    size_t index = calcIndex(mousePosition);
    if (index >= value.size()) return;

    if (barState[index] != BarState::active) {
      return;
    }

    if (event.mods.isShiftDown()) {
      setValueAtIndex(index, value[index] + wheel.deltaY * altScrollSensitivity);
    } else {
      setValueAtIndex(index, value[index] + wheel.deltaY * scrollSensitivity);
    }
    attachment.setValueAsCompleteGesture(int(index), scale.map(value[index]));
    repaint();
  }

  bool keyPressed(const juce::KeyPress &key) override
  {
    constexpr auto isKey = juce::KeyPress::isKeyCurrentlyDown;

    if (!isMouseEntered || !key.isValid()) return false;

    size_t index = calcIndex(mousePosition);
    if (index >= value.size()) index = 0;

    const auto shift = key.getModifiers().isShiftDown();
    if (isKey('a')) {
      alternateSign(index);
    } else if (shift && isKey('d')) { // Alternative default. (toggle min/max)
      toggleMinMidMax(index);
    } else if (isKey('d')) { // reset to Default.
      resetToDefault();
    } else if (shift && isKey('e')) {
      emphasizeHigh(index);
    } else if (isKey('e')) {
      emphasizeLow(index);
    } else if (shift && isKey('f')) {
      highpass(index);
    } else if (isKey('f')) {
      averageLowpass(index);
    } else if (shift && isKey('i')) {
      invertInRange(index);
    } else if (isKey('i')) {
      invertFull(index);
    } else if (shift && isKey('l')) {
      lockAll(index);
    } else if (isKey('l')) {
      barState[index]
        = barState[index] == BarState::active ? BarState::lock : BarState::active;
    } else if (shift && isKey('n')) {
      normalizeFull(index);
    } else if (isKey('n')) {
      normalizeInRange(index);
    } else if (isKey('p')) { // Permute.
      applyAlgorithm(index, [&]() {
        std::random_device device;
        std::mt19937 rng(device());
        std::shuffle(active.begin(), active.end(), rng);
      });
    } else if (shift && isKey('r')) {
      sparseRandomize(index);
    } else if (isKey('r')) {
      totalRandomize(index);
    } else if (shift && isKey('S')) { // Sort ascending order.
      applyAlgorithm(index, [&]() { std::sort(active.begin(), active.end()); });
    } else if (isKey('s')) { // Sort descending order.
      applyAlgorithm(
        index, [&]() { std::sort(active.begin(), active.end(), std::greater<>()); });
    } else if (shift && isKey('t')) { // subTle randomize.
      mixRandomize(index, float(0.02));
    } else if (isKey('t')) { // subTle randomize. Random walk.
      randomize(index, float(0.02));
    } else if (shift && isKey('z')) { // Redo
      redo();
      editAndUpdateValue();
      repaint();
      return true;
    } else if (isKey('z')) { // Undo
      undo();
      editAndUpdateValue();
      repaint();
      return true;
    } else if (isKey(',')) { // Rotate back.
      applyAlgorithm(
        index, [&]() { std::rotate(active.begin(), active.begin() + 1, active.end()); });
    } else if (isKey('.')) { // Rotate forward.
      applyAlgorithm(index, [&]() {
        std::rotate(active.rbegin(), active.rbegin() + 1, active.rend());
      });
    } else if (isKey('1')) { // Decrease.
      multiplySkip(index, 1);
    } else if (isKey('2')) { // Decrease 2n.
      multiplySkip(index, 2);
    } else if (isKey('3')) { // Decrease 3n.
      multiplySkip(index, 3);
    } else if (isKey('4')) { // Decrease 4n.
      multiplySkip(index, 4);
    } else if (isKey('5')) { // Decimate and hold 2 samples.
      decimateHold(index, 2);
    } else if (isKey('6')) { // Decimate and hold 3 samples.
      decimateHold(index, 3);
    } else if (isKey('7')) { // Decimate and hold 4 samples.
      decimateHold(index, 4);
    } else if (isKey('8')) { // Decimate and hold 5 samples.
      decimateHold(index, 5);
    } else if (isKey('9')) { // Decimate and hold 6 samples.
      decimateHold(index, 6);
    } else {
      return false;
    }
    repaint();
    editAndUpdateValue();
    pushUndoValue();
    return true;
  }

  void setViewRange(float left, float right)
  {
    indexL = int(std::clamp<float>(left, float(0), float(1)) * value.size());
    indexR = int(std::clamp<float>(right, float(0), float(1)) * value.size());
    indexRange = indexR >= indexL ? indexR - indexL : 0;
    refreshSliderWidth(float(getWidth()));
    repaint();
  }

private:
  inline size_t calcIndex(juce::Point<float> &position)
  {
    return size_t(indexL + position.x / sliderWidth);
  }

  void refreshSliderWidth(float width)
  {
    sliderWidth = indexRange >= 1 ? width / indexRange : width;
    barMargin = sliderWidth <= float(4) ? float(1) : float(2);
  }

  float snap(float currentValue)
  {
    size_t index = 0;
    for (; index < snapValue.size(); ++index) {
      if (snapValue[index] >= currentValue) break;
    }
    return index < snapValue.size() ? snapValue[index] : float(1);
  }

  BarState setStateFromPosition(juce::Point<float> &position, BarState state)
  {
    size_t index = calcIndex(position);
    if (index >= value.size()) return BarState::active;

    barState[index] = barState[index] != state ? state : BarState::active;
    return barState[index];
  }

  void setStateFromLine(juce::Point<float> &p0, juce::Point<float> &p1, BarState state)
  {
    if (p0.x > p1.x) std::swap(p0, p1);

    int last = int(value.size()) - 1;
    if (last < 0) last = 0; // std::clamp is undefined if low is greater than high.

    int left = int(calcIndex(p0));
    int right = int(calcIndex(p1));

    if ((left < 0 && right < 0) || (left > last && right > last)) return;

    left = std::clamp(left, 0, last);
    right = std::clamp(right, 0, last);

    for (int idx = left; idx >= 0 && idx <= right; ++idx) barState[idx] = state;

    repaint();
  }

  void
  setValueFromPosition(juce::Point<float> &position, const juce::ModifierKeys &modifiers)
  {
    setValueFromPosition(position, modifiers.isCommandDown(), modifiers.isShiftDown());
  }

  void
  setValueFromPosition(juce::Point<float> &position, const bool ctrl, const bool shift)
  {
    size_t index = calcIndex(position);
    if (index >= value.size()) return;
    if (barState[index] != BarState::active) return;

    if (ctrl && !shift)
      setValueAtIndex(index, defaultValue[index]);
    else if (!ctrl && shift)
      setValueAtIndex(index, snap(float(1) - position.y / getHeight()));
    else
      setValueAtIndex(index, float(1) - position.y / getHeight());

    attachment.setValueAsPartOfGesture(int(index), scale.map(value[index]));
    repaint();
  }

  void setValueAtIndex(size_t index, float normalized)
  {
    if (barState[index] != BarState::active) return;
    if (index >= value.size()) return;
    attachment.beginGesture(int(index));
    value[index] = std::clamp(normalized, float(0), float(1));
  }

  void setValueFromLine(
    juce::Point<float> p0, juce::Point<float> p1, const juce::ModifierKeys &modifiers)
  {
    if (p0.x > p1.x) std::swap(p0, p1);

    size_t left = calcIndex(p0);
    size_t right = calcIndex(p1);
    if (left >= value.size() || right >= value.size()) return;

    const float p0y = p0.y;
    const float p1y = p1.y;

    if (left == right) { // p0 and p1 are in a same bar.
      if (barState[left] != BarState::active) return;

      if (modifiers.isCommandDown())
        setValueAtIndex(left, defaultValue[left]);
      else if (modifiers.isShiftDown())
        setValueAtIndex(left, snap(float(1) - anchor.y / getHeight()));
      else
        setValueAtIndex(left, float(1) - anchor.y / getHeight());

      attachment.setValueAsPartOfGesture(int(left), scale.map(value[left]));
      repaint();
      return;
    } else if (modifiers.isCommandDown()) {
      for (size_t idx = left; idx >= 0 && idx <= right; ++idx) {
        if (barState[left] != BarState::active) return;
        setValueAtIndex(idx, defaultValue[idx]);
      }
      if (liveUpdateLineEdit) updateValue();
      return;
    }

    const bool isSnapping = modifiers.isShiftDown();

    if (barState[left] == BarState::active) {
      auto val = float(1) - p0y / getHeight();
      setValueAtIndex(left, isSnapping ? snap(val) : val);
    }
    if (barState[right] == BarState::active) {
      auto val = float(1) - p1y / getHeight();
      setValueAtIndex(right, isSnapping ? snap(val) : val);
    }

    // In between.
    const float p0x = sliderWidth * (left + 1);
    const float p1x = sliderWidth * right;
    float pDiff = p1x - p0x;
    constexpr auto eps = std::numeric_limits<float>::epsilon();
    if (std::abs(pDiff) < eps) pDiff = std::copysign(eps, pDiff);
    const float slope = (p1y - p0y) / pDiff;

    const float yInc = slope * sliderWidth;
    float y = p0y;
    for (size_t idx = left + 1; idx < right; ++idx) {
      auto val = 1.0f - (y + 0.5f * yInc) / getHeight();
      setValueAtIndex(idx, isSnapping ? snap(val) : val);
      y += yInc;
    }

    if (liveUpdateLineEdit) updateValue();
    repaint();
  }

  void pushUndoValue()
  {
    std::rotate(undoValue.begin(), undoValue.begin() + 1, undoValue.end());
    undoValue.back() = value;
  }

  void undo()
  {
    std::rotate(undoValue.rbegin(), undoValue.rbegin() + 1, undoValue.rend());
    value = undoValue.back();
  }

  void redo()
  {
    std::rotate(undoValue.begin(), undoValue.begin() + 1, undoValue.end());
    value = undoValue.back();
  }

  template<typename Func> void applyAlgorithm(size_t start, Func func)
  {
    active.resize(0);
    locked.resize(0);

    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] == BarState::active)
        active.push_back(value[i]);
      else
        locked.push_back(value[i]);
    }

    func();

    size_t activeIndex = 0;
    size_t lockedIndex = 0;
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] == BarState::active) {
        value[i] = active[activeIndex];
        ++activeIndex;
      } else {
        value[i] = locked[lockedIndex];
        ++lockedIndex;
      }
    }
  }

  void resetToDefault()
  {
    for (size_t i = 0; i < value.size(); ++i) {
      if (barState[i] == BarState::active) value[i] = defaultValue[i];
    }
  }

  void toggleMinMidMax(size_t start)
  {
    float filler = 0;
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      filler = value[i] == 0 ? float(0.5) : value[i] == float(0.5) ? float(1) : float(0);
      start = i;
      break;
    }

    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] == BarState::active) value[i] = filler;
    }
  }

  void lockAll(size_t index)
  {
    std::fill(
      barState.begin(), barState.end(),
      barState[index] == BarState::active ? BarState::lock : BarState::active);
  }

  void alternateSign(size_t start)
  {
    for (size_t i = start; i < value.size(); i += 2) {
      if (barState[i] != BarState::active) continue;
      setValueAtIndex(i, 2 * sliderZero - value[i]);
    }
  }

  void averageLowpass(size_t start)
  {
    const int32_t range = 1;

    std::array<float, nParameter> result{value};
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      result[i] = 0;
      for (int32_t j = -range; j <= range; ++j) {
        size_t index = i + j; // Note that index is unsigned.
        if (index >= value.size()) continue;
        result[i] += value[index] - sliderZero;
      }
      setValueAtIndex(i, sliderZero + result[i] / float(2 * range + 1));
    }
  }

  /**
  Highpass equation is:
    `value[i] = sum((-0.5, 1.0, -0.5) * value[(i - 1, i, i + 1)])`
  Value of index outside of array is assumed to be same as closest element.
  */
  void highpass(size_t start)
  {
    std::array<float, nParameter> result{value};
    size_t last = value.size() - 1;
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      auto val = value[i] - sliderZero;
      result[i] = 0;
      result[i] -= (i >= 1) ? value[i - 1] - sliderZero : val;
      result[i] -= (i < last) ? value[i + 1] - sliderZero : val;
      result[i] = val + float(0.5) * result[i];
      setValueAtIndex(i, sliderZero + result[i]);
    }
  }

  void totalRandomize(size_t start)
  {
    std::random_device dev;
    std::mt19937_64 rng(dev());
    std::uniform_real_distribution<float> dist(float(0), float(1));
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      value[i] = dist(rng);
    }
  }

  void randomize(size_t start, float amount)
  {
    std::random_device dev;
    std::mt19937_64 rng(dev());
    amount /= 2;
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      std::uniform_real_distribution<float> dist(value[i] - amount, value[i] + amount);
      setValueAtIndex(i, dist(rng));
    }
  }

  void mixRandomize(size_t start, float mix)
  {
    std::random_device dev;
    std::mt19937_64 rng(dev());
    std::uniform_real_distribution<float> dist(
      sliderZero - float(0.5), sliderZero + float(0.5));
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      setValueAtIndex(i, value[i] + mix * (dist(rng) - value[i]));
    }
  }

  void sparseRandomize(size_t start)
  {
    std::random_device device;
    std::mt19937_64 rng(device());
    std::uniform_real_distribution<float> dist(float(0), float(1));
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      if (dist(rng) < float(0.1)) value[i] = dist(rng);
    }
  }

  struct ValuePeak {
    float minNeg = 2;
    float minPos = 2;
    float maxNeg = -1;
    float maxPos = -1;
  };

  ValuePeak getValuePeak(size_t start, bool skipZero)
  {
    ValuePeak pk;
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      float val = std::abs(value[i] - sliderZero);
      if (value[i] == sliderZero) {
        if (skipZero) continue;
        pk.minNeg = 0;
        pk.minPos = 0;
      } else if (value[i] < sliderZero) {
        if (val > pk.maxNeg)
          pk.maxNeg = val;
        else if (val < pk.minNeg)
          pk.minNeg = val;
      } else {
        if (val > pk.maxPos)
          pk.maxPos = val;
        else if (val < pk.minPos)
          pk.minPos = val;
      }
    }
    if (pk.minNeg > 1) pk.minNeg = 0;
    if (pk.minPos > 1) pk.minPos = 0;
    if (pk.maxNeg < 0) pk.maxNeg = 0;
    if (pk.maxPos < 0) pk.maxPos = 0;
    return pk;
  }

  void invertInRange(size_t start)
  {
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      float val = value[i] >= sliderZero ? float(1) - value[i] + sliderZero
                                         : sliderZero - value[i];
      setValueAtIndex(i, val);
    }
  }

  void invertFull(size_t start)
  {
    auto pk = getValuePeak(start, false);
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;

      if (value[i] < sliderZero) {
        auto x = float(1) + value[i] - (value[i] / sliderZero);
        setValueAtIndex(i, std::clamp(x, sliderZero, float(1)));
      } else {
        auto x
          = sliderZero - sliderZero * (value[i] - sliderZero) / (float(1) - sliderZero);
        setValueAtIndex(i, std::clamp(x, float(0), sliderZero));
      }
    }
  }

  void normalizeFull(size_t start)
  {
    float min = float(1);
    float max = float(0);
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      if (value[i] < min) min = value[i];
      if (value[i] > max) max = value[i];
    }

    if (max == min) return;

    float scaling = float(1) / max - min;

    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      setValueAtIndex(i, std::clamp(value[i] * scaling, float(0), float(1)));
    }
  }

  void normalizeInRange(size_t start) noexcept
  {
    auto pk = getValuePeak(start, true);

    float diffNeg = pk.maxNeg - pk.minNeg;
    float diffPos = pk.maxPos - pk.minPos;

    float mulNeg = (sliderZero - pk.minNeg) / diffNeg;
    float mulPos = (float(1) - sliderZero - pk.minPos) / diffPos;

    if (diffNeg == float(0)) {
      mulNeg = float(0);
      pk.minNeg = pk.maxNeg == float(0) ? float(0) : float(1);
    }
    if (diffPos == float(0)) {
      mulPos = float(0);
      pk.minPos = pk.maxPos == float(0) ? float(0) : float(1);
    }

    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      if (value[i] == sliderZero) continue;
      auto val = value[i] < sliderZero
        ? (value[i] - sliderZero + pk.minNeg) * mulNeg + sliderZero - pk.minNeg
        : (value[i] - sliderZero - pk.minPos) * mulPos + sliderZero + pk.minPos;
      setValueAtIndex(i, val);
    }
  }

  void multiplySkip(size_t start, size_t interval) noexcept
  {
    for (size_t i = start; i < value.size(); i += interval) {
      if (barState[i] != BarState::active) continue;
      setValueAtIndex(i, (value[i] - sliderZero) * float(0.9) + sliderZero);
    }
  }

  void decimateHold(size_t start, size_t interval)
  {
    size_t counter = 0;
    float hold = 0;
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;

      if (counter == 0) hold = value[i];
      counter = (counter + 1) % interval;
      setValueAtIndex(i, hold);
    }
  }

  void emphasizeLow(size_t start)
  {
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      setValueAtIndex(
        i, (value[i] - sliderZero) / std::pow(float(i + 1), float(0.0625)) + sliderZero);
    }
  }

  void emphasizeHigh(size_t start)
  {
    for (size_t i = start; i < value.size(); ++i) {
      if (barState[i] != BarState::active) continue;
      auto emphasis = float(0.9) + float(0.1) * float(i + 1) / value.size();
      setValueAtIndex(i, (value[i] - sliderZero) * emphasis + sliderZero);
    }
  }
};

} // namespace Uhhyou
