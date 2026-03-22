// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Uhhyou/librarylicense.hpp"
#include "widgets.hpp"

#include <functional>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

namespace Uhhyou {

template<typename ProcessorType>
class EditorBase : public juce::AudioProcessorEditor,
                   private juce::FocusChangeListener,
                   private juce::Timer {
public:
  EditorBase(ProcessorType& p, const juce::String& infoText)
      : AudioProcessorEditor(p), processor_(p), focusOverlay_(palette_), numberEditor_(palette_),
        statusBar_(*this, palette_),
        pluginInfoButton_(*this, palette_, statusBar_, navManager_, processor_.getName(), infoText,
                          getLibraryLicenseText()),
        undoButton_(*this, palette_, statusBar_, numberEditor_, "Undo", "",
                    [&]() {
                      if (processor_.undoManager.canUndo()) { processor_.undoManager.undo(); }
                    }),
        redoButton_(*this, palette_, statusBar_, numberEditor_, "Redo", "",
                    [&]() {
                      if (processor_.undoManager.canRedo()) { processor_.undoManager.redo(); }
                    }),
        randomizeButton_(*this, palette_, statusBar_, numberEditor_, "Randomize",
                         "Undo/Redo to revert.", [this]() { performRandomize(); }),
        presetManager_(*this, palette_, statusBar_, &(processor_.undoManager),
                       processor_.param.tree),
        settingsButton_(
          *this, palette_, statusBar_, numberEditor_, "GUI Settings", "Open settings menu",
          [this]() {
            juce::PopupMenu menu;

            bool focus
              = getStateTree().getProperty("KeyboardFocusEnabled", palette_.keyboardFocusEnabled());
            menu.addItem("Keyboard Navigation (Steals host shortcuts)", true, focus,
                         [this, focus]() {
                           setGlobalKeyboardFocus(!focus);
                           palette_.updateSetting("keyboardFocusEnabled", !focus);
                         });

            menu.addItem("Reset Window Size to 100%", [this]() {
              float currentScale
                = getStateTree().getProperty("windowScale", palette_.windowScale());
              if (currentScale > 0.0f) {
                setSize(juce::roundToInt(getWidth() / currentScale),
                        juce::roundToInt(getHeight() / currentScale));
              }
            });

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsButton_));
          }) {
    juce::Desktop::getInstance().addFocusChangeListener(this);

    bool initialFocus
      = getStateTree().getProperty("KeyboardFocusEnabled", palette_.keyboardFocusEnabled());
    setResizable(true, false);
    setWantsKeyboardFocus(initialFocus);
    setMouseClickGrabsKeyboardFocus(initialFocus);

    navManager_.pushScope(this, &mainScope_);
    mainScope_.setKeyboardFocusEnabled(initialFocus);

    tooltipWindow_.setOpaque(false);
    registerInteractive(undoButton_);
    registerInteractive(redoButton_);
    registerInteractive(randomizeButton_);
    registerInteractive(presetManager_);
    registerInteractive(settingsButton_);
    registerInteractive(pluginInfoButton_);

    auto savedLocks = getStateTree().getProperty("ParamLocks").toString();
    auto lockedIds = juce::StringArray::fromTokens(savedLocks, ",", "");
    for (const auto& id : lockedIds) {
      if (auto* prm = processor_.param.tree.getParameter(id)) {
        paramPtrToId_[prm] = id;
        paramLocks_.lock(prm);
      }
    }

    paramLocks_.setOnChange([this]() {
      saveParamLocks();
      repaint();
    });
  }

  ~EditorBase() override { juce::Desktop::getInstance().removeFocusChangeListener(this); }

  void resized() override {
    focusOverlay_.setBounds(getLocalBounds());
    focusOverlay_.toFront(false);

    // A mitigation to disable mouse events while resizing. See `mouseEnter` for details.
    setInterceptsMouseClicks(false, false);
    startTimer(100);
  }

  void timerCallback() override {
    // Inheritance of juce::Timer and `timerCallback` is a mitigation to disable mouse events while
    // resizing. See `mouseEnter` for details.
    stopTimer();
    setInterceptsMouseClicks(true, true);
  }

  void paint(juce::Graphics& ctx) override {
    ctx.setColour(palette_.background());
    ctx.fillAll();

    auto groupLabelFont = palette_.getFont(TextSize::normal);
    ctx.setColour(palette_.foreground());
    ctx.setFont(groupLabelFont);
    for (const auto& x : groupLabels_) {
      x.paint(ctx, groupLabelFont, 2 * palette_.borderWidth(),
              palette_.getFontHeight(TextSize::normal));
    }
  }

  void paintOverChildren(juce::Graphics& ctx) override {
    int shadowSize = std::max(1, int(palette_.borderWidth()));
    juce::DropShadow shadow(palette_.foreground(), 4 * shadowSize, {0, shadowSize});

    auto drawShadowIfHovered = [&](juce::Component& cmp) -> bool {
      if (!cmp.isVisible() || !(cmp.isMouseOverOrDragging(true) || cmp.hasKeyboardFocus(false))) {
        return false;
      }
      auto bounds = getLocalArea(&cmp, cmp.getLocalBounds());
      juce::Graphics::ScopedSaveState save(ctx);
      ctx.excludeClipRegion(bounds);
      if (tooltipWindow_.isVisible()) { ctx.excludeClipRegion(tooltipWindow_.getBounds()); }

      for (auto& safeDrawer : managedDrawers_) {
        if (auto* drawer = safeDrawer.getComponent()) {
          if (&cmp == &(drawer->getToggleButton())) {
            bounds = getLocalArea(drawer, drawer->getLocalBounds());
            ctx.excludeClipRegion(
              juce::Rectangle<int>(bounds.getX(), 0, getWidth() - bounds.getX(), getHeight()));
            break;
          }
        }
      }

      shadow.drawForRectangle(ctx, bounds);
      return true;
    };

    for (auto& safePtr : mainScope_.components) {
      if (auto* cmp = safePtr.getComponent()) {
        if (drawShadowIfHovered(*cmp)) { break; }
      }
    }
  }

  virtual void mouseDown(const juce::MouseEvent& event) override {
    if (event.originalComponent == this && getWantsKeyboardFocus()) { grabKeyboardFocus(); }
    numberEditor_.setVisible(false);
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    // `enableUnboundedMouseMovement(false)` is a mitigation. At least on Windows 11, when resizing
    // a plugin window, it seems like a hidden cursor remains in the initial position where resizing
    // is started while dragging the edge. The issue is that the hidden cursor fires mouse events.
    // Same for `mouseExit`.
    event.source.enableUnboundedMouseMovement(false);
    repaint();
  }

  void mouseExit(const juce::MouseEvent& event) override {
    event.source.enableUnboundedMouseMovement(false);
    repaint();
  }

  void globalFocusChanged(juce::Component*) override { repaint(); }

  bool keyPressed(const juce::KeyPress& key) override {
    if (key == juce::KeyPress::escapeKey) {
      if (!navManager_.handleEscape()) {
        giveAwayKeyboardFocus();
        if (auto* peer = getPeer()) { peer->grabFocus(); }
      }
      return true;
    }

    if (key.isKeyCode(juce::KeyPress::tabKey)) {
      return navManager_.handleTab(key.getModifiers().isShiftDown());
    }

    return false;
  }

  std::unique_ptr<juce::ComponentTraverser> createFocusTraverser() override { return nullptr; }

  std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override {
    return nullptr;
  }

  void focusGained(juce::Component::FocusChangeType) override { repaint(); }
  void focusLost(juce::Component::FocusChangeType) override { repaint(); }

