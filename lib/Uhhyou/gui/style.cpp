// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

// This source is splitted because nlohmann/json.hpp is slow to compile.

#include "style.hpp"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace Uhhyou {

static std::mutex styleMutex;

inline juce::File getInternalStyleDir() {
  juce::File configDir;

#if JUCE_LINUX
  if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
    configDir = juce::File(juce::CharPointer_UTF8(xdg));
  } else {
    configDir
      = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".config");
  }
#elif JUCE_MAC
  configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("Preferences");
#else
  configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#endif

  return configDir.getChildFile("UhhyouPlugins").getChildFile("style2");
}

juce::File Palette::getStyleDirectory() const { return getInternalStyleDir(); }

/**
data[key] must come in string of hex color code. "#123456", "#aabbccdd" etc.
*/
inline void loadColor(const nlohmann::ordered_json& data, const std::string& key,
                      juce::Colour& color) {
  if (!data.contains(key)) { return; }
  const auto& value = data[key];
  if (!value.is_string()) { return; }

  const std::string& hexRef = value.get_ref<const std::string&>();
  std::string_view hex = hexRef;
  if (hex.size() != 7 && hex.size() != 9) { return; }

  auto strHexToUInt8 = [](std::string_view sv) -> juce::uint8 {
    uint8_t result{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result, 16);
    if (ec != std::errc{}) { return 0; }
    return result;
  };

  color = juce::Colour(strHexToUInt8(hex.substr(1, 2)), strHexToUInt8(hex.substr(3, 2)),
                       strHexToUInt8(hex.substr(5, 2)),
                       hex.size() != 9 ? juce::uint8(255) : strHexToUInt8(hex.substr(7, 2)));
}

inline void loadString(const nlohmann::ordered_json& data, const std::string& key,
                       juce::String& value) {
  if (!data.contains(key) || !data[key].is_string()) { return; }
  std::string loaded = data[key];
  if (!loaded.empty()) { value = loaded; }
}

inline std::string colorToHex(const juce::Colour& c) {
  return juce::String::formatted("#%02x%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue(),
                                 c.getAlpha())
    .toStdString();
}

static void saveJson(const juce::File& file, const nlohmann::ordered_json& data) {
  file.getParentDirectory().createDirectory(); // ensure dir exists
  juce::String jsonStr = juce::String::fromUTF8((data.dump(2) + "\n").c_str());
  if (!file.replaceWithText(jsonStr)) {
    juce::Logger::writeToLog("Uhhyou: Failed to securely save JSON: " + file.getFullPathName());
  }
}

static std::optional<nlohmann::ordered_json> loadJson(const juce::File& file) {
  if (!file.existsAsFile()) { return std::nullopt; }

  std::unique_ptr<juce::FileInputStream> stream(file.createInputStream());
  if (stream == nullptr || stream->failedToOpen()) {
    juce::Logger::writeToLog("Uhhyou: Failed to open JSON file for reading: "
                             + file.getFullPathName());
    return std::nullopt;
  }

  juce::String content = stream->readEntireStreamAsString();
  if (content.isEmpty()) { return std::nullopt; }

  try {
    return nlohmann::ordered_json::parse(content.toStdString());
  } catch (const nlohmann::ordered_json::parse_error& e) {
    juce::Logger::writeToLog("Uhhyou: JSON Parse error in " + file.getFullPathName() + ": "
                             + juce::String(e.what()));
    return std::nullopt;
  }
}

void Palette::saveStyleJson() {
  juce::File styleDir = getInternalStyleDir();
  juce::File styleJsonPath = styleDir.getChildFile("style.json");

  nlohmann::ordered_json data;
  visitProperties([&](const char* key, auto& member) {
    using T = std::decay_t<decltype(member)>;
    if constexpr (std::is_same_v<T, juce::String>) {
      data[key] = member.toStdString();
    } else if constexpr (std::is_same_v<T, juce::Colour>) {
      data[key] = colorToHex(member);
    } else if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, float>) {
      data[key] = member;
    }
  });

  juce::InterProcessLock ipLock("UhhyouStyleLock");
  if (ipLock.enter(2000)) {
    saveJson(styleJsonPath, data);
  } else {
    juce::Logger::writeToLog("Uhhyou: Failed to acquire InterProcessLock for saving style.json");
  }
}

