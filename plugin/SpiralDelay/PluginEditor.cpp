// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "./gui/popupinformationtext.hpp"
#include "Uhhyou/librarylicense.hpp"

#include <array>
#include <random>
#include <string>

#define HEAD (*this), (palette), &(processor.undoManager)
#define PARAMETER(id) processor.param.tree.getParameter(id)
#define SCALE(name) processor.param.scale.name
#define VALUE(name) processor.param.value.name
#define TREE() processor.param.tree

// Parameter related arguments.
#define ACTION_BUTTON (*this), palette, statusBar, numberEditor
#define PRM(id, scale) HEAD, PARAMETER(id), SCALE(scale), statusBar, numberEditor

namespace Uhhyou {

constexpr int defaultWidth = 3 * 210 + 3 * 20;
constexpr int defaultHeight = 18 * 30;

inline juce::File getPresetDirectory(const juce::AudioProcessor &processor)
{
  auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                  .getFullPathName();
  auto sep = juce::File::getSeparatorString();

  juce::File presetDir(appDir + sep + "Uhhyou" + sep + processor.getName());
  if (!(presetDir.exists() && presetDir.isDirectory())) presetDir.createDirectory();
  return presetDir;
}

template<size_t nParameter>
inline auto constructParamArray(
  juce::AudioProcessorValueTreeState &tree, std::string baseName, size_t indexOffset = 0)
{
  std::array<juce::RangedAudioParameter *, nParameter> params;
  for (size_t idx = 0; idx < nParameter; ++idx) {
    params[idx] = tree.getParameter(baseName + std::to_string(idx + indexOffset));
  }
  return params;
}

Editor::Editor(Processor &processor)
  : AudioProcessorEditor(processor)
  , processor(processor)
  , statusBar(*this, palette)
  , numberEditor(palette)

  , pluginNameButton(
      *this, palette, processor.getName(), informationText, getLibraryLicenseText())
  , undoButton(
      ACTION_BUTTON,
      "Undo",
      [&]() {
        if (processor.undoManager.canUndo()) processor.undoManager.undo();
      })
  , redoButton(
      ACTION_BUTTON,
      "Redo",
      [&]() {
        if (processor.undoManager.canRedo()) processor.undoManager.redo();
      })
  , randomizeButton(
      ACTION_BUTTON,
      "Randomize",
      [&]() {
        std::uniform_real_distribution<float> dist{0.0f, 1.0f};
        std::random_device dev;
        std::mt19937 rng(dev());

        auto params = processor.getParameters();
        for (auto &prm : params) {
          prm->beginChangeGesture();
          prm->setValueNotifyingHost(dist(rng));
          prm->endChangeGesture();
        }
      })
  , presetManager((*this), (palette), &(processor.undoManager), processor.param.tree)

