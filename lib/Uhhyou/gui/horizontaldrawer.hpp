#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "numbereditor.hpp"
#include "style.hpp"

#include <format>
#include <functional>

namespace Uhhyou {

class HorizontalDrawer : public juce::Component {
private:
  class DrawerButton : public juce::Component {
  private:
    HorizontalDrawer& owner_;
    Palette& pal_;
    StatusBar& statusBar_;

    class AccessibilityHandler : public juce::AccessibilityHandler {
    public:
      explicit AccessibilityHandler(DrawerButton& button)
          : juce::AccessibilityHandler(button, juce::AccessibilityRole::button,
                                       createActions(button)),
            button_(button) {}

      juce::String getTitle() const override { return button_.owner_.name_ + " Drawer"; }

      juce::String getHelp() const override { return "Press Space or Enter to toggle the drawer."; }

      juce::AccessibleState getCurrentState() const override {
        auto state = juce::AccessibilityHandler::getCurrentState();
        return button_.owner_.isOpen() ? state.withExpanded() : state.withCollapsed();
      }

    private:
      static juce::AccessibilityActions createActions(DrawerButton& button) {
        juce::AccessibilityActions actions;
        auto toggleCb = [&button]() { button.owner_.setOpen(!button.owner_.isOpen()); };

        // Registering both press and toggle actions provides optimal compatibility with various
        // screen reader heuristics.
        actions.addAction(juce::AccessibilityActionType::press, toggleCb);
        actions.addAction(juce::AccessibilityActionType::toggle, toggleCb);
        return actions;
      }

      DrawerButton& button_;
    };

  public:
    DrawerButton(HorizontalDrawer& owner, Palette& pal, StatusBar& statusBar)
        : owner_(owner), pal_(pal), statusBar_(statusBar) {
      setWantsKeyboardFocus(true);
      setMouseClickGrabsKeyboardFocus(true);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
      return std::make_unique<AccessibilityHandler>(*this);
    }

    void paint(juce::Graphics& ctx) override {
      juce::Colour bgColour = pal_.surface();
      ctx.setColour(bgColour);
      ctx.fillRect(getLocalBounds());

      if (isMouseOverOrDragging()) {
        ctx.setColour(pal_.main().withAlpha(0.3f));
        ctx.fillRect(getLocalBounds());
      }

      ctx.setColour(pal_.getForeground(bgColour));

      float w = static_cast<float>(getWidth());
      float h = static_cast<float>(getHeight());
      float cx = w / 2.0f;
      float cy = h / 2.0f;
      float size = w * 0.25f;

      constexpr float arrowHeightMultiplier = 4.0f;
      float arrowWidth = size * 2.0f;
      float arrowHeight = size * arrowHeightMultiplier;
      float arrowX = cx - size;
      float arrowY = cy - (arrowHeight / 2.0f);

      juce::Path p;
      if (owner_.isOpen()) {
        p.startNewSubPath(0.0f, 0.5f);
        p.lineTo(1.0f, 0.0f);
        p.lineTo(0.62f, 0.5f);
        p.lineTo(1.0f, 1.0f);
        p.closeSubPath();
      } else {
        p.startNewSubPath(1.0f, 0.5f);
        p.lineTo(0.0f, 0.0f);
        p.lineTo(0.38f, 0.5f);
        p.lineTo(0.0f, 1.0f);
        p.closeSubPath();
      }
      p.scaleToFit(arrowX, arrowY, arrowWidth, arrowHeight, false);
      ctx.fillPath(p);
    }

    void mouseEnter(const juce::MouseEvent&) override {
      repaint();
      statusBar_.setText(std::format("Drawer: {}", owner_.isOpen() ? "Open" : "Closed"));
      owner_.repaint();
    }

    void mouseExit(const juce::MouseEvent&) override {
      repaint();
      statusBar_.clear();
      owner_.repaint();
    }

    void mouseDown(const juce::MouseEvent&) override { owner_.setOpen(!owner_.isOpen()); }

    bool keyPressed(const juce::KeyPress& key) override {
      if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey) {
        owner_.setOpen(!owner_.isOpen());
        return true;
      }
      return false;
    }

    void focusGained(juce::Component::FocusChangeType) override {
      repaint();
      owner_.repaint();
    }

    void focusLost(juce::Component::FocusChangeType) override {
      repaint();
      owner_.repaint();
    }
  };

  Palette& pal_;
  juce::String name_;
  bool isOpen_;

  DrawerButton toggleButton_;
  juce::Component contentContainer_;

  friend class DrawerButton;

public:
  std::function<void(bool)> onStateChange;

  HorizontalDrawer(Palette& palette, StatusBar& statusBar, const juce::String& name,
                   bool startOpen = true)
      : pal_(palette), name_(name), isOpen_(startOpen), toggleButton_(*this, pal_, statusBar) {
    addAndMakeVisible(toggleButton_);
    addChildComponent(contentContainer_);
    toggleButton_.setExplicitFocusOrder(1);
    contentContainer_.setExplicitFocusOrder(2);
    contentContainer_.setVisible(isOpen_);
  }

  void addContent(juce::Component* component) {
    if (component) { contentContainer_.addAndMakeVisible(component); }
  }

  void setOpen(bool shouldBeOpen, bool notify = true) {
    if (isOpen_ == shouldBeOpen) { return; }

    if (!shouldBeOpen) {
      auto* focused = juce::Component::getCurrentlyFocusedComponent();
      if (focused != nullptr && contentContainer_.isParentOf(focused)) {
        toggleButton_.grabKeyboardFocus();
      }
    }

    isOpen_ = shouldBeOpen;
    contentContainer_.setVisible(isOpen_);
    toggleButton_.repaint();

    if (auto* handler = toggleButton_.getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::structureChanged);
    }

    if (notify && onStateChange) { onStateChange(isOpen_); }
    resized();
  }

  bool isOpen() const { return isOpen_; }
  juce::Component& getToggleButton() { return toggleButton_; }

  static int calculateButtonWidth(float scale) {
    constexpr float baseButtonWidth = 20.0f;
    return static_cast<int>(std::lround(baseButtonWidth * scale));
  }

  void paint(juce::Graphics& ctx) override {
    if (isOpen_ && (toggleButton_.isMouseOverOrDragging() || toggleButton_.hasKeyboardFocus(false)))
    {
      auto contentBounds = getLocalBounds().withTrimmedLeft(toggleButton_.getWidth());
      ctx.setColour(pal_.main().withAlpha(0.1f));
      ctx.fillRect(contentBounds);
    }
  }

  void resized() override {
    int buttonWidth = static_cast<int>(std::lround(pal_.getFontHeight(TextSize::large)));
    toggleButton_.setBounds(0, 0, buttonWidth, getHeight());
    contentContainer_.setBounds(buttonWidth, 0, getWidth() - buttonWidth, getHeight());
  }
};

} // namespace Uhhyou