void Palette::loadStyleFromFile(const juce::File& file) {
  std::lock_guard<std::mutex> lock(styleMutex);

  auto loadedData = loadJson(file);
  if (!loadedData) { return; }

  visitProperties([&](const char* key, auto& member) {
    using T = std::decay_t<decltype(member)>;
    if constexpr (std::is_same_v<T, juce::String>) {
      loadString(*loadedData, key, member);
    } else if constexpr (std::is_same_v<T, juce::Colour>) {
      loadColor(*loadedData, key, member);
    } else if constexpr (std::is_same_v<T, bool>) {
      if (loadedData->contains(key) && (*loadedData)[key].is_boolean()) {
        member = (*loadedData)[key].get<bool>();
      }
    } else if constexpr (std::is_same_v<T, float>) {
      if (loadedData->contains(key) && (*loadedData)[key].is_number()) {
        member = (*loadedData)[key].get<float>();
      }
    }
  });

  updateCaches();
  saveStyleJson(); // Persist the freshly loaded config as the active `style.json`.
  StyleNotifier::getInstance().sendChangeMessage();
}

void Palette::load() {
  std::lock_guard<std::mutex> lock(styleMutex);
  juce::File styleJsonPath = getInternalStyleDir().getChildFile("style.json");

  std::optional<nlohmann::ordered_json> loadedData;
  {
    juce::InterProcessLock ipLock("UhhyouStyleLock");
    if (ipLock.enter(2000)) {
      loadedData = loadJson(styleJsonPath);
    } else {
      juce::Logger::writeToLog("Uhhyou: Failed to acquire InterProcessLock for loading style.json");
    }
  }

  if (loadedData) {
    visitProperties([&](const char* key, auto& member) {
      using T = std::decay_t<decltype(member)>;
      if constexpr (std::is_same_v<T, juce::String>) {
        loadString(*loadedData, key, member);
      } else if constexpr (std::is_same_v<T, juce::Colour>) {
        loadColor(*loadedData, key, member);
      } else if constexpr (std::is_same_v<T, bool>) {
        if (loadedData->contains(key) && (*loadedData)[key].is_boolean()) {
          member = (*loadedData)[key].get<bool>();
        }
      } else if constexpr (std::is_same_v<T, float>) {
        if (loadedData->contains(key) && (*loadedData)[key].is_number()) {
          member = (*loadedData)[key].get<float>();
        }
      }
    });
  } else {
    saveStyleJson();
  }

  updateCaches();
}

template<typename T> inline void updateSettingImpl(const std::string& key, T value) {
  juce::File styleJsonPath = getInternalStyleDir().getChildFile("style.json");

  juce::InterProcessLock ipLock("UhhyouStyleLock");
  if (!ipLock.enter(2000)) {
    juce::Logger::writeToLog("Uhhyou: Failed to acquire InterProcessLock. Aborting updateSetting.");
    return;
  }

  nlohmann::ordered_json data;

  if (styleJsonPath.existsAsFile()) {
    auto loadedData = loadJson(styleJsonPath);

    if (!loadedData) {
      juce::Logger::writeToLog("Uhhyou: Aborting updateSetting to prevent data wipe (could not "
                               "read existing style.json).");
      return;
    }
    data = *loadedData;
  }

  data[key] = value;
  saveJson(styleJsonPath, data);
}

void Palette::updateSetting(const std::string& key, bool value) {
  std::lock_guard<std::mutex> lock(styleMutex);
  if (key == "keyboardFocusEnabled") { keyboardFocusEnabled_ = value; }
  updateSettingImpl(key, value);
}

void Palette::updateSetting(const std::string& key, float value) {
  std::lock_guard<std::mutex> lock(styleMutex);
  if (key == "windowScale") { windowScale_ = value; }
  updateSettingImpl(key, value);
}

} // namespace Uhhyou
