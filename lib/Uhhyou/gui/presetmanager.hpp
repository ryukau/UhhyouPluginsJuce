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
    PresetManagerAccessibilityHandler(PresetManager& pm, juce::AccessibilityActions actions)
        : juce::AccessibilityHandler(pm, juce::AccessibilityRole::comboBox, std::move(actions)),
          presetManager(pm) {}

    juce::String getTitle() const override { return "Preset: " + presetManager.text; }

    juce::String getHelp() const override {
      return "Press Space or Enter to open the preset menu. Use Left and Right arrow keys to cycle "
             "presets.";
    }

  private:
    PresetManager& presetManager;
  };

  juce::AudioProcessorEditor& editor;
  juce::AudioProcessorValueTreeState& tree;
  juce::UndoManager* undoManager = nullptr;

  Palette& pal;
  StatusBar& statusBar;
  juce::Font font;
  juce::String text{"Default"};

  std::unique_ptr<juce::FileChooser> fileChooser;

  fs::path currentPresetPath;
  std::vector<fs::path> presetCache;

  bool isMouseEntered = false;
  juce::Point<int> mousePosition;

  juce::Rectangle<int> previousButtonRegion;
  juce::Rectangle<int> nextButtonRegion;
  juce::Rectangle<int> textRegion;

  void updateStatusBar(juce::Point<int> position) {
    if (previousButtonRegion.contains(position)) {
      statusBar.setText("Previous preset");
    } else if (nextButtonRegion.contains(position)) {
      statusBar.setText("Next preset");
    } else {
      statusBar.setText("Preset menu");
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
                       .getChildFile(editor.processor.getName());

    if (!presetDir.isDirectory()) {
      if (presetDir.createDirectory().failed()) { setPresetText("Error: FS Init Failed"); }
    }
    return toStdPath(presetDir);
  }

  void setPresetText(const juce::String& newText) {
    if (text == newText) { return; }
    text = newText;
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

  void savePreset() {
    auto xmlString = tree.copyState().toXmlString();
    auto rootJuce = toJuceFile(getPresetRoot());

    fileChooser = std::make_unique<juce::FileChooser>("Save Preset", rootJuce, "*.xml");

    auto callback = [this, xmlString](const juce::FileChooser& chooser) {
      auto file = chooser.getResult();
      if (file == juce::File()) { return; }

      if (file.replaceWithText(xmlString, false, false, "\n")) {
        currentPresetPath = toStdPath(file);

        // Invalidate cache so the new file appears in Next/Prev sequence
        presetCache.clear();

        setPresetText(file.getFileNameWithoutExtension());
        return;
      }

      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Save Failed",
                                             "Could not save file: " + file.getFileName());
    };

    using FBC = juce::FileBrowserComponent;
    fileChooser->launchAsync(FBC::saveMode | FBC::warnAboutOverwriting, callback);
  }

  void loadPresetFromFileChooser() {
    fileChooser
      = std::make_unique<juce::FileChooser>("Load Preset", toJuceFile(getPresetRoot()), "*.xml");

    using FBC = juce::FileBrowserComponent;
    fileChooser->launchAsync(FBC::openMode | FBC::canSelectFiles,
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

    if (xmlState == nullptr || !xmlState->hasTagName(tree.state.getType())) {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Load Failed",
                                             "Invalid preset file.");
      return;
    }

    tree.state.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*xmlState), undoManager);

    currentPresetPath = path;

    // If we loaded a file that isn't in our current cache, the cache is likely stale.
    // However, for performance, we don't rebuild immediately. We wait for Next/Prev.
    setPresetText(juceFile.getFileNameWithoutExtension());
  }

  void ensureCacheLoaded() {
    if (!presetCache.empty()) { return; }

    auto root = getPresetRoot();
    std::error_code ec;
    if (!fs::exists(root, ec)) { return; }

    presetCache.clear();

    auto opts = fs::directory_options::skip_permission_denied;
    for (const auto& entry : fs::recursive_directory_iterator(root, opts, ec)) {
      if (entry.is_regular_file(ec) && entry.path().extension() == ".xml") {
        presetCache.push_back(entry.path());
      }
    }

    std::ranges::sort(presetCache);
  }

  void cyclePreset(int direction) {
    ensureCacheLoaded();
    if (presetCache.empty()) { return; }

    auto it = std::ranges::find(presetCache, currentPresetPath);
    ptrdiff_t index = -1;

    if (it != presetCache.end()) { index = std::distance(presetCache.begin(), it); }

    index += direction;

    if (index < 0) {
      index = (ptrdiff_t)presetCache.size() - 1;
    } else if (index >= (ptrdiff_t)presetCache.size()) {
      index = 0;
    }

    loadPreset(presetCache[(size_t)index]);
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
      menu.addItem(Item("Refresh").setID(3).setAction([this] { presetCache.clear(); }));
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
      if (!currentPresetPath.empty()) {
        auto rel = fs::relative(currentPresetPath, d, ec);
        if (!ec && !rel.empty() && rel.native()[0] != '.') { isParent = true; }
      }

      menu.addItem(d.filename().string() + " >", true, isParent, [this, d] { showPresetMenu(d); });
    }

    for (const auto& f : files) {
      menu.addItem(f.stem().string(), true, (f == currentPresetPath), [this, f] { loadPreset(f); });
    }

    menu.showMenuAsync(juce::PopupMenu::Options()
                         .withInitiallySelectedItem(dir == root ? 1 : 4)
                         .withTargetComponent(this),
                       [this](int) { repaint(); });
  }

