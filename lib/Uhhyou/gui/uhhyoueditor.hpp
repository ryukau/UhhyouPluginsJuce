// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/widgets.hpp"
#include "Uhhyou/librarylicense.hpp"

#include "PluginProcessor.h"

#include <array>
#include <vector>

namespace Uhhyou {

class UhhyouEditor : public juce::AudioProcessorEditor {
public:
  // JUCE related internals.
  Processor &processor;
  Palette palette;

  // Action items.
  NumberEditor numberEditor;
  PopUpButton pluginNameButton;
  ActionButton<> undoButton;
  ActionButton<> redoButton;
  ActionButton<> randomizeButton;
  std::unique_ptr<juce::FileChooser> fileChooser;
  PresetManager presetManager;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UhhyouEditor)

  UhhyouEditor(
    Processor &processor,
    const juce::String &informationText,
    std::function<void(void)> onRandomize)
    : AudioProcessorEditor(processor)
    , processor(processor)

    , pluginNameButton(
        *this, palette, processor.getName(), informationText, libraryLicenseText)
    , undoButton(
        *this,
        palette,
        numberEditor,
        "Undo",
        [&]() {
          if (processor.undoManager.canUndo()) processor.undoManager.undo();
        })
    , redoButton(
        *this,
        palette,
        numberEditor,
        "Redo",
        [&]() {
          if (processor.undoManager.canRedo()) processor.undoManager.redo();
        })
    , randomizeButton(*this, palette, numberEditor, "Randomize", onRandomize)
    , presetManager((*this), (palette), &(processor.undoManager), processor.param.tree)
  {
  }

  ~UhhyouEditor() {}

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    numberEditor.setVisible(false);
  }
};

} // namespace Uhhyou
