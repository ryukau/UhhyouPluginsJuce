// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "filemenu.hpp"
#include "style.hpp"

#include <algorithm>
#include <ranges>
#include <vector>

namespace Uhhyou {

class PresetManager : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)

  class PresetManagerAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    PresetManagerAccessibilityHandler(PresetManager& pm, juce::AccessibilityActions act)
        : juce::AccessibilityHandler(pm, juce::AccessibilityRole::comboBox, std::move(act)),
          presetManager_(pm) {}

    juce::String getTitle() const override { return "Preset: " + presetManager_.text_; }

    juce::String getHelp() const override {
      return "Press Space or Enter to open the preset menu. Use Left and Right arrow keys to cycle "
             "presets.";
    }

  private:
    PresetManager& presetManager_;
  };

  juce::AudioProcessorEditor& editor_;
  juce::AudioProcessorValueTreeState& tree_;
  juce::UndoManager* undoManager_ = nullptr;

  Palette& pal_;
  StatusBar& statusBar_;
  juce::String text_{"Default"};

  std::unique_ptr<juce::FileChooser> fileChooser_;

  juce::File currentPresetFile_;
  std::vector<juce::File> presetCache_;

  bool isMouseEntered_ = false;
  juce::Point<int> mousePosition_;

  juce::Rectangle<int> previousButtonRegion_;
  juce::Rectangle<int> nextButtonRegion_;
  juce::Rectangle<int> textRegion_;

  void updateStatusBar(juce::Point<int> position) {
    if (previousButtonRegion_.contains(position)) {
      statusBar_.setText("Previous preset");
    } else if (nextButtonRegion_.contains(position)) {
      statusBar_.setText("Next preset");
    } else {
      statusBar_.setText("Preset menu");
    }
  }

  juce::File getPresetRoot() {
    auto presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("UhhyouPlugins")
                       .getChildFile(editor_.processor.getName());

    if (!presetDir.isDirectory()) {
      if (presetDir.createDirectory().failed()) { setPresetText("Error: FS Init Failed"); }
    }
    return presetDir;
  }

  void setPresetText(const juce::String& newText) {
    if (text_ == newText) { return; }
    text_ = newText;
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

  void savePreset() {
    auto xmlString = tree_.copyState().toXmlString();
    auto rootJuce = getPresetRoot();

    fileChooser_ = std::make_unique<juce::FileChooser>("Save Preset", rootJuce, "*.xml");

    auto callback = [this, xmlString](const juce::FileChooser& chooser) {
      auto file = chooser.getResult();
      if (file == juce::File()) { return; }

      if (file.replaceWithText(xmlString)) {
        currentPresetFile_ = file;
        presetCache_.clear();
        setPresetText(file.getFileNameWithoutExtension());
      } else {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Save Failed",
                                               "Could not save file: " + file.getFileName());
      }
    };

    using FBC = juce::FileBrowserComponent;
    fileChooser_->launchAsync(FBC::saveMode | FBC::warnAboutOverwriting, callback);
  }

  void loadPresetFromFileChooser() {
    fileChooser_ = std::make_unique<juce::FileChooser>("Load Preset", getPresetRoot(), "*.xml");

    using FBC = juce::FileBrowserComponent;
    fileChooser_->launchAsync(FBC::openMode | FBC::canSelectFiles,
                              [&](const juce::FileChooser& chooser) {
                                auto f = chooser.getResult();
                                if (f.existsAsFile()) { loadPreset(f); }
                              });
  }

  void loadPreset(const juce::File& file) {
    if (!file.existsAsFile()) { return; }

    auto xmlState = juce::parseXML(file);

    if (xmlState == nullptr || !xmlState->hasTagName(tree_.state.getType())) {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Load Failed",
                                             "Invalid preset file.");
      return;
    }

    auto loadedState = juce::ValueTree::fromXml(*xmlState);
    auto currentGuiState = tree_.state.getChildWithName("GUI").createCopy();
    auto presetGuiState = loadedState.getChildWithName("GUI");
    if (presetGuiState.isValid()) { loadedState.removeChild(presetGuiState, nullptr); }
    if (currentGuiState.isValid()) { loadedState.appendChild(currentGuiState, nullptr); }
    tree_.state.copyPropertiesAndChildrenFrom(loadedState, undoManager_);

    currentPresetFile_ = file;
    setPresetText(file.getFileNameWithoutExtension());
  }

  void ensureCacheLoaded() {
    if (!presetCache_.empty()) { return; }

    auto root = getPresetRoot();
    if (!root.isDirectory()) { return; }

    presetCache_.clear();

    auto files = root.findChildFiles(juce::File::findFiles, true, "*.xml");
    for (const auto& f : files) { presetCache_.push_back(f); }

    auto naturalSort = [](const juce::File& a, const juce::File& b) {
      return a.getFileName().compareNatural(b.getFileName()) < 0;
    };
    std::ranges::sort(presetCache_, naturalSort);
  }

  void cyclePreset(int direction) {
    ensureCacheLoaded();
    if (presetCache_.empty()) { return; }

    auto it = std::ranges::find(presetCache_, currentPresetFile_);
    ptrdiff_t index = (it != presetCache_.end()) ? std::distance(presetCache_.begin(), it) : -1;

    if (index == -1) {
      index = (direction > 0) ? 0 : (ptrdiff_t)presetCache_.size() - 1;
    } else {
      index += direction;
      if (index < 0) {
        index = (ptrdiff_t)presetCache_.size() - 1;
      } else if (index >= (ptrdiff_t)presetCache_.size()) {
        index = 0;
      }
    }

    loadPreset(presetCache_[(size_t)index]);
  }

  FileMenu::Options getPresetMenuOptions() {
    FileMenu::Options opts;
    opts.rootDir = getPresetRoot();
    opts.targetExtension = ".xml";
    opts.currentActiveFile = currentPresetFile_;
    opts.targetComponent = this;
    juce::Component::SafePointer<PresetManager> safeThis(this);

    opts.onMenuDismissed = [safeThis]() {
      if (safeThis != nullptr) { safeThis->repaint(); }
    };

    opts.onFileSelected = [safeThis](const juce::File& p) {
      if (safeThis != nullptr) { safeThis->loadPreset(p); }
    };

    opts.injectHeaderItems = [this](juce::PopupMenu& menu, const juce::File& currentDir) {
      using Item = juce::PopupMenu::Item;
      if (currentDir == getPresetRoot()) {
        menu.addSectionHeader("Preset");
        menu.addItem(Item("Save").setID(1).setAction([this] { savePreset(); }));
        menu.addItem(Item("Load").setID(2).setAction([this] { loadPresetFromFileChooser(); }));
        menu.addItem(Item("Refresh").setID(3).setAction([this] { presetCache_.clear(); }));
        menu.addSeparator();
      } else {
        menu.addSectionHeader(currentDir.getFileName());
        auto parentDir = currentDir.getParentDirectory();
        menu.addItem(Item(".. (Back)").setID(4).setAction([this, p = parentDir] {
          FileMenu::showDrillDown(p, getPresetMenuOptions());
        }));
        menu.addSeparator();
      }
    };

    return opts;
  }

  void openMenu() { FileMenu::showDrillDown(getPresetRoot(), getPresetMenuOptions()); }

