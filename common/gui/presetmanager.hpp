// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "style.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <vector>

namespace Uhhyou {

class PresetManager : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)

protected:
  juce::AudioProcessorEditor &editor;
  juce::AudioProcessorValueTreeState &tree;
  juce::UndoManager *undoManager = nullptr;

  Palette &pal;

  juce::Font font;
  juce::String text{"Default"};

  juce::PopupMenu menu;
  std::unique_ptr<juce::FileChooser> fileChooser;

  int presetFileIndex = 0;
  juce::Array<juce::File> presetFile;

  bool isMouseEntered = false;
  juce::Point<int> mousePosition;

  juce::Rectangle<int> previousButtonRegion;
  juce::Rectangle<int> nextButtonRegion;
  juce::Rectangle<int> textRegion;

  juce::File getPresetDirectory()
  {
    auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getFullPathName();
    auto sep = juce::File::getSeparatorString();

    auto path = appDir + sep + "UhhyouPlugins" + sep + editor.processor.getName();
    juce::File presetDir(path);
    if (!(presetDir.exists() && presetDir.isDirectory())) {
      if (presetDir.createDirectory().failed()) {
        text = "Error: Failed to create preset directory. " + path;
      }
    }
    return presetDir;
  }

  void savePreset()
  {
    auto state = tree.copyState();
    auto xmlString = state.toXmlString();

    fileChooser
      = std::make_unique<juce::FileChooser>("Save Preset", getPresetDirectory(), "*.xml");

    using FBC = juce::FileBrowserComponent;
    fileChooser->launchAsync(
      FBC::saveMode | FBC::warnAboutOverwriting,
      [&, xmlString](const juce::FileChooser &chooser) {
        juce::File file(chooser.getResult());
        if (!file.replaceWithText(xmlString, false, false, "\n")) {
          text = "Error: Failed to write file.";
          return;
        }
        refreshPreset();
      });
  }

  void loadPresetFromFileChooser()
  {
    fileChooser
      = std::make_unique<juce::FileChooser>("Load Preset", getPresetDirectory(), "*.xml");

    using FBC = juce::FileBrowserComponent;
    fileChooser->launchAsync(
      FBC::openMode | FBC::canSelectFiles,
      [&](const juce::FileChooser &chooser) { loadPreset(chooser.getResult()); });
  }

  void loadPreset(const juce::File &file)
  {
    if (!file.existsAsFile()) return;

    auto xmlState = juce::XmlDocument::parse(file.loadFileAsString());
    if (xmlState.get() == nullptr) {
      text = "Error: Failed to parse XML.";
      return;
    }
    if (!xmlState->hasTagName(tree.state.getType())) {
      text = "Error: XML is missing some element.";
      return;
    }

    tree.state.copyPropertiesAndChildrenFrom(
      juce::ValueTree::fromXml(*xmlState), undoManager);

    text = file.getFileNameWithoutExtension();
  }

  void refreshPreset()
  {
    auto presetDir = getPresetDirectory();
    presetFile = presetDir.findChildFiles(
      juce::File::TypesOfFileToFind::findFiles, true, "*.xml",
      juce::File::FollowSymlinks::noCycles);

    if (presetFileIndex >= presetFile.size()) presetFileIndex = presetFile.size() - 1;
  }

  void selectPreviousPreset()
  {
    if (presetFile.size() < 0) return;
    if (--presetFileIndex < 0) presetFileIndex = presetFile.size() - 1;
    loadPreset(presetFile[presetFileIndex]);
  }

  void selectNextPreset()
  {
    if (presetFile.size() < 0) return;
    if (++presetFileIndex >= presetFile.size()) presetFileIndex = 0;
    loadPreset(presetFile[presetFileIndex]);
  }

public:
  PresetManager(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::AudioProcessorValueTreeState &tree)
    : editor(editor), tree(tree), undoManager(undoManager), pal(palette)
  {
    refreshPreset();

    editor.addAndMakeVisible(*this, 0);
  }

  virtual ~PresetManager() override {}

  virtual void resized() override
  {
    font = pal.getFont(pal.textSizeUi());

    const int width = getWidth();
    const int height = getHeight();
    const int bw = width >= 3 * height ? height : width / 3; // Button width.
    previousButtonRegion.setBounds(0, 0, bw, height);
    nextButtonRegion.setBounds(width - bw, 0, bw, height);
    textRegion.setBounds(bw, 0, width - 2 * bw, height);
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());

    // Background.
    ctx.setColour(pal.boxBackground());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    ctx.setColour(pal.border());
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    ctx.setFont(font);
    ctx.setColour(pal.foreground());
    ctx.drawText(text, textRegion, juce::Justification::centred);

    // Triangles.
    juce::Path leftArrow;
    leftArrow.startNewSubPath(float(0), float(0.5));
    leftArrow.lineTo(float(1), float(0));
    leftArrow.lineTo(float(0.62), float(0.5));
    leftArrow.lineTo(float(1), float(1));
    leftArrow.closeSubPath();
    const auto prevRect = previousButtonRegion.toFloat();
    leftArrow.scaleToFit(
      prevRect.getX(), float(height / 4), prevRect.getWidth(), float(height / 2), true);
    ctx.fillPath(leftArrow);

    juce::Path rightArrow;
    rightArrow.startNewSubPath(float(1), float(0.5));
    rightArrow.lineTo(float(0), float(0));
    rightArrow.lineTo(float(0.38), float(0.5));
    rightArrow.lineTo(float(0), float(1));
    rightArrow.closeSubPath();
    const auto nextRect = nextButtonRegion.toFloat();
    rightArrow.scaleToFit(
      nextRect.getX(), float(height / 4), nextRect.getWidth(), float(height / 2), true);
    ctx.fillPath(rightArrow);

    // Highlight.
    if (isMouseEntered) {
      ctx.setColour(pal.overlayHighlight());
      if (previousButtonRegion.contains(mousePosition)) {
        ctx.fillRoundedRectangle(previousButtonRegion.toFloat(), lw2);
      } else if (nextButtonRegion.contains(mousePosition)) {
        ctx.fillRoundedRectangle(nextButtonRegion.toFloat(), lw2);
      } else if (textRegion.contains(mousePosition)) {
        ctx.fillRoundedRectangle(textRegion.toFloat(), lw2);
      }
    }
  }

  virtual void mouseDown(const juce::MouseEvent &event) override
  {
    auto position = event.getPosition();
    if (previousButtonRegion.contains(position)) {
      selectPreviousPreset();
    } else if (nextButtonRegion.contains(position)) {
      selectNextPreset();
    } else {
      menu.clear();
      menu.addItem("Save", [&]() { savePreset(); });
      menu.addItem("Load", [&]() { loadPresetFromFileChooser(); });
      menu.addItem("Next", [&]() { selectNextPreset(); });
      menu.addItem("Previous", [&]() { selectPreviousPreset(); });
      menu.addItem("Refresh", [&]() { refreshPreset(); });
      menu.addSeparator();
      menu.addSectionHeader(text);
      menu.showMenuAsync(juce::PopupMenu::Options(), [&](int) { repaint(); });
    }

    repaint();
  }

  virtual void mouseUp(const juce::MouseEvent &) override {}

  virtual void mouseMove(const juce::MouseEvent &event) override
  {
    mousePosition = event.getPosition();
    repaint();
  }

  virtual void mouseDrag(const juce::MouseEvent &) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent &) override {}

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override
  {
  }

  virtual void mouseEnter(const juce::MouseEvent &) override
  {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent &) override
  {
    isMouseEntered = false;
    repaint();
  }
};

} // namespace Uhhyou
