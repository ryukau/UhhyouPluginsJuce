// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "style.hpp"

#include <algorithm>
#include <filesystem>
#include <ranges>
#include <vector>

namespace Uhhyou {

namespace fs = std::filesystem;

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
  juce::Font font_;
  juce::String text_{"Default"};

  std::unique_ptr<juce::FileChooser> fileChooser_;

  fs::path currentPresetPath_;
  std::vector<fs::path> presetCache_;

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

  static fs::path toStdPath(const juce::File& f) {
    return fs::path(f.getFullPathName().toStdString());
  }

  static juce::File toJuceFile(const fs::path& p) {
    const auto u8 = p.u8string();
    return juce::File(juce::String(reinterpret_cast<const char*>(u8.c_str())));
  }

  fs::path getPresetRoot() {
    auto presetDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("UhhyouPlugins")
                       .getChildFile(editor_.processor.getName());

    if (!presetDir.isDirectory()) {
      if (presetDir.createDirectory().failed()) { setPresetText("Error: FS Init Failed"); }
    }
    return toStdPath(presetDir);
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
    auto rootJuce = toJuceFile(getPresetRoot());

    fileChooser_ = std::make_unique<juce::FileChooser>("Save Preset", rootJuce, "*.xml");

    auto callback = [this, xmlString](const juce::FileChooser& chooser) {
      auto file = chooser.getResult();
      if (file == juce::File()) { return; }

      if (file.replaceWithText(xmlString, false, false, "\n")) {
        currentPresetPath_ = toStdPath(file);

        // Invalidate cache so the new file appears in Next/Prev sequence
        presetCache_.clear();

        setPresetText(file.getFileNameWithoutExtension());
        return;
      }

      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Save Failed",
                                             "Could not save file: " + file.getFileName());
    };

    using FBC = juce::FileBrowserComponent;
    fileChooser_->launchAsync(FBC::saveMode | FBC::warnAboutOverwriting, callback);
  }

  void loadPresetFromFileChooser() {
    fileChooser_
      = std::make_unique<juce::FileChooser>("Load Preset", toJuceFile(getPresetRoot()), "*.xml");

    using FBC = juce::FileBrowserComponent;
    fileChooser_->launchAsync(FBC::openMode | FBC::canSelectFiles,
                              [&](const juce::FileChooser& chooser) {
                                auto f = chooser.getResult();
                                if (f.existsAsFile()) { loadPreset(toStdPath(f)); }
                              });
  }

  void loadPreset(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_regular_file(path, ec)) { return; }

    auto juceFile = toJuceFile(path);
    auto xmlState = juce::parseXML(juceFile);

    if (xmlState == nullptr || !xmlState->hasTagName(tree_.state.getType())) {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Load Failed",
                                             "Invalid preset file.");
      return;
    }

    tree_.state.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*xmlState), undoManager_);

    currentPresetPath_ = path;

    // If we loaded a file that isn't in our current cache, the cache is likely stale.
    // However, for performance, we don't rebuild immediately. We wait for Next/Prev.
    setPresetText(juceFile.getFileNameWithoutExtension());
  }

  void ensureCacheLoaded() {
    if (!presetCache_.empty()) { return; }

    auto root = getPresetRoot();
    std::error_code ec;
    if (!fs::exists(root, ec)) { return; }

    presetCache_.clear();

    auto opts = fs::directory_options::skip_permission_denied;
    for (const auto& entry : fs::recursive_directory_iterator(root, opts, ec)) {
      if (entry.is_regular_file(ec) && entry.path().extension() == ".xml") {
        presetCache_.push_back(entry.path());
      }
    }

    std::ranges::sort(presetCache_);
  }

  void cyclePreset(int direction) {
    ensureCacheLoaded();
    if (presetCache_.empty()) { return; }

    auto it = std::ranges::find(presetCache_, currentPresetPath_);
    ptrdiff_t index = -1;

    if (it != presetCache_.end()) { index = std::distance(presetCache_.begin(), it); }

    index += direction;

    if (index < 0) {
      index = (ptrdiff_t)presetCache_.size() - 1;
    } else if (index >= (ptrdiff_t)presetCache_.size()) {
      index = 0;
    }

    loadPreset(presetCache_[(size_t)index]);
  }

  void showPresetMenu(const fs::path& dir) {
    // Drill down menu is employed to achieve lazy loading.

    juce::PopupMenu menu;
    auto root = getPresetRoot();

    using Item = juce::PopupMenu::Item;

    if (dir == root) {
      menu.addSectionHeader("Preset");
      menu.addItem(Item("Save").setID(1).setAction([this] { savePreset(); }));
      menu.addItem(Item("Load").setID(2).setAction([this] { loadPresetFromFileChooser(); }));
      menu.addItem(Item("Refresh").setID(3).setAction([this] { presetCache_.clear(); }));
      menu.addSeparator();
    } else {
      menu.addSectionHeader(dir.filename().string());
      if (dir.has_parent_path()) {
        menu.addItem(Item(".. (Back)").setID(4).setAction([this, p = dir.parent_path()] {
          showPresetMenu(p);
        }));
      }
      menu.addSeparator();
    }

    std::vector<fs::path> subDirs;
    std::vector<fs::path> files;
    std::error_code ec;

    if (fs::exists(dir, ec) && fs::is_directory(dir, ec)) {
      for (const auto& entry :
           fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec))
      {
        if (entry.is_directory(ec)) {
          subDirs.push_back(entry.path());
        } else if (entry.is_regular_file(ec) && entry.path().extension() == ".xml") {
          files.push_back(entry.path());
        }
      }
    }

    std::ranges::sort(subDirs);
    std::ranges::sort(files);

    for (const auto& d : subDirs) {
      bool isParent = false;
      if (!currentPresetPath_.empty()) {
        auto rel = fs::relative(currentPresetPath_, d, ec);
        if (!ec && !rel.empty() && rel.native()[0] != '.') { isParent = true; }
      }

      menu.addItem(d.filename().string() + " >", true, isParent, [this, d] { showPresetMenu(d); });
    }

    for (const auto& f : files) {
      menu.addItem(f.stem().string(), true, (f == currentPresetPath_),
                   [this, f] { loadPreset(f); });
    }

    menu.showMenuAsync(juce::PopupMenu::Options()
                         .withInitiallySelectedItem(dir == root ? 1 : 4)
                         .withTargetComponent(this),
                       [this](int) { repaint(); });
  }

