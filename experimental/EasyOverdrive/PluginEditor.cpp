// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Uhhyou/librarylicense.hpp"
#include "gui/popupinformationtext.hpp"

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

constexpr int defaultWidth = 6 * 100 + 18 * 5;
constexpr int defaultHeight = 18 * 30 + (8 - 2) * 5;

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
      *this, palette, processor.getName(), informationText, libraryLicenseText)
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

  , preDriveGain(PRM("preDriveGain", gain), 5)
  , postDriveGain(PRM("postDriveGain", gain), 5)

  , overDriveType(
      PRM("overDriveType", overDriveType),
      {"Immediate", "HardGate", "Spike", "SpikeCubic", "CutoffMod", "Matched",
       "BadLimiter", "PolyDrive"})
  , overDriveHoldSecond(PRM("overDriveHoldSecond", overDriveHoldSecond), 5)
  , overDriveQ(PRM("overDriveQ", filterQ), 5)
  , overDriveCharacterAmp(PRM("overDriveCharacterAmp", gain), 5)

  , asymDriveEnabled(PRM("asymDriveEnabled", boolean), "Asym. Drive")
  , asymDriveDecaySecond(PRM("asymDriveDecaySecond", envelopeSecond), 5)
  , asymDriveDecayBias(PRM("asymDriveDecayBias", asymDriveDecayBias), 5)
  , asymDriveQ(PRM("asymDriveQ", filterQ), 5)
  , asymExponentRange(PRM("asymExponentRange", asymExponentRange), 5)

  , limiterEnabled(PRM("limiterEnabled", boolean), "Limiter")
  , limiterInputGain(PRM("limiterInputGain", gain), 5)
  , limiterReleaseSecond(PRM("limiterReleaseSecond", envelopeSecond), 5)

  , oversampling(PRM("oversampling", oversampling), {"1x", "2x", "16x"})
  , parameterSmoothingSecond(PRM("parameterSmoothingSecond", parameterSmoothingSecond), 5)

  , oversamplingAttachment(
      *PARAMETER("oversampling"),
      [&](float) { processor.setLatencySamples(int(processor.dsp.getLatency())); },
      nullptr)
  , limiterEnabledAttachment(
      *PARAMETER("limiterEnabled"),
      [&](float) { processor.setLatencySamples(int(processor.dsp.getLatency())); },
      nullptr)
{
  setDefaultColor(lookAndFeel, palette);

  setResizable(true, false);

  constexpr double ratio = double(defaultWidth) / double(defaultHeight);
  getConstrainer()->setFixedAspectRatio(ratio);
  setSize(defaultWidth, defaultHeight);
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

  const int asymTop0 = layoutVerticalSection(
    labels, groupLabels, left0, top0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "Drive",
    {
      {"Pre Gain [dB]", preDriveGain},
      {"Output [dB]", postDriveGain},
      {"Type", overDriveType},
      {"Hold [s]", overDriveHoldSecond},
      {"Q", overDriveQ},
      {"Character", overDriveCharacterAmp},
    });

  const int limiterTop0 = layoutVerticalSection(
    labels, groupLabels, left0, asymTop0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "",
    {
      {"", asymDriveEnabled, LabeledWidget::expand},
      {"Pre Gain [dB]", asymDriveDecaySecond},
      {"Decay [s]", asymDriveDecayBias},
      {"Q", asymDriveQ},
      {"Character", asymExponentRange},
    });

  layoutVerticalSection(
    labels, groupLabels, left0, limiterTop0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "",
    {
      {"", limiterEnabled, LabeledWidget::expand},
      {"Pre Gain [dB]", limiterInputGain},
      {"Release [s]", limiterReleaseSecond},
    });

  const int actionTop0 = layoutVerticalSection(
    labels, groupLabels, left1, top0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, "Misc.",
    {
      {"Smoothing [s]", parameterSmoothingSecond},
      {"Oversampling", oversampling},
    });

  const int nameTop0 = layoutActionSection(
    groupLabels, left1, actionTop0, sectionWidth, labelWidth, labelWidth, labelX,
    labelHeight, labelY, undoButton, redoButton, randomizeButton, presetManager);

  statusBar.setBounds(
    Rect{left0, bottom - labelHeight - uiMargin, 2 * sectionWidth, labelHeight});

  pluginNameButton.setBounds(Rect{left1, nameTop0, sectionWidth, labelHeight});
  pluginNameButton.scale(scale);
}

} // namespace Uhhyou
