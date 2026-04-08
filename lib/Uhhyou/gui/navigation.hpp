// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

namespace Uhhyou {

struct FocusScope {
  std::vector<juce::Component::SafePointer<juce::Component>> components;
  std::unordered_set<juce::Component*> componentSet;

  bool add(juce::Component& c) {
    if (componentSet.contains(&c)) { return false; }
    componentSet.insert(&c);
    components.emplace_back(&c);
    return true;
  }

  void clear() {
    componentSet.clear();
    components.clear();
  }

  bool contains(juce::Component* c) const { return componentSet.contains(c); }

  void setKeyboardFocus(bool enable) {
    for (auto& safePtr : components) {
      if (auto* cmp = safePtr.getComponent()) {
        cmp->setWantsKeyboardFocus(enable);
        cmp->setMouseClickGrabsKeyboardFocus(enable);
      }
    }
  }

private:
  JUCE_DECLARE_WEAK_REFERENCEABLE(FocusScope)
};

class NavigationManager : private juce::FocusChangeListener {
private:
  struct ScopeEntry {
    juce::Component::SafePointer<juce::Component> owner;
    juce::WeakReference<FocusScope> scope;
  };

  std::vector<ScopeEntry> stack_;
  juce::Component::SafePointer<juce::Component> lastFocusedComponent_;
  bool isFocusInside_ = false;

public:
  NavigationManager() { juce::Desktop::getInstance().addFocusChangeListener(this); }

  ~NavigationManager() override { juce::Desktop::getInstance().removeFocusChangeListener(this); }

  void pushScope(juce::Component* owner, FocusScope* scope) {
    if (owner && scope) { stack_.push_back({owner, scope}); }
  }

  void popScope(juce::Component* owner) {
    auto it = std::remove_if(stack_.begin(), stack_.end(),
                             [&](const ScopeEntry& entry) { return entry.owner == owner; });

    stack_.erase(it, stack_.end());
  }

  bool handleTab(bool isShiftDown) {
    cleanupStack();
    if (stack_.empty()) { return false; }

    FocusScope* scope = stack_.back().scope;
    if (scope == nullptr || scope->components.empty()) { return false; }

    std::vector<juce::Component*> focusable;
    focusable.reserve(scope->components.size());
    for (auto& safePtr : scope->components) {
      if (auto* c = safePtr.getComponent()) {
        if (isValidTarget(c)) { focusable.push_back(c); }
      }
    }
    if (focusable.empty()) { return false; }

    const std::ptrdiff_t count = std::ssize(focusable);
    juce::Component* focused = juce::Component::getCurrentlyFocusedComponent();

    const auto it = std::ranges::find_if(focusable | std::views::reverse,
                                         [focused](auto* c) { return c == focused; });
    const std::ptrdiff_t currentIndex = std::distance(focusable.begin(), it.base()) - 1;

    std::ptrdiff_t nextIndex = 0;
    if (currentIndex == -1) {
      nextIndex = isShiftDown ? (count - 1) : 0;
    } else {
      const std::ptrdiff_t direction = isShiftDown ? -1 : 1;
      nextIndex = (currentIndex + direction + count) % count;
    }

    if (auto* c = focusable[static_cast<size_t>(nextIndex)]) {
      c->grabKeyboardFocus();
      return true;
    }

    return false;
  }

  bool handleEscape() {
    cleanupStack();

    if (stack_.size() <= 1) { return false; }

    if (auto* owner = stack_.back().owner.getComponent()) {
      bool handled = owner->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
      if (!handled && owner->getWantsKeyboardFocus()) { owner->grabKeyboardFocus(); }
      return true;
    }

    return false;
  }

private:
  void cleanupStack() {
    while (!stack_.empty()) {
      if (stack_.back().owner != nullptr && stack_.back().scope != nullptr) { break; }
      stack_.pop_back();
    }
  }

  bool isValidTarget(juce::Component* c) {
    return c != nullptr && c->getWantsKeyboardFocus() && c->isEnabled() && c->isShowing();
  }

  void globalFocusChanged(juce::Component* focusedComponent) override {
    bool currentlyInside = isComponentInside(focusedComponent);

    if (currentlyInside) {
      if (!isFocusInside_) {
        isFocusInside_ = true;

        bool isSpecificTarget = false;
        for (const auto& entry : stack_) {
          FocusScope* scope = entry.scope;
          if (scope != nullptr && scope->contains(focusedComponent)) {
            isSpecificTarget = true;
            break;
          }
        }

        if (!isSpecificTarget) {
          if (auto* target = findResumeTarget()) {
            if (target != focusedComponent) {
              juce::Component::SafePointer<juce::Component> safeTarget(target);
              juce::MessageManager::callAsync([safeTarget]() {
                if (auto* c = safeTarget.getComponent()) { c->grabKeyboardFocus(); }
              });
            }
          }
        }
      }

      if (isValidTarget(focusedComponent)) {
        for (const auto& entry : stack_) {
          FocusScope* scope = entry.scope;
          if (scope != nullptr && scope->contains(focusedComponent)) {
            lastFocusedComponent_ = focusedComponent;
            break;
          }
        }
      }
    } else {
      isFocusInside_ = false;
    }
  }

  bool isComponentInside(juce::Component* c) {
    if (!c) { return false; }
    if (stack_.empty()) { return false; }

    auto* root = stack_.front().owner.getComponent();
    if (!root) { return false; }
    return c == root || root->isParentOf(c);
  }

  juce::Component* findResumeTarget() {
    cleanupStack();
    if (stack_.empty()) { return nullptr; }

    FocusScope* currentScope = stack_.back().scope;
    if (currentScope == nullptr) { return nullptr; }

    auto* last = lastFocusedComponent_.getComponent();
    if (isValidTarget(last) && currentScope->contains(last)) { return last; }

    for (auto& safePtr : currentScope->components) {
      if (auto* c = safePtr.getComponent()) {
        if (isValidTarget(c)) { return c; }
      }
    }

    return nullptr;
  }
};

class FocusRingOverlay : public juce::Component, private juce::FocusChangeListener {
public:
  FocusRingOverlay(Palette& palette) : pal_(palette) {
    setInterceptsMouseClicks(false, false);
    juce::Desktop::getInstance().addFocusChangeListener(this);
  }

  ~FocusRingOverlay() override { juce::Desktop::getInstance().removeFocusChangeListener(this); }

  void globalFocusChanged(juce::Component*) override {
    if (getParentComponent() != nullptr) { toFront(false); }
    repaint();
  }

  void paint(juce::Graphics& ctx) override {
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (!focused) { return; }

    auto* parent = getParentComponent();
    if (!parent) { return; }

    if (focused != parent && parent->isParentOf(focused)) {
      auto bounds = getLocalArea(focused, focused->getLocalBounds()).toFloat();
      float thickness = pal_.borderWidth() * float(3);
      auto ringBounds = bounds.expanded(thickness);
      ctx.setColour(pal_.getForeground(pal_.background()).withAlpha(float(0.5)));
      ctx.drawRect(ringBounds, thickness);
    }
  }

private:
  Palette& pal_;
};

} // namespace Uhhyou