protected:
  ProcessorType& processor_;
  Palette palette_;
  Uhhyou::LookAndFeel lookAndFeel_{palette_};

  NavigationManager navManager_;
  FocusScope mainScope_;

  juce::TooltipWindow tooltipWindow_{this, 400};
  FocusRingOverlay focusOverlay_;
  NumberEditor numberEditor_;
  StatusBar statusBar_;
  PluginInfoButton pluginInfoButton_;
  ActionButton<> undoButton_;
  ActionButton<> redoButton_;
  ActionButton<> randomizeButton_;
  PresetManager presetManager_;
  ActionButton<> settingsButton_;
  std::vector<GroupLabel> groupLabels_;

  using ComponentSharedPtr = std::shared_ptr<juce::Component>;
  std::vector<ComponentSharedPtr> widgetStorage_;
  std::unordered_map<juce::String, std::vector<ComponentSharedPtr>> sections_;
  std::vector<juce::Component::SafePointer<HorizontalDrawer>> managedDrawers_;

  ParameterLockRegistry paramLocks_;
  std::unordered_map<const juce::AudioProcessorParameter*, juce::String> paramPtrToId_;

  using RandomizeFn = std::function<float(float)>;
  std::unordered_map<const juce::AudioProcessorParameter*, RandomizeFn> randomizers_;

  virtual void performRandomize() {
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    std::random_device dev;
    std::mt19937 rng(dev());

    auto params = processor_.getParameters();
    for (auto& prm : params) {
      if (paramLocks_.isLocked(prm)) { continue; }

      float normalized = dist(rng);
      if (auto it = randomizers_.find(prm); it != randomizers_.end()) {
        normalized = it->second(normalized);
      }

      prm->beginChangeGesture();
      prm->setValueNotifyingHost(normalized);
      prm->endChangeGesture();
    }
  }

  inline juce::ValueTree getStateTree() {
    return processor_.param.tree.state.getOrCreateChildWithName("GUI", nullptr);
  }

  float getWindowScale() { return getStateTree().getProperty("windowScale", 1.0f); }

  void initWindow(int scaledWidth, int scaledHeight) {
    getConstrainer()->setFixedAspectRatio(double(scaledWidth) / double(scaledHeight));
    setSize(scaledWidth, scaledHeight);

    addAndMakeVisible(focusOverlay_);
    focusOverlay_.toFront(false);
  }

  float updateScale(int defaultHeight) {
    const float scale = getHeight() / float(defaultHeight);
    getStateTree().setProperty("windowScale", scale, nullptr);
    palette_.resize(scale);
    groupLabels_.clear();
    statusBar_.setText("Window Scale: " + juce::String(juce::roundToInt(scale * 100.0f)) + "%");
    return scale;
  }

  int layoutActionSectionAndPluginInfo(int left, int top, int sectionWidth, int labelW, int labelX,
                                       int labelH, int labelY, float scale) {
    const int nameTop0 = layoutActionSection(groupLabels_, left, top, sectionWidth, labelW, labelX,
                                             labelH, labelY, undoButton_, redoButton_,
                                             randomizeButton_, presetManager_, settingsButton_);
    pluginInfoButton_.setBounds(juce::Rectangle<int>{left, nameTop0, sectionWidth, labelH});
    pluginInfoButton_.scale(scale);
    return nameTop0;
  }

  void setGlobalKeyboardFocus(bool enable) {
    getStateTree().setProperty("KeyboardFocusEnabled", enable, nullptr);

    setWantsKeyboardFocus(enable);
    setMouseClickGrabsKeyboardFocus(enable);

    mainScope_.setKeyboardFocusEnabled(enable);

    if (!enable) {
      juce::MessageManager::callAsync([]() { juce::Component::unfocusAllComponents(); });
    }
  }

  void saveParamLocks() {
    juce::StringArray ids;
    for (auto* p : paramLocks_.getLockedParameters()) {
      if (auto it = paramPtrToId_.find(p); it != paramPtrToId_.end()) { ids.add(it->second); }
    }
    getStateTree().setProperty("ParamLocks", ids.joinIntoString(","), nullptr);
  }

  // Registers a component for interaction (Focus, Shadows, Mouse events). Does not take ownership.
  void registerInteractive(juce::Component& widget) {
    if (mainScope_.add(widget)) {
      widget.addMouseListener(this, true);

      bool focusEnabled
        = getStateTree().getProperty("KeyboardFocusEnabled", palette_.keyboardFocusEnabled());
      widget.setWantsKeyboardFocus(focusEnabled);
      widget.setMouseClickGrabsKeyboardFocus(focusEnabled);
    }
  }

  void registerDrawer(HorizontalDrawer& drawer, const juce::String& stateKey,
                      const std::vector<juce::String>& sectionNames,
                      std::function<juce::Point<int>(float scale, bool isOpen)> calcSize) {
    managedDrawers_.push_back(&drawer);

    for (const auto& secName : sectionNames) {
      if (auto st = sections_.find(secName); st != sections_.end()) {
        for (auto& widget : st->second) { drawer.addContent(widget.get()); }
      }
    }

    bool isDrawerOpen = getStateTree().getProperty(stateKey, drawer.isOpen());
    drawer.setOpen(isDrawerOpen, false);

    drawer.onStateChange = [this, stateKey, calcSize](bool isOpen) {
      getStateTree().setProperty(stateKey, isOpen, nullptr);
      float currentScale = getWindowScale();
      auto newSize = calcSize(currentScale, isOpen);
      getConstrainer()->setFixedAspectRatio(double(newSize.x) / double(newSize.y));
      setSize(newSize.x, newSize.y);
    };
  }

  void registerParameterId(const juce::String& id, const juce::AudioProcessorParameter* prm) {
    if (prm) { paramPtrToId_[prm] = id; }
  }

  void registerRandomizer(const juce::AudioProcessorParameter* prm, RandomizeFn randomizer) {
    if (prm != nullptr && randomizer != nullptr) { randomizers_[prm] = std::move(randomizer); }
  }

  // Adds a widget to a layout section (takes Ownership via shared_ptr).
  void addRawWidgetToSection(const juce::String& sectionTitle,
                             std::shared_ptr<juce::Component> widget) {
    auto [iter, inserted] = sections_.try_emplace(sectionTitle);
    iter->second.push_back(widget);
  }

  // Helper to compose a LabeledWidget, register it, and store ownership.
  void addToSection(const juce::String& sectionTitle, const juce::String& label,
                    juce::Component& component, const juce::RangedAudioParameter* const parameter,
                    LabeledWidget::Layout option = LabeledWidget::showLabel) {
    auto widget = std::make_shared<LabeledWidget>(paramLocks_, palette_, statusBar_, label,
                                                  component, parameter, option);

    addAndMakeVisible(*widget);
    addRawWidgetToSection(sectionTitle, widget);

    if (option == LabeledWidget::showLabel) { registerInteractive(widget->lockableLabel()); }
    registerInteractive(component);
  }

  template<Style style = Style::common, typename Scale>
  auto addMomentaryButton(const juce::String& section, const juce::String& paramId, Scale& scale,
                          const juce::String& label = "", const juce::String& hint = "",
                          LabeledWidget::Layout layout = LabeledWidget::expand,
                          RandomizeFn randomizer = nullptr) {
    using ButtonType = MomentaryButton<Scale, style>;
    return addButton_<ButtonType>(section, paramId, scale, label, hint, layout,
                                  std::move(randomizer));
  }

  template<Style style = Style::common, typename Scale>
  auto addToggleButton(const juce::String& section, const juce::String& paramId, Scale& scale,
                       const juce::String& label = "", const juce::String& hint = "",
                       LabeledWidget::Layout layout = LabeledWidget::expand,
                       RandomizeFn randomizer = nullptr) {
    using ButtonType = ToggleButton<Scale, style>;
    return addButton_<ButtonType>(section, paramId, scale, label, hint, layout,
                                  std::move(randomizer));
  }

  template<Style style = Style::common, typename Scale>
  auto addTextKnob(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                   const std::vector<float> snaps, int precision = 5,
                   const juce::String& label = "",
                   LabeledWidget::Layout layoutOption = LabeledWidget::showLabel,
                   RandomizeFn randomizer = nullptr) {
    using KnobT = TextKnob<Scale, style>;
    return addKnob_<KnobT>(sectionTitle, paramId, scale, snaps, precision, label, layoutOption,
                           std::move(randomizer));
  }

  template<Style style = Style::common, typename Scale>
  auto addRotaryTextKnob(const juce::String& sectionTitle, const juce::String& paramId,
                         Scale& scale, const std::vector<float> snaps, int precision = 5,
                         const juce::String& label = "",
                         LabeledWidget::Layout layoutOption = LabeledWidget::showLabel,
                         RandomizeFn randomizer = nullptr) {
    using KnobT = RotaryTextKnob<Scale, style>;
    return addKnob_<KnobT>(sectionTitle, paramId, scale, snaps, precision, label, layoutOption,
                           std::move(randomizer));
  }

  template<Style style = Style::common, typename Scale>
  auto addComboBox(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                   const std::vector<juce::String>& menuItems, juce::String label,
                   LabeledWidget::Layout layoutOption = LabeledWidget::showLabel,
                   RandomizeFn randomizer = nullptr) {
    auto prm = processor_.param.tree.getParameter(paramId);
    if (label.isEmpty() && prm != nullptr) { label = prm->getName(2048); }
    auto widget = std::make_shared<ComboBox<Scale, style>>(
      *this, palette_, &processor_.undoManager, prm, scale, statusBar_, numberEditor_, menuItems);

    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, prm, layoutOption);
    } else {
      addRawWidgetToSection("Misc", widget);
      registerInteractive(*widget);
    }

    registerParameterId(paramId, prm);

    if (randomizer) {
      registerRandomizer(prm, std::move(randomizer));
    } else if (prm != nullptr && !menuItems.empty()) {
      registerRandomizer(prm, [&scale, size = menuItems.size()](float randVal) {
        int idx = int(randVal * float(size));
        if (idx >= int(size)) { idx = int(size) - 1; }
        return float(scale.invmap(float(idx)));
      });
    }

    widgetStorage_.push_back(widget);
    return widget;
  }

  std::shared_ptr<XYPad> addXYPad(const juce::String& sectionTitle, const juce::String& paramIdX,
                                  const juce::String& paramIdY, const std::vector<float> snapsX,
                                  const std::vector<float> snapsY, juce::String labelX = "",
                                  juce::String labelY = "", RandomizeFn randX = nullptr,
                                  RandomizeFn randY = nullptr) {
    auto prmX = processor_.param.tree.getParameter(paramIdX);
    auto prmY = processor_.param.tree.getParameter(paramIdY);

    registerParameterId(paramIdX, prmX);
    registerParameterId(paramIdY, prmY);

    registerRandomizer(prmX, std::move(randX));
    registerRandomizer(prmY, std::move(randY));

    if (labelX.isEmpty() && prmX) { labelX = prmX->getName(2048); }
    if (labelY.isEmpty() && prmY) { labelY = prmY->getName(2048); }

    auto pad
      = std::make_shared<XYPad>(*this, palette_, &processor_.undoManager,
                                std::array<juce::RangedAudioParameter*, 2>{prmX, prmY}, statusBar_);
    pad->setSnaps(snapsX, snapsY);

    juce::String snapKey = juce::String(paramIdX) + "_" + paramIdY + "_Snap";
    bool initialSnap = getStateTree().getProperty(snapKey, true);

    auto wrapper = std::make_shared<LabeledXYPad>(
      paramLocks_, palette_, statusBar_, *pad, labelX, labelY, prmX, prmY, initialSnap,
      [this, snapKey](bool state) { getStateTree().setProperty(snapKey, state, nullptr); });

    addAndMakeVisible(*wrapper);

    if (sectionTitle.isNotEmpty()) {
      addRawWidgetToSection(sectionTitle, wrapper);
    } else {
      addRawWidgetToSection("Misc", wrapper);
    }

    registerInteractive(wrapper->getToggle());
    registerInteractive(wrapper->getLabelX());
    registerInteractive(wrapper->getLabelY());
    registerInteractive(*pad);

    widgetStorage_.push_back(pad);
    return pad;
  }

