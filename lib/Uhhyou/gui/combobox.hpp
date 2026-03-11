// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "./numbereditor.hpp"
#include "style.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <vector>

namespace Uhhyou {

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
class ComboBox : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBox)

protected:
  class ComboBoxValueInterface : public juce::AccessibilityValueInterface {
  public:
    explicit ComboBoxValueInterface(ComboBox& cb) : comboBox(cb) {}

    bool isReadOnly() const override { return false; }

    double getCurrentValue() const override { return static_cast<double>(comboBox.itemIndex); }

    void setValue(double newValue) override {
      comboBox.attachment.setValueAsCompleteGesture(static_cast<float>(newValue));
    }

    juce::String getCurrentValueAsString() const override {
      if (comboBox.itemIndex < comboBox.items.size()) { return comboBox.items[comboBox.itemIndex]; }
      return {};
    }

    void setValueAsString(const juce::String& newValue) override {
      auto it = std::find(comboBox.items.begin(), comboBox.items.end(), newValue);
      if (it != comboBox.items.end()) {
        auto index = static_cast<size_t>(std::distance(comboBox.items.begin(), it));
        comboBox.attachment.setValueAsCompleteGesture(static_cast<float>(index));
      }
    }

    AccessibleValueRange getRange() const override {
      return {{0.0, static_cast<double>(comboBox.items.size() - 1)}, 0.0};
    }

  private:
    ComboBox& comboBox;
  };

  class ComboBoxAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    explicit ComboBoxAccessibilityHandler(ComboBox& cbToWrap, juce::AccessibilityActions actions)
        : juce::AccessibilityHandler(
            cbToWrap, juce::AccessibilityRole::comboBox, std::move(actions),
            Interfaces{std::make_unique<ComboBoxValueInterface>(cbToWrap)}),
          cb(cbToWrap) {}

    juce::String getTitle() const override {
      if (cb.parameter != nullptr) { return cb.parameter->getName(256); }
      return juce::AccessibilityHandler::getTitle();
    }

  private:
    ComboBox& cb;
  };

  juce::AudioProcessorEditor& editor;
  const juce::RangedAudioParameter* const parameter;

  Scale& scale;
  Palette& pal;
  StatusBar& statusBar;
  NumberEditor& numberEditor;
  juce::ParameterAttachment attachment;

  juce::PopupMenu menu;

  size_t itemIndex{};
  size_t defaultIndex{};

  bool isMouseEntered = false;
  juce::Font font;
  std::vector<juce::String> items;

  void showHostMenuNative(juce::Point<int> position) {
    if (auto* hostContext = editor.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter)) {
        hostContextMenu->showNativeMenu(position);
      }
    }
  }

  void showHostMenuJuce() {
    if (auto* hostContext = editor.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter)) {
        hostContextMenu->getEquivalentPopupMenu().showMenuAsync(
          juce::PopupMenu::Options().withTargetComponent(this));
      }
    }
  }

  void updateStatusBar() {
    if (items.empty()) { return; }
    statusBar.setText(std::format("{}: {} ({}/{})", parameter->getName(256).toRawUTF8(),
                                  items[itemIndex].toRawUTF8(), itemIndex + 1, items.size()));
  }

  void setInternalValue(size_t newIndex) {
    if (itemIndex == newIndex || newIndex >= items.size()) { return; }
    itemIndex = newIndex;
    updateStatusBar();
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
    }
  }

  void invokeMenu() {
    menu.clear();
    menu.addSectionHeader(parameter->getName(2048));

    for (size_t idx = 0; idx < items.size(); ++idx) {
      menu.addItem(juce::PopupMenu::Item(items[idx])
                     .setID(static_cast<int>(idx) + 1)
                     .setTicked(idx == itemIndex));
    }

    menu.showMenuAsync(juce::PopupMenu::Options()
                         .withInitiallySelectedItem(static_cast<int>(itemIndex) + 1)
                         .withTargetComponent(this),
                       [this](int id) {
                         if (id != 0) { // 0 means menu was dismissed without selection
                           size_t idx = static_cast<size_t>(id - 1);
                           if (idx < items.size()) {
                             setInternalValue(idx);
                             attachment.setValueAsCompleteGesture(static_cast<float>(itemIndex));
                           }
                         }
                         this->repaint();
                       });
  }