public:
  PresetManager(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
                juce::UndoManager* undoManager, juce::AudioProcessorValueTreeState& tree)
      : editor_(editor), tree_(tree), undoManager_(undoManager), pal_(palette),
        statusBar_(statusBar), font_(juce::FontOptions{}) {
    // No scanning in constructor (Lazy)
    editor_.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::showMenu,
                      [this]() { showPresetMenu(getPresetRoot()); });
    actions.addAction(juce::AccessibilityActionType::press,
                      [this]() { showPresetMenu(getPresetRoot()); });
    return std::make_unique<PresetManagerAccessibilityHandler>(*this, std::move(actions));
  }

  void resized() override {
    font_ = pal_.getFont(TextSize::normal);

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

    ctx.setColour(pal_.surface());
    ctx.fillRoundedRectangle(bounds, corner);

    // Highlight
    if (isMouseEntered_) {
      ctx.setColour(pal_.main());
      if (previousButtonRegion_.contains(mousePosition_)) {
        ctx.fillRoundedRectangle(previousButtonRegion_.toFloat(), corner);
      } else if (nextButtonRegion_.contains(mousePosition_)) {
        ctx.fillRoundedRectangle(nextButtonRegion_.toFloat(), corner);
      } else if (textRegion_.contains(mousePosition_)) {
        ctx.fillRoundedRectangle(textRegion_.toFloat(), corner);
      }
    }

    ctx.setColour(pal_.border());
    ctx.drawRoundedRectangle(bounds, corner, lw);

    ctx.setFont(font_);
    ctx.setColour(pal_.foreground());
    ctx.drawText(text_, textRegion_, juce::Justification::centred);

    // Draw Arrows
    juce::Path leftArrow;
    leftArrow.startNewSubPath(float(0), float(0.5));
    leftArrow.lineTo(float(1), float(0));
    leftArrow.lineTo(float(0.62), float(0.5));
    leftArrow.lineTo(float(1), float(1));
    leftArrow.closeSubPath();

    auto prevRect = previousButtonRegion_.toFloat();
    leftArrow.scaleToFit(prevRect.getX(), float(height / 4), prevRect.getWidth(), float(height / 2),
                         true);
    ctx.fillPath(leftArrow);

    juce::Path rightArrow;
    rightArrow.startNewSubPath(float(1), float(0.5));
    rightArrow.lineTo(float(0), float(0));
    rightArrow.lineTo(float(0.38), float(0.5));
    rightArrow.lineTo(float(0), float(1));
    rightArrow.closeSubPath();

    auto nextRect = nextButtonRegion_.toFloat();
    rightArrow.scaleToFit(nextRect.getX(), float(height / 4), nextRect.getWidth(),
                          float(height / 2), true);
    ctx.fillPath(rightArrow);
  }

  void mouseDown(const juce::MouseEvent& event) override {
    auto position = event.getPosition();
    if (previousButtonRegion_.contains(position)) {
      cyclePreset(-1); // Prev
    } else if (nextButtonRegion_.contains(position)) {
      cyclePreset(1); // Next
    } else {
      showPresetMenu(getPresetRoot()); // Start menu at root
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
      showPresetMenu(getPresetRoot());
      return true;
    }
    return false;
  }
};

} // namespace Uhhyou