private:
  template<typename KnobType, typename Scale>
  auto addKnob_(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                const std::vector<float>& snaps, int precision, juce::String label,
                LabeledWidget::Layout layoutOption, RandomizeFn randomizer) {
    auto prm = processor_.param.tree.getParameter(paramId);

    if (label.isEmpty() && prm != nullptr) {
      label = prm->getName(2048);
      auto unit = prm->getLabel();
      if (!unit.isEmpty()) { label += " [" + unit + "]"; }
    }

    auto widget = std::make_shared<KnobType>(*this, palette_, &processor_.undoManager, prm, scale,
                                             statusBar_, numberEditor_, precision);

    widget->setSnaps(snaps);

    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, prm, layoutOption);
    } else {
      addRawWidgetToSection("Misc", widget);
      registerInteractive(*widget);
    }

    registerParameterId(paramId, prm);
    registerRandomizer(prm, std::move(randomizer));
    widgetStorage_.push_back(widget);
    return widget;
  }

  template<typename ButtonType, typename Scale>
  auto addButton_(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                  juce::String label, const juce::String& hint, LabeledWidget::Layout layout,
                  RandomizeFn randomizer) {
    auto prm = processor_.param.tree.getParameter(paramId);
    if (label.isEmpty() && prm) { label = prm->getName(2048); }

    auto widget = std::make_shared<ButtonType>(*this, palette_, &processor_.undoManager, prm, scale,
                                               statusBar_, numberEditor_, label, hint);

    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, prm, layout);
    } else {
      addRawWidgetToSection("Misc", widget);
      registerInteractive(*widget);
    }

    registerParameterId(paramId, prm);
    registerRandomizer(prm, std::move(randomizer));
    widgetStorage_.push_back(widget);
    return widget;
  }
};

} // namespace Uhhyou