public:
  PresetManager(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
                juce::UndoManager* undoManager, juce::AudioProcessorValueTreeState& tree)
      : editor_(editor), tree_(tree), undoManager_(undoManager), pal_(palette),
        statusBar_(statusBar) {
    editor_.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::showMenu, [this]() { openMenu(); });
    actions.addAction(juce::AccessibilityActionType::press, [this]() { openMenu(); });
    return std::make_unique<PresetManagerAccessibilityHandler>(*this, std::move(actions));
  }

  void resized() override {
    const int width = getWidth();
    const int height = getHeight();
    const int bw = width >= 3 * height ? height : width / 3;
    previousButtonRegion_.setBounds(0, 0, bw, height);
    nextButtonRegion_.setBounds(width - bw, 0, bw, height);
    textRegion_.setBounds(bw, 0, width - 2 * bw, height);
  }

  void paint(juce::Graphics& ctx) override {
    const float lw = pal_.borderWidth();
    const float corner = float(2) * lw;
    const float height = float(getHeight());
    const auto bounds = getLocalBounds().toFloat().reduced(lw * float(0.5));

    juce::Colour bgColour = pal_.surface();
    ctx.setColour(bgColour);
    ctx.fillRoundedRectangle(bounds, corner);

    // Track specific hover states
    bool prevHovered = isMouseEntered_ && previousButtonRegion_.contains(mousePosition_);
    bool nextHovered = isMouseEntered_ && nextButtonRegion_.contains(mousePosition_);
    bool textHovered = isMouseEntered_ && textRegion_.contains(mousePosition_);

    // Highlight
    if (isMouseEntered_) {
      ctx.setColour(pal_.main());
      if (prevHovered) {
        ctx.fillRoundedRectangle(previousButtonRegion_.toFloat(), corner);
      } else if (nextHovered) {
        ctx.fillRoundedRectangle(nextButtonRegion_.toFloat(), corner);
      } else if (textHovered) {
        ctx.fillRoundedRectangle(textRegion_.toFloat(), corner);
      }
    }

    ctx.setColour(pal_.border());
    ctx.drawRoundedRectangle(bounds, corner, lw);

    ctx.setFont(pal_.getFont(TextSize::normal));
    juce::Colour textBgColour = textHovered ? pal_.main() : bgColour;
    ctx.setColour(pal_.getForeground(textBgColour));
    ctx.drawText(text_, textRegion_, juce::Justification::centred);

    // Draw Left Arrow
    juce::Path leftArrow;
    leftArrow.startNewSubPath(float(0), float(0.5));
    leftArrow.lineTo(float(1), float(0));
    leftArrow.lineTo(float(0.62), float(0.5));
    leftArrow.lineTo(float(1), float(1));
    leftArrow.closeSubPath();

    auto prevRect = previousButtonRegion_.toFloat();
    leftArrow.scaleToFit(prevRect.getX(), float(height / 4), prevRect.getWidth(), float(height / 2),
                         true);

    // Evaluate arrow color against its specific background state
    ctx.setColour(pal_.getForeground(prevHovered ? pal_.main() : bgColour));
    ctx.fillPath(leftArrow);

    // Draw Right Arrow
    juce::Path rightArrow;
    rightArrow.startNewSubPath(float(1), float(0.5));
    rightArrow.lineTo(float(0), float(0));
    rightArrow.lineTo(float(0.38), float(0.5));
    rightArrow.lineTo(float(0), float(1));
    rightArrow.closeSubPath();

    auto nextRect = nextButtonRegion_.toFloat();
    rightArrow.scaleToFit(nextRect.getX(), float(height / 4), nextRect.getWidth(),
                          float(height / 2), true);

    // Evaluate arrow color against its specific background state
    ctx.setColour(pal_.getForeground(nextHovered ? pal_.main() : bgColour));
    ctx.fillPath(rightArrow);
  }

  void mouseDown(const juce::MouseEvent& event) override {
    auto position = event.getPosition();
    if (previousButtonRegion_.contains(position)) {
      cyclePreset(-1); // Prev
    } else if (nextButtonRegion_.contains(position)) {
      cyclePreset(1); // Next
    } else {
      openMenu();
    }
  }

  void mouseMove(const juce::MouseEvent& event) override {
    mousePosition_ = event.getPosition();
    updateStatusBar(mousePosition_);
    repaint();
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    isMouseEntered_ = true;
    updateStatusBar(event.getPosition());
    repaint();
  }
  void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered_ = false;
    statusBar_.clear();
    repaint();
  }

  bool keyPressed(const juce::KeyPress& key) override {
    if (key == juce::KeyPress::leftKey) {
      cyclePreset(-1);
      return true;
    }
    if (key == juce::KeyPress::rightKey) {
      cyclePreset(1);
      return true;
    }
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey) {
      openMenu();
      return true;
    }
    return false;
  }
};

} // namespace Uhhyou
