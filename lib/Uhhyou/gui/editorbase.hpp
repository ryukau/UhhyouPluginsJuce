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
      : AudioProcessorEditor(p), processor(p), focusOverlay(palette), numberEditor(palette),
        statusBar(*this, palette),
        pluginInfoButton(*this, palette, statusBar, navManager, processor.getName(), infoText,
                         getLibraryLicenseText()),
        undoButton(*this, palette, statusBar, numberEditor, "Undo", "",
                   [&]() {
                     if (processor.undoManager.canUndo()) { processor.undoManager.undo(); }
                   }),
        redoButton(*this, palette, statusBar, numberEditor, "Redo", "",
                   [&]() {
                     if (processor.undoManager.canRedo()) { processor.undoManager.redo(); }
                   }),
        randomizeButton(*this, palette, statusBar, numberEditor, "Randomize",
                        "Undo/Redo to revert.", [this]() { performRandomize(); }),
        presetManager(*this, palette, statusBar, &(processor.undoManager), processor.param.tree),
        settingsButton(
          *this, palette, statusBar, numberEditor, "GUI Settings", "Open settings menu", [this]() {
            juce::PopupMenu menu;

            bool focus
              = getStateTree().getProperty("KeyboardFocusEnabled", palette.keyboardFocusEnabled());
            menu.addItem("Keyboard Navigation (Steals host shortcuts)", true, focus,
                         [this, focus]() {
                           setGlobalKeyboardFocus(!focus);
                           palette.updateSetting("keyboardFocusEnabled", !focus);
                         });

            menu.addItem("Reset Window Size to 100%", [this]() {
              float currentScale = getStateTree().getProperty("windowScale", palette.windowScale());
              if (currentScale > 0.0f) {
                setSize(juce::roundToInt(getWidth() / currentScale),
                        juce::roundToInt(getHeight() / currentScale));
              }
            });

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsButton));
          }) {
    juce::Desktop::getInstance().addFocusChangeListener(this);

    bool initialFocus
      = getStateTree().getProperty("KeyboardFocusEnabled", palette.keyboardFocusEnabled());
    setResizable(true, false);
    setWantsKeyboardFocus(initialFocus);
    setMouseClickGrabsKeyboardFocus(initialFocus);

    navManager.pushScope(this, &mainScope);
    mainScope.setKeyboardFocusEnabled(initialFocus);

    tooltipWindow.setOpaque(false);
    registerInteractive(undoButton);
    registerInteractive(redoButton);
    registerInteractive(randomizeButton);
    registerInteractive(presetManager);
    registerInteractive(settingsButton);
    registerInteractive(pluginInfoButton);

    auto savedLocks = getStateTree().getProperty("ParamLocks").toString();
    auto lockedIds = juce::StringArray::fromTokens(savedLocks, ",", "");
    for (const auto& id : lockedIds) {
      if (auto* prm = processor.param.tree.getParameter(id)) {
        paramPtrToId[prm] = id;
        paramLocks.lock(prm);
      }
    }

    paramLocks.setOnChange([this]() {
      this->saveParamLocks();
      this->repaint();
    });
  }

  ~EditorBase() override { juce::Desktop::getInstance().removeFocusChangeListener(this); }

  void resized() override {
    focusOverlay.setBounds(getLocalBounds());
    focusOverlay.toFront(false);

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
    ctx.setColour(palette.background());
    ctx.fillAll();

    auto groupLabelFont = palette.getFont(TextSize::normal);
    ctx.setColour(palette.foreground());
    ctx.setFont(groupLabelFont);
    for (const auto& x : groupLabels) {
      x.paint(ctx, groupLabelFont, 2 * palette.borderWidth(),
              palette.getFontHeight(TextSize::normal));
    }
  }

  void paintOverChildren(juce::Graphics& ctx) override {
    int shadowSize = std::max(1, int(palette.borderWidth()));
    juce::DropShadow shadow(palette.foreground(), 4 * shadowSize, {0, shadowSize});

    auto drawShadowIfHovered = [&](juce::Component& cmp) -> bool {
      if (!cmp.isVisible() || !cmp.isMouseOverOrDragging(true)) { return false; }
      auto bounds = getLocalArea(&cmp, cmp.getLocalBounds());
      juce::Graphics::ScopedSaveState save(ctx);
      ctx.excludeClipRegion(bounds);
      if (tooltipWindow.isVisible()) { ctx.excludeClipRegion(tooltipWindow.getBounds()); }
      shadow.drawForRectangle(ctx, bounds);
      return true;
    };

    for (auto& safePtr : mainScope.components) {
      if (auto* cmp = safePtr.getComponent()) {
        if (drawShadowIfHovered(*cmp)) { break; }
      }
    }
  }

  virtual void mouseDown(const juce::MouseEvent& event) override {
    if (event.originalComponent == this && this->getWantsKeyboardFocus()) {
      this->grabKeyboardFocus();
    }
    numberEditor.setVisible(false);
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
      if (!navManager.handleEscape()) {
        giveAwayKeyboardFocus();
        if (auto* peer = getPeer()) { peer->grabFocus(); }
      }
      return true;
    }

    if (key.isKeyCode(juce::KeyPress::tabKey)) {
      return navManager.handleTab(key.getModifiers().isShiftDown());
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
  ProcessorType& processor;
  Palette palette;
  Uhhyou::LookAndFeel lookAndFeel{palette};

  NavigationManager navManager;
  FocusScope mainScope;

  juce::TooltipWindow tooltipWindow{this, 400};
  FocusRingOverlay focusOverlay;
  NumberEditor numberEditor;
  StatusBar statusBar;
  ActionButton<> undoButton;
  ActionButton<> redoButton;
  ActionButton<> randomizeButton;
  PresetManager presetManager;
  ActionButton<> settingsButton;
  PluginInfoButton pluginInfoButton;
  std::vector<GroupLabel> groupLabels;

  using ComponentSharedPtr = std::shared_ptr<juce::Component>;
  std::vector<ComponentSharedPtr> widgetStorage;
  std::unordered_map<juce::String, std::vector<ComponentSharedPtr>> sections;

  ParameterLockRegistry paramLocks;
  std::unordered_map<const juce::AudioProcessorParameter*, juce::String> paramPtrToId;

  using RandomizeFn = std::function<float(float)>;
  std::unordered_map<const juce::AudioProcessorParameter*, RandomizeFn> randomizers;

  virtual void performRandomize() {
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    std::random_device dev;
    std::mt19937 rng(dev());

    auto params = processor.getParameters();
    for (auto& prm : params) {
      if (this->paramLocks.isLocked(prm)) { continue; }

      float normalized = dist(rng);
      if (auto it = this->randomizers.find(prm); it != this->randomizers.end()) {
        normalized = it->second(normalized);
      }

      prm->beginChangeGesture();
      prm->setValueNotifyingHost(normalized);
      prm->endChangeGesture();
    }
  }

  inline juce::ValueTree getStateTree() {
    return processor.param.tree.state.getOrCreateChildWithName("GUI", nullptr);
  }

  float getWindowScale() { return getStateTree().getProperty("windowScale", 1.0f); }

  void initWindow(int scaledWidth, int scaledHeight) {
    getConstrainer()->setFixedAspectRatio(double(scaledWidth) / double(scaledHeight));
    setSize(scaledWidth, scaledHeight);

    addAndMakeVisible(focusOverlay);
    focusOverlay.toFront(false);
  }

  float updateScale(int defaultHeight) {
    const float scale = getHeight() / float(defaultHeight);
    getStateTree().setProperty("windowScale", scale, nullptr);
    palette.resize(scale);
    groupLabels.clear();
    statusBar.setText("Window Scale: " + juce::String(juce::roundToInt(scale * 100.0f)) + "%");
    return scale;
  }

  int layoutActionSectionAndPluginInfo(int left, int top, int sectionWidth, int labelW, int labelX,
                                       int labelH, int labelY, float scale) {
    const int nameTop0
      = layoutActionSection(groupLabels, left, top, sectionWidth, labelW, labelX, labelH, labelY,
                            undoButton, redoButton, randomizeButton, presetManager, settingsButton);
    pluginInfoButton.setBounds(juce::Rectangle<int>{left, nameTop0, sectionWidth, labelH});
    pluginInfoButton.scale(scale);
    return nameTop0;
  }

  void setGlobalKeyboardFocus(bool enable) {
    getStateTree().setProperty("KeyboardFocusEnabled", enable, nullptr);

    setWantsKeyboardFocus(enable);
    setMouseClickGrabsKeyboardFocus(enable);

    mainScope.setKeyboardFocusEnabled(enable);

    if (!enable) {
      juce::MessageManager::callAsync([]() { juce::Component::unfocusAllComponents(); });
    }
  }

  void saveParamLocks() {
    juce::StringArray ids;
    for (auto* p : paramLocks.getLockedParameters()) {
      if (auto it = paramPtrToId.find(p); it != paramPtrToId.end()) { ids.add(it->second); }
    }
    getStateTree().setProperty("ParamLocks", ids.joinIntoString(","), nullptr);
  }

  // Registers a component for interaction (Focus, Shadows, Mouse events). Does not take ownership.
  void registerInteractive(juce::Component& widget) {
    if (mainScope.add(widget)) {
      widget.addMouseListener(this, true);

      bool focusEnabled
        = getStateTree().getProperty("KeyboardFocusEnabled", palette.keyboardFocusEnabled());
      widget.setWantsKeyboardFocus(focusEnabled);
      widget.setMouseClickGrabsKeyboardFocus(focusEnabled);
    }
  }

  void registerParameterId(const juce::String& id, const juce::AudioProcessorParameter* prm) {
    if (prm) { paramPtrToId[prm] = id; }
  }

  void registerRandomizer(const juce::AudioProcessorParameter* prm, RandomizeFn randomizer) {
    if (prm != nullptr && randomizer != nullptr) { randomizers[prm] = std::move(randomizer); }
  }

  // Adds a widget to a layout section (takes Ownership via shared_ptr).
  void addRawWidgetToSection(const juce::String& sectionTitle,
                             std::shared_ptr<juce::Component> widget) {
    auto [iter, inserted] = sections.try_emplace(sectionTitle);
    iter->second.push_back(widget);
  }

  // Helper to compose a LabeledWidget, register it, and store ownership.
  void addToSection(const juce::String& sectionTitle, const juce::String& label,
                    juce::Component& component, const juce::RangedAudioParameter* const parameter,
                    LabeledWidget::Layout option = LabeledWidget::showLabel) {
    auto widget = std::make_shared<LabeledWidget>(paramLocks, palette, statusBar, label, component,
                                                  parameter, option);

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
    using KnobType = TextKnob<Scale, style>;
    return addKnob_<KnobType>(sectionTitle, paramId, scale, snaps, precision, label, layoutOption,
                              std::move(randomizer));
  }

  template<Style style = Style::common, typename Scale>
  auto addRotaryTextKnob(const juce::String& sectionTitle, const juce::String& paramId,
                         Scale& scale, const std::vector<float> snaps, int precision = 5,
                         const juce::String& label = "",
                         LabeledWidget::Layout layoutOption = LabeledWidget::showLabel,
                         RandomizeFn randomizer = nullptr) {
    using KnobType = RotaryTextKnob<Scale, style>;
    return addKnob_<KnobType>(sectionTitle, paramId, scale, snaps, precision, label, layoutOption,
                              std::move(randomizer));
  }

  template<Style style = Style::common, typename Scale>
  auto addComboBox(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                   const std::vector<juce::String>& menuItems, juce::String label,
                   LabeledWidget::Layout layoutOption = LabeledWidget::showLabel,
                   RandomizeFn randomizer = nullptr) {
    auto prm = processor.param.tree.getParameter(paramId);
    if (label.isEmpty() && prm != nullptr) { label = prm->getName(2048); }
    auto widget = std::make_shared<ComboBox<Scale, style>>(
      *this, palette, &processor.undoManager, prm, scale, statusBar, numberEditor, menuItems);

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

    widgetStorage.push_back(widget);
    return widget;
  }

  std::shared_ptr<XYPad> addXYPad(const juce::String& sectionTitle, const juce::String& paramIdX,
                                  const juce::String& paramIdY, const std::vector<float> snapsX,
                                  const std::vector<float> snapsY, juce::String labelX = "",
                                  juce::String labelY = "", RandomizeFn randX = nullptr,
                                  RandomizeFn randY = nullptr) {
    auto prmX = processor.param.tree.getParameter(paramIdX);
    auto prmY = processor.param.tree.getParameter(paramIdY);

    registerParameterId(paramIdX, prmX);
    registerParameterId(paramIdY, prmY);

    registerRandomizer(prmX, std::move(randX));
    registerRandomizer(prmY, std::move(randY));

    if (labelX.isEmpty() && prmX) { labelX = prmX->getName(2048); }
    if (labelY.isEmpty() && prmY) { labelY = prmY->getName(2048); }

    auto pad
      = std::make_shared<XYPad>(*this, palette, &processor.undoManager,
                                std::array<juce::RangedAudioParameter*, 2>{prmX, prmY}, statusBar);
    pad->setSnaps(snapsX, snapsY);

    juce::String snapKey = juce::String(paramIdX) + "_" + paramIdY + "_Snap";
    bool initialSnap = getStateTree().getProperty(snapKey, true);

    auto wrapper = std::make_shared<LabeledXYPad>(
      paramLocks, palette, statusBar, *pad, labelX, labelY, prmX, prmY, initialSnap,
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

    widgetStorage.push_back(pad);
    return pad;
  }

private:
  template<typename KnobType, typename Scale>
  auto addKnob_(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                const std::vector<float>& snaps, int precision, juce::String label,
                LabeledWidget::Layout layoutOption, RandomizeFn randomizer) {
    auto prm = processor.param.tree.getParameter(paramId);

    if (label.isEmpty() && prm != nullptr) {
      label = prm->getName(2048);
      auto unit = prm->getLabel();
      if (!unit.isEmpty()) { label += " [" + unit + "]"; }
    }

    auto widget = std::make_shared<KnobType>(*this, palette, &processor.undoManager, prm, scale,
                                             statusBar, numberEditor, precision);

    widget->setSnaps(snaps);

    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, prm, layoutOption);
    } else {
      addRawWidgetToSection("Misc", widget);
      registerInteractive(*widget);
    }

    registerParameterId(paramId, prm);
    registerRandomizer(prm, std::move(randomizer));
    widgetStorage.push_back(widget);
    return widget;
  }

  template<typename ButtonType, typename Scale>
  auto addButton_(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                  juce::String label, const juce::String& hint, LabeledWidget::Layout layout,
                  RandomizeFn randomizer) {
    auto prm = processor.param.tree.getParameter(paramId);
    if (label.isEmpty() && prm) { label = prm->getName(2048); }

    auto widget = std::make_shared<ButtonType>(*this, palette, &processor.undoManager, prm, scale,
                                               statusBar, numberEditor, label, hint);

    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, prm, layout);
    } else {
      addRawWidgetToSection("Misc", widget);
      registerInteractive(*widget);
    }

    registerParameterId(paramId, prm);
    registerRandomizer(prm, std::move(randomizer));
    widgetStorage.push_back(widget);
    return widget;
  }
};

} // namespace Uhhyou
