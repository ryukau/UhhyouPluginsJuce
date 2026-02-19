#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

namespace Uhhyou {

struct FocusScope {
  std::vector<juce::Component::SafePointer<juce::Component>> components;
  std::unordered_set<juce::Component *> componentSet;

  bool add(juce::Component &c)
  {
    if (componentSet.contains(&c)) return false;

    componentSet.insert(&c);
    components.emplace_back(&c);
    return true;
  }

  void clear()
  {
    componentSet.clear();
    components.clear();
  }

  bool contains(juce::Component *c) const { return componentSet.contains(c); }
};

class NavigationManager {
private:
  struct ScopeEntry {
    juce::Component::SafePointer<juce::Component> owner;
    FocusScope *scope;
  };

  std::vector<ScopeEntry> stack;

public:
  void pushScope(juce::Component *owner, FocusScope *scope)
  {
    if (owner && scope) stack.push_back({owner, scope});
  }

  void popScope(juce::Component *owner)
  {
    auto it = std::remove_if(
      stack.begin(), stack.end(),
      [&](const ScopeEntry &entry) { return entry.owner == owner; });

    stack.erase(it, stack.end());
  }

  bool handleTab(bool isShiftDown)
  {
    cleanupStack();
    if (stack.empty()) return false;

    auto *scope = stack.back().scope;
    if (!scope || scope->components.empty()) return false;

    std::vector<juce::Component *> focusable;
    focusable.reserve(scope->components.size());
    for (auto &safePtr : scope->components) {
      if (auto *c = safePtr.getComponent()) {
        if (isValidTarget(c)) focusable.push_back(c);
      }
    }
    if (focusable.empty()) return false;

    const std::ptrdiff_t count = std::ssize(focusable);
    juce::Component *focused = juce::Component::getCurrentlyFocusedComponent();

    const auto it = std::ranges::find_if(
      focusable | std::views::reverse, [focused](auto *c) { return c == focused; });
    const std::ptrdiff_t currentIndex = std::distance(focusable.begin(), it.base()) - 1;

    std::ptrdiff_t nextIndex = 0;
    if (currentIndex == -1) {
      nextIndex = isShiftDown ? (count - 1) : 0;
    } else {
      const std::ptrdiff_t direction = isShiftDown ? -1 : 1;
      nextIndex = (currentIndex + direction + count) % count;
    }

    if (focusable[nextIndex]) {
      focusable[nextIndex]->grabKeyboardFocus();
      return true;
    }

    return false;
  }

private:
  void cleanupStack()
  {
    while (!stack.empty()) {
      if (stack.back().owner != nullptr) break;
      stack.pop_back();
    }
  }

  bool isValidTarget(juce::Component *c)
  {
    return c != nullptr && c->getWantsKeyboardFocus() && c->isEnabled() && c->isShowing();
  }
};

} // namespace Uhhyou
