// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "../common/librarylicense.hpp"
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
#define PRM(id, scale) HEAD, PARAMETER(id), SCALE(scale)

namespace Uhhyou {

constexpr int defaultWidth = 6 * 100 + 18 * 5;
constexpr int defaultHeight = 18 * 30 + (8 - 2) * 5;

inline juce::File getPresetDirectory(const juce::AudioProcessor &processor)
{
  auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                  .getFullPathName();
  auto sep = juce::File::getSeparatorString();

  juce::File presetDir(appDir + sep + "Audiobulb" + sep + processor.getName());
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

  , preDriveGain(PRM("preDriveGain", gain), 5)
  , postDriveGain(PRM("postDriveGain", gain), 5)

  , overDriveType(
      PRM("overDriveType", overDriveType),
      {"Immediate", "Matched", "BadLimiter", "PolyDrive"})
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

  , pluginNameButton(
      *this, palette, processor.getName(), informationText, libraryLicenseText)
  , undoButton(
      *this,
      palette,
      "Undo",
      [&]() {
        if (processor.undoManager.canUndo()) processor.undoManager.undo();
      })
  , redoButton(
      *this,
      palette,
      "Redo",
      [&]() {
        if (processor.undoManager.canRedo()) processor.undoManager.redo();
      })
  , randomizeButton(
      *this,
      palette,
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
  , presetManager(HEAD, processor.param.tree)
  , oversamplingAttachment(
      *PARAMETER("oversampling"),
      [&](float) { processor.setLatencySamples(int(processor.dsp.getLatency())); },
      nullptr)
  , limiterEnabledAttachment(
      *PARAMETER("limiterEnabled"),
      [&](float) { processor.setLatencySamples(int(processor.dsp.getLatency())); },
      nullptr)
{
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
  auto groupLabelMarginWidth = groupLabelFont.getStringWidthFloat("W");
  for (const auto &x : groupLabels) {
    x.paint(ctx, groupLabelFont, 2 * palette.borderThin(), groupLabelMarginWidth);
  }
}

void Editor::resized()
{
  using Rect = juce::Rectangle<int>;
  using PointF = juce::Point<float>;

  const float scale = getDesktopScaleFactor() * getHeight() / float(defaultHeight);
  palette.resize(scale);

  lines.clear();
  labels.clear();
  groupLabels.clear();

  const int margin = int(5 * scale);
  const int labelHeight = int(20 * scale);
  const int labelWidth = int(100 * scale);

  const int uiMargin = 4 * margin;
  const int labelX = labelWidth + 2 * margin;
  const int labelY = labelHeight + 2 * margin;
  const int sectionWidth = 2 * labelWidth + 2 * margin;

  const int top0 = uiMargin;
  const int left0 = uiMargin;
  const int left1 = left0 + 1 * labelX;
  const int left2 = left0 + 2 * labelX;
  const int left3 = left0 + 3 * labelX;

  const int driveTop0 = top0;
  const int driveTop1 = driveTop0 + 1 * labelY;
  const int driveTop2 = driveTop0 + 2 * labelY;
  const int driveTop3 = driveTop0 + 3 * labelY;
  const int driveTop4 = driveTop0 + 4 * labelY;
  const int driveTop5 = driveTop0 + 5 * labelY;
  const int driveTop6 = driveTop0 + 6 * labelY;
  const int driveLeft0 = left0;
  const int driveLeft1 = left1;
  groupLabels.push_back(
    {"Drive", Rect{driveLeft0, driveTop0, sectionWidth, labelHeight}});

  labels.push_back(
    {"Pre Gain [dB]", Rect{driveLeft0, driveTop1, labelWidth, labelHeight}});
  preDriveGain.setBounds(Rect{driveLeft1, driveTop1, labelWidth, labelHeight});

  labels.push_back({"Output [dB]", Rect{driveLeft0, driveTop2, labelWidth, labelHeight}});
  postDriveGain.setBounds(Rect{driveLeft1, driveTop2, labelWidth, labelHeight});

  labels.push_back({"Type", Rect{driveLeft0, driveTop3, labelWidth, labelHeight}});
  overDriveType.setBounds(Rect{driveLeft1, driveTop3, labelWidth, labelHeight});

  labels.push_back({"Hold [s]", Rect{driveLeft0, driveTop4, labelWidth, labelHeight}});
  overDriveHoldSecond.setBounds(Rect{driveLeft1, driveTop4, labelWidth, labelHeight});

  labels.push_back({"Q", Rect{driveLeft0, driveTop5, labelWidth, labelHeight}});
  overDriveQ.setBounds(Rect{driveLeft1, driveTop5, labelWidth, labelHeight});

  labels.push_back({"Character", Rect{driveLeft0, driveTop6, labelWidth, labelHeight}});
  overDriveCharacterAmp.setBounds(Rect{driveLeft1, driveTop6, labelWidth, labelHeight});

  const int asymTop0 = driveTop6 + labelY;
  const int asymTop1 = asymTop0 + 1 * labelY;
  const int asymTop2 = asymTop0 + 2 * labelY;
  const int asymTop3 = asymTop0 + 3 * labelY;
  const int asymTop4 = asymTop0 + 4 * labelY;
  const int asymLeft0 = left0;
  const int asymLeft1 = left1;
  asymDriveEnabled.setBounds(Rect{asymLeft0, asymTop0, sectionWidth, labelHeight});

  labels.push_back({"Decay [s]", Rect{asymLeft0, asymTop1, labelWidth, labelHeight}});
  asymDriveDecaySecond.setBounds(Rect{asymLeft1, asymTop1, labelWidth, labelHeight});

  labels.push_back({"Bias", Rect{asymLeft0, asymTop2, labelWidth, labelHeight}});
  asymDriveDecayBias.setBounds(Rect{asymLeft1, asymTop2, labelWidth, labelHeight});

  labels.push_back({"Q", Rect{asymLeft0, asymTop3, labelWidth, labelHeight}});
  asymDriveQ.setBounds(Rect{asymLeft1, asymTop3, labelWidth, labelHeight});

  labels.push_back({"Character", Rect{asymLeft0, asymTop4, labelWidth, labelHeight}});
  asymExponentRange.setBounds(Rect{asymLeft1, asymTop4, labelWidth, labelHeight});

  const int limiterTop0 = asymTop4 + labelY;
  const int limiterTop1 = limiterTop0 + 1 * labelY;
  const int limiterTop2 = limiterTop0 + 2 * labelY;
  const int limiterLeft0 = left0;
  const int limiterLeft1 = left1;
  limiterEnabled.setBounds(Rect{limiterLeft0, limiterTop0, sectionWidth, labelHeight});

  labels.push_back(
    {"Pre Gain [dB]", Rect{limiterLeft0, limiterTop1, labelWidth, labelHeight}});
  limiterInputGain.setBounds(Rect{limiterLeft1, limiterTop1, labelWidth, labelHeight});

  labels.push_back(
    {"Release [s]", Rect{limiterLeft0, limiterTop2, labelWidth, labelHeight}});
  limiterReleaseSecond.setBounds(
    Rect{limiterLeft1, limiterTop2, labelWidth, labelHeight});

  const int miscTop0 = top0;
  const int miscTop1 = miscTop0 + 1 * labelY;
  const int miscTop2 = miscTop0 + 2 * labelY;
  const int miscLeft0 = left2;
  const int miscLeft1 = left3;
  groupLabels.push_back({"Misc.", Rect{miscLeft0, miscTop0, sectionWidth, labelHeight}});

  labels.push_back({"Smoothing [s]", Rect{miscLeft0, miscTop1, labelWidth, labelHeight}});
  parameterSmoothingSecond.setBounds(Rect{miscLeft1, miscTop1, labelWidth, labelHeight});

  labels.push_back({"Oversampling", Rect{miscLeft0, miscTop2, labelWidth, labelHeight}});
  oversampling.setBounds(Rect{miscLeft1, miscTop2, labelWidth, labelHeight});

  const int actionTop0 = miscTop2 + labelY;
  const int actionTop1 = actionTop0 + 1 * labelY;
  const int actionTop2 = actionTop0 + 2 * labelY;
  const int actionLeft0 = left2;
  const int actionLeft1 = left3;
  groupLabels.push_back(
    {"Action", Rect{actionLeft0, actionTop0, sectionWidth, labelHeight}});

  undoButton.setBounds(Rect{actionLeft0, actionTop1, labelWidth, labelHeight});
  redoButton.setBounds(Rect{actionLeft1, actionTop1, labelWidth, labelHeight});

  randomizeButton.setBounds(Rect{actionLeft0, actionTop2, sectionWidth, labelHeight});

  const int presetTop0 = actionTop2 + labelY;
  const int presetTop1 = presetTop0 + 1 * labelY;
  const int presetLeft0 = left2;
  groupLabels.push_back(
    {"Preset", Rect{presetLeft0, presetTop0, sectionWidth, labelHeight}});

  presetManager.setBounds(Rect{presetLeft0, presetTop1, sectionWidth, labelHeight});

  // Plugin name.
  pluginNameButton.setBounds(
    Rect{presetLeft0, presetTop1 + labelY, sectionWidth, labelHeight});
  pluginNameButton.scale(scale);
}

} // namespace Uhhyou
