// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "widgets.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Uhhyou {

template<typename ProcessorType>
class EditorBase : public juce::AudioProcessorEditor, private juce::FocusChangeListener {
public:
  EditorBase(ProcessorType& p, const juce::String& infoText, const juce::String& licenseText,
             std::function<void(void)> randomizeButtonOnClick)
      : AudioProcessorEditor(p), processor(p), focusOverlay(palette), numberEditor(palette),
        statusBar(*this, palette), pluginInfoButton(*this, palette, statusBar, navManager,
                                                    processor.getName(), infoText, licenseText),
        undoButton(*this, palette, statusBar, numberEditor, "Undo", "",
                   [&]() {
                     if (processor.undoManager.canUndo()) { processor.undoManager.undo(); }
                   }),
        redoButton(*this, palette, statusBar, numberEditor, "Redo", "",
                   [&]() {
                     if (processor.undoManager.canRedo()) { processor.undoManager.redo(); }
                   }),
        randomizeButton(*this, palette, statusBar, numberEditor, "Randomize",
                        "Undo/Redo to revert.", randomizeButtonOnClick),
        presetManager(*this, palette, statusBar, &(processor.undoManager), processor.param.tree) {
    juce::Desktop::getInstance().addFocusChangeListener(this);

    setResizable(true, false);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    navManager.pushScope(this, &mainScope);

    tooltipWindow.setOpaque(false);
    registerInteractive(undoButton);
    registerInteractive(redoButton);
    registerInteractive(randomizeButton);
    registerInteractive(presetManager);
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
  }

  void paint(juce::Graphics& ctx) override {
    ctx.setColour(palette.background());
    ctx.fillAll();

    auto groupLabelFont = palette.getFont(palette.textSizeUi());
    ctx.setColour(palette.foreground());
    ctx.setFont(groupLabelFont);
    for (const auto& x : groupLabels) {
      x.paint(ctx, groupLabelFont, 2 * palette.borderThin(), palette.textSizeUi());
    }
  }

  void paintOverChildren(juce::Graphics& ctx) override {
    int shadowSize = int(palette.borderThin());
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
    if (event.originalComponent == this) { this->grabKeyboardFocus(); }
    numberEditor.setVisible(false);
  }

  void mouseEnter(const juce::MouseEvent&) override { repaint(); }
  void mouseExit(const juce::MouseEvent&) override { repaint(); }
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
  PluginInfoButton pluginInfoButton;
  std::vector<GroupLabel> groupLabels;

  using ComponentSharedPtr = std::shared_ptr<juce::Component>;
  std::vector<ComponentSharedPtr> widgetStorage;
  std::unordered_map<juce::String, std::vector<ComponentSharedPtr>> sections;

  ParameterLockRegistry paramLocks;
  std::unordered_map<const juce::AudioProcessorParameter*, juce::String> paramPtrToId;

  inline juce::ValueTree getStateTree() {
    return processor.param.tree.state.getOrCreateChildWithName("GUI", nullptr);
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
    if (mainScope.add(widget)) { widget.addMouseListener(this, true); }
  }

  // Adds a widget to a layout section (takes Ownership via shared_ptr).
  void addRawWidgetToSection(const juce::String& sectionTitle,
                             std::shared_ptr<juce::Component> widget) {
    auto [iter, inserted] = sections.try_emplace(sectionTitle);
    iter->second.push_back(widget);
  }

  void registerParameterId(const juce::String& id, const juce::AudioProcessorParameter* prm) {
    if (prm) { paramPtrToId[prm] = id; }
  }

  // Helper to compose a LabeledWidget, register it, and store ownership.
  void addToSection(const juce::String& sectionTitle, const juce::String& label,
                    juce::Component& component,
                    const juce::AudioProcessorParameter* const parameter,
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
                          LabeledWidget::Layout layout = LabeledWidget::expand) {
    using ButtonType = MomentaryButton<Scale, style>;
    return addButton_<ButtonType>(section, paramId, scale, label, hint, layout);
  }

  template<Style style = Style::common, typename Scale>
  auto addToggleButton(const juce::String& section, const juce::String& paramId, Scale& scale,
                       const juce::String& label = "", const juce::String& hint = "",
                       LabeledWidget::Layout layout = LabeledWidget::expand) {
    using ButtonType = ToggleButton<Scale, style>;
    return addButton_<ButtonType>(section, paramId, scale, label, hint, layout);
  }

  template<Style style = Style::common, typename Scale>
  auto addTextKnob(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                   const std::vector<float> snaps, int precision = 5,
                   const juce::String& label = "",
                   LabeledWidget::Layout layoutOption = LabeledWidget::showLabel) {
    using KnobType = TextKnob<Scale, style>;
    return addKnob_<KnobType>(sectionTitle, paramId, scale, snaps, precision, label, layoutOption);
  }

  template<Style style = Style::common, typename Scale>
  auto addRotaryTextKnob(const juce::String& sectionTitle, const juce::String& paramId,
                         Scale& scale, const std::vector<float> snaps, int precision = 5,
                         const juce::String& label = "",
                         LabeledWidget::Layout layoutOption = LabeledWidget::showLabel) {
    using KnobType = RotaryTextKnob<Scale, style>;
    return addKnob_<KnobType>(sectionTitle, paramId, scale, snaps, precision, label, layoutOption);
  }

  template<Style style = Style::common, typename Scale>
  auto addComboBox(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                   const std::vector<juce::String>& menuItems, juce::String label,
                   LabeledWidget::Layout layoutOption = LabeledWidget::showLabel) {
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
    widgetStorage.push_back(widget);
    return widget;
  }

  std::shared_ptr<XYPad> addXYPad(const juce::String& sectionTitle, const juce::String& paramIdX,
                                  const juce::String& paramIdY, const std::vector<float> snapsX,
                                  const std::vector<float> snapsY, juce::String labelX = "",
                                  juce::String labelY = "") {
    auto prmX = processor.param.tree.getParameter(paramIdX);
    auto prmY = processor.param.tree.getParameter(paramIdY);

    registerParameterId(paramIdX, prmX);
    registerParameterId(paramIdY, prmY);

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
                LabeledWidget::Layout layoutOption) {
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
    widgetStorage.push_back(widget);
    return widget;
  }

  template<typename ButtonType, typename Scale>
  auto addButton_(const juce::String& sectionTitle, const juce::String& paramId, Scale& scale,
                  juce::String label, const juce::String& hint, LabeledWidget::Layout layout) {
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
    widgetStorage.push_back(widget);
    return widget;
  }
};

} // namespace Uhhyou
