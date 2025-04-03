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

constexpr int defaultWidth = 2 * 210 + 2 * 20;
constexpr int defaultHeight = 10 * 30;

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

  , amType(
      PRM("amType", amType),
      {
        "Double Side-band (DSB)",
        "Upper Side-band (USB)",
        "Lower Side-band (LSB)",
        "DSB Upper AA",
        "DSB Full AA",
        "USB AA",
        "LSB AA",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
        "- Reserved -",
      })
  , swapCarriorAndModulator(PRM("swapCarriorAndModulator", boolean), "Swap Input")
  , carriorSideBandMix(PRM("carriorSideBandMix", unipolar), 5)
  , outputGain(PRM("outputGain", gain), 5)
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

  const int uiMargin = 4 * margin;
  const int labelX = labelWidth + 2 * margin;
  const int labelY = labelHeight + 2 * margin;
  const int sectionWidth = 2 * labelWidth + 2 * margin;

  const int top0 = uiMargin;
  const int left0 = uiMargin;
  const int left1 = left0 + 1 * labelX;
  const int left2 = left0 + 2 * labelX;
  const int left3 = left0 + 3 * labelX;

  const int eqTop0 = top0;
  const int eqTop1 = eqTop0 + 1 * labelY;
  const int eqTop2 = eqTop0 + 2 * labelY;
  const int eqTop3 = eqTop0 + 3 * labelY;
  const int eqTop4 = eqTop0 + 4 * labelY;
  const int eqLeft0 = left0;
  const int eqLeft1 = left1;
  groupLabels.push_back(
    {"Amplitude Modulator", Rect{eqLeft0, eqTop0, sectionWidth, labelHeight}});

  labels.push_back({"Type", Rect{eqLeft0, eqTop1, labelWidth, labelHeight}});
  amType.setBounds(Rect{eqLeft1, eqTop1, labelWidth, labelHeight});

  labels.push_back({"Side-band Mix", Rect{eqLeft0, eqTop2, labelWidth, labelHeight}});
  carriorSideBandMix.setBounds(Rect{eqLeft1, eqTop2, labelWidth, labelHeight});

  labels.push_back({"Output [dB]", Rect{eqLeft0, eqTop3, labelWidth, labelHeight}});
  outputGain.setBounds(Rect{eqLeft1, eqTop3, labelWidth, labelHeight});

  swapCarriorAndModulator.setBounds(Rect{eqLeft0, eqTop4, sectionWidth, labelHeight});

  const int actionTop0 = top0;
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

  addChildComponent(statusBar);
  statusBar.setBounds(
    Rect{left0, defaultHeight - labelHeight - uiMargin, 2 * sectionWidth, labelHeight});
}

} // namespace Uhhyou