public:
  ComboBox(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
           juce::RangedAudioParameter* parameter, Scale& scale, StatusBar& statusBar,
           NumberEditor& numberEditor, std::vector<juce::String> menuItems)
      : editor(editor), parameter(parameter), scale(scale), pal(palette), statusBar(statusBar),
        numberEditor(numberEditor),
        attachment(
          *parameter,
          [&](float newRaw) {
            size_t idx
              = static_cast<size_t>(std::max(0, static_cast<int>(std::floor(newRaw) + 0.5f)));
            setInternalValue(idx);
          },
          undoManager),
        defaultIndex([&]() -> size_t {
          if (menuItems.empty()) { return 0; }
          // TODO: C++26 std::saturate_cast may be used to simplify.
          auto value = scale.map(parameter->getDefaultValue());
          auto mapped = static_cast<size_t>(std::max(static_cast<decltype(value)>(0), value));
          return std::min(mapped, menuItems.size() - 1);
        }()),
        font(palette.getFont(TextSize::normal)), items(menuItems) {
    attachment.sendInitialUpdate();
    editor.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [this]() { invokeMenu(); });
    actions.addAction(juce::AccessibilityActionType::showMenu, [this]() { showHostMenuJuce(); });
    return std::make_unique<ComboBoxAccessibilityHandler>(*this, std::move(actions));
  }

  virtual void resized() override { font = pal.getFont(TextSize::normal); }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal.borderWidth(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());

    // Background.
    ctx.setColour(pal.surface());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    if constexpr (style == Uhhyou::Style::accent) {
      ctx.setColour(isMouseEntered ? pal.accent() : pal.border());
    } else if constexpr (style == Uhhyou::Style::warning) {
      ctx.setColour(isMouseEntered ? pal.warning() : pal.border());
    } else {
      ctx.setColour(isMouseEntered ? pal.main() : pal.border());
    }
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    if (itemIndex < items.size()) {
      ctx.setFont(font);
      ctx.setColour(pal.foreground());
      ctx.drawText(items[itemIndex], juce::Rectangle<float>(float(0), float(0), width, height),
                   juce::Justification::centred);
    }
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered = true;
    updateStatusBar();
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    statusBar.clear();
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent& event) override {
    if (event.mods.isRightButtonDown()) {
      showHostMenuNative(editor.getMouseXYRelative());
      return;
    }

    if (event.mods.isCommandDown()) {
      setInternalValue(defaultIndex);
      attachment.setValueAsCompleteGesture(static_cast<float>(itemIndex));
      return;
    }

    if (event.mods.isLeftButtonDown()) { invokeMenu(); }
  }

  virtual void mouseWheelMove(const juce::MouseEvent&,
                              const juce::MouseWheelDetails& wheel) override {
    if (std::abs(wheel.deltaY) <= std::numeric_limits<float>::epsilon()) { return; }
    if (items.empty()) { return; }

    size_t nextIndex = (itemIndex + (wheel.deltaY < 0 ? items.size() - 1 : 1)) % items.size();

    setInternalValue(nextIndex);
    attachment.setValueAsCompleteGesture(static_cast<float>(itemIndex));
  }

  virtual bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    const auto& mods = key.getModifiers();

    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      invokeMenu();
      return true;
    }

    if (key.isKeyCode(KP::F10Key) && mods.isShiftDown()) {
      showHostMenuJuce();
      return true;
    }

    if (key.isKeyCode(KP::backspaceKey) || key.isKeyCode(KP::deleteKey)) {
      if (mods.isCommandDown()) {
        setInternalValue(defaultIndex);
        attachment.setValueAsCompleteGesture(static_cast<float>(itemIndex));
        return true;
      }
    }

    if (items.empty()) { return false; }

    int direction = 0;
    if (key.isKeyCode(KP::upKey) || key.isKeyCode(KP::leftKey)) {
      direction = -1;
    } else if (key.isKeyCode(KP::downKey) || key.isKeyCode(KP::rightKey)) {
      direction = 1;
    }

    if (direction != 0) {
      size_t nextIndex = (itemIndex + (direction < 0 ? items.size() - 1 : 1)) % items.size();
      setInternalValue(nextIndex);
      attachment.setValueAsCompleteGesture(static_cast<float>(itemIndex));
      return true;
    }

    return false;
  }
};

} // namespace Uhhyou