public:
  PresetManager(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
                juce::UndoManager* undoManager, juce::AudioProcessorValueTreeState& tree)
      : editor(editor), tree(tree), undoManager(undoManager), pal(palette), statusBar(statusBar),
        font(juce::FontOptions{}) {
    // No scanning in constructor (Lazy)
    editor.addAndMakeVisible(*this, 0);
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
    font = pal.getFont(pal.textSizeUi());

    const int width = getWidth();
    const int height = getHeight();
    const int bw = width >= 3 * height ? height : width / 3;
    previousButtonRegion.setBounds(0, 0, bw, height);
    nextButtonRegion.setBounds(width - bw, 0, bw, height);
    textRegion.setBounds(bw, 0, width - 2 * bw, height);
  }

  void paint(juce::Graphics& ctx) override {
    const float lw = pal.borderThin();
    const float corner = float(2) * lw;
    const float height = float(getHeight());
    const auto bounds = getLocalBounds().toFloat().reduced(lw * float(0.5));

    ctx.setColour(pal.boxBackground());
    ctx.fillRoundedRectangle(bounds, corner);

    // Highlight
    if (isMouseEntered) {
      ctx.setColour(pal.highlightButton());
      if (previousButtonRegion.contains(mousePosition)) {
        ctx.fillRoundedRectangle(previousButtonRegion.toFloat(), corner);
      } else if (nextButtonRegion.contains(mousePosition)) {
        ctx.fillRoundedRectangle(nextButtonRegion.toFloat(), corner);
      } else if (textRegion.contains(mousePosition)) {
        ctx.fillRoundedRectangle(textRegion.toFloat(), corner);
      }
    }

    ctx.setColour(pal.border());
    ctx.drawRoundedRectangle(bounds, corner, lw);

    ctx.setFont(font);
    ctx.setColour(pal.foreground());
    ctx.drawText(text, textRegion, juce::Justification::centred);

    // Draw Arrows
    juce::Path leftArrow;
    leftArrow.startNewSubPath(float(0), float(0.5));
    leftArrow.lineTo(float(1), float(0));
    leftArrow.lineTo(float(0.62), float(0.5));
    leftArrow.lineTo(float(1), float(1));
    leftArrow.closeSubPath();

    auto prevRect = previousButtonRegion.toFloat();
    leftArrow.scaleToFit(prevRect.getX(), float(height / 4), prevRect.getWidth(), float(height / 2),
                         true);
    ctx.fillPath(leftArrow);

    juce::Path rightArrow;
    rightArrow.startNewSubPath(float(1), float(0.5));
    rightArrow.lineTo(float(0), float(0));
    rightArrow.lineTo(float(0.38), float(0.5));
    rightArrow.lineTo(float(0), float(1));
    rightArrow.closeSubPath();

    auto nextRect = nextButtonRegion.toFloat();
    rightArrow.scaleToFit(nextRect.getX(), float(height / 4), nextRect.getWidth(),
                          float(height / 2), true);
    ctx.fillPath(rightArrow);
  }

  void mouseDown(const juce::MouseEvent& event) override {
    auto position = event.getPosition();
    if (previousButtonRegion.contains(position)) {
      cyclePreset(-1); // Prev
    } else if (nextButtonRegion.contains(position)) {
      cyclePreset(1); // Next
    } else {
      showPresetMenu(getPresetRoot()); // Start menu at root
    }
  }

  void mouseMove(const juce::MouseEvent& event) override {
    mousePosition = event.getPosition();
    updateStatusBar(mousePosition);
    repaint();
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    isMouseEntered = true;
    updateStatusBar(event.getPosition());
    repaint();
  }
  void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    statusBar.clear();
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