  , dryGain(PRM("dryGain", gain), 5)
  , wetGain(PRM("wetGain", gain), 5)
  , crossGain(PRM("crossGain", gain), 5)
  , saturationGain(PRM("saturationGain", saturationGain), 5)
  , delayTimeMs(PRM("delayTimeMs", delayTimeMs), 5)
  , delayTimeRatio(PRM("delayTimeRatio", unipolar), 5)
  , feedback0(PRM("feedback0", feedback), 5)
  , feedback1(PRM("feedback1", feedback), 5)
  , rotationHz(PRM("rotationHz", rotationHz), 5)
  , stereoPhaseOffset(PRM("stereoPhaseOffset", unipolar), 5)
  , rotationToDelayTimeOctave0(PRM("rotationToDelayTimeOctave0", timeModOctave), 5)
  , rotationToDelayTimeOctave1(PRM("rotationToDelayTimeOctave1", timeModOctave), 5)
  , notchMix(PRM("notchMix", unipolar), 5)
  , notchWidth(PRM("notchWidth", notchWidth), 5)
  , notchTracking(PRM("notchTracking", notchTracking), 5)
  , highpassCutoffHz(PRM("highpassCutoffHz", highpassCutoffHz), 5)
  , crossModulationOctave0(PRM("crossModulationOctave0", timeModOctave), 5)
  , crossModulationOctave1(PRM("crossModulationOctave1", timeModOctave), 5)
  , oversampling(PRM("oversampling", boolean), "2x Sampling")
{
  setDefaultColor(lookAndFeel, palette);

  setResizable(true, false);

  constexpr double ratio = double(defaultWidth) / double(defaultHeight);
  getConstrainer()->setFixedAspectRatio(ratio);
  const float scale = getStateTree().getProperty("scale", 1.0f);
  setSize(int(scale * defaultWidth), int(scale * defaultHeight));
}

Editor::~Editor() {}

void Editor::paint(juce::Graphics &ctx)
{
  ctx.setColour(palette.background());
  ctx.fillAll();

  ctx.setColour(palette.foreground());
  for (const auto &x : lines) x.paint(ctx);

  ctx.setFont(palette.getFont(palette.textSizeUi()));
  for (const auto &x : labels) x.paint(ctx);

  auto groupLabelFont = palette.getFont(palette.textSizeUi());
  auto groupLabelMarginWidth
    = juce::GlyphArrangement::getStringWidth(groupLabelFont, "W");
  for (const auto &x : groupLabels) {
    x.paint(ctx, groupLabelFont, 2 * palette.borderThin(), groupLabelMarginWidth);
  }
}

void Editor::resized()
{
  using Rect = juce::Rectangle<int>;

  const float scale = getDesktopScaleFactor() * getHeight() / float(defaultHeight);
  getStateTree().setProperty("scale", scale, nullptr);
  palette.resize(scale);

  lines.clear();
  labels.clear();
  groupLabels.clear();

  const int margin = int(5 * scale);
  const int labelHeight = int(20 * scale);
  const int labelWidth = int(100 * scale);
  const int bottom = int(scale * defaultHeight);

  const int uiMargin = 4 * margin;
  const int labelX = labelWidth + 2 * margin;
  const int labelY = labelHeight + 2 * margin;
  const int sectionWidth = 2 * labelWidth + 2 * margin;

  const int top0 = uiMargin;
  const int left0 = uiMargin;
  const int left1 = left0 + 2 * labelX;
  const int left2 = left1 + 2 * labelX;

  const int topUnderMix = layoutVerticalSection(
    labels, groupLabels, left0, top0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "Mix",
    {
      {"dryGain", dryGain},
      {"wetGain", wetGain},
      {"crossGain", crossGain},
      {"", oversampling},
    });

  layoutVerticalSection(
    labels, groupLabels, left0, topUnderMix, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "Saturation",
    {
      {"saturationGain", saturationGain},
    });

  const int topUnderDelay = layoutVerticalSection(
    labels, groupLabels, left1, top0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "Delay",
    {
      {"Time [ms]", delayTimeMs},
      {"Time Ratio", delayTimeRatio},
      {"Feedback 0", feedback0},
      {"Feedback 1", feedback1},
      {"Rotation [Hz]", rotationHz},
      {"Stereo Phase", stereoPhaseOffset},
      {"Notch Mix", notchMix},
      {"Notch Width", notchWidth},
      {"Notch Tracking", notchTracking},
      {"Highpass [Hz]", highpassCutoffHz},
    });

  layoutVerticalSection(
    labels, groupLabels, left1, topUnderDelay, sectionWidth, labelWidth, labelWidth,
    labelX, labelHeight, labelY, "Modulation",
    {
      {"Rot. Mod. 0 [oct.]", rotationToDelayTimeOctave0},
      {"Rot. Mod. 1 [oct.]", rotationToDelayTimeOctave1},
      {"In -> Time Mod. 0 [oct.]", crossModulationOctave0},
      {"In -> Time Mod. 1 [oct.]", crossModulationOctave1},
    });

  const int nameTop0 = layoutActionSection(
    groupLabels, left2, top0, sectionWidth, labelWidth, labelWidth, labelX, labelHeight,
    labelY, undoButton, redoButton, randomizeButton, presetManager);

  statusBar.setBounds(
    Rect{left0, bottom - labelHeight - uiMargin, 2 * sectionWidth, labelHeight});

  pluginNameButton.setBounds(Rect{left2, nameTop0, sectionWidth, labelHeight});
  pluginNameButton.scale(scale);
}

} // namespace Uhhyou
