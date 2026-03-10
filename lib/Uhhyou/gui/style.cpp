// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

// This source is splitted because nlohmann/json.hpp is slow to compile.

#include "style.hpp"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace Uhhyou {

namespace fs = std::filesystem;

// Shared mutex to synchronize file I/O across all plugin instances globally.
static std::mutex styleMutex;

/**
Specification of $XDG_CONFIG_HOME:
https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
*/
inline fs::path getConfigHome() {
#ifdef _WIN32
  char* appdataDir = nullptr;
  size_t size = 0;
  if (_dupenv_s(&appdataDir, &size, "AppData") == 0 && appdataDir != nullptr) {
    auto path = fs::path(appdataDir);
    free(appdataDir);
    return path;
  }

  juce::Logger::writeToLog("Uhhyou: %AppData% is empty.");
#elif __APPLE__
  const char* home = std::getenv("HOME");
  if (home != nullptr) { return fs::path(home) / "Library/Preferences"; }

  juce::Logger::writeToLog("Uhhyou: $HOME is empty.");
#else
  const char* configDir = std::getenv("XDG_CONFIG_HOME");
  if (configDir != nullptr) { return fs::path(configDir); }

  const char* home = std::getenv("HOME");
  if (home != nullptr) { return fs::path(home) / ".config"; }

  juce::Logger::writeToLog("Uhhyou: $XDG_CONFIG_HOME and $HOME are empty.");

#endif
  return fs::path("");
}

/**
data[key] must come in string of hex color code. "#123456", "#aabbccdd" etc.
Color will be only loaded if the size of string is either 7 or 9 (RGB or RGBA).
First character is ignored. So "!303030", " 0000ff88" are valid.
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
    if (ec != std::errc{}) {
      return 0; // Fallback to 0 if parsing fails
    }
    return result;
  };

  color = juce::Colour(strHexToUInt8(hex.substr(1, 2)), strHexToUInt8(hex.substr(3, 2)),
                       strHexToUInt8(hex.substr(5, 2)),
                       hex.size() != 9 ? juce::uint8(255) : strHexToUInt8(hex.substr(7, 2)));
}

inline void loadString(const nlohmann::ordered_json& data, const std::string& key,
                       juce::String& value) {
  if (!data.contains(key)) { return; }
  if (!data[key].is_string()) { return; }

  std::string loaded = data[key];
  if (loaded.empty()) { return; }

  value = loaded;
}

inline std::string colorToHex(const juce::Colour& c) {
  return juce::String::formatted("#%02x%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue(),
                                 c.getAlpha())
    .toStdString();
}

void Palette::load() {
  std::lock_guard<std::mutex> lock(styleMutex);

  fs::path home = getConfigHome();
  if (home.empty()) { return; }

  auto styleDir = home / fs::path("UhhyouPlugins/style");
  auto styleJsonPath = styleDir / "style.json";

  // Single source of truth for config mappings
  auto visitProperties = [&](auto&& visitor) {
    visitor("keyboardFocusEnabled", _keyboardFocusEnabled);
    visitor("windowScale", _windowScale);

    visitor("fontUiName", _fontUiName);
    visitor("fontUiStyle", _fontUiStyle);
    visitor("fontMonoName", _fontMonoName);
    visitor("fontMonoStyle", _fontMonoStyle);

    visitor("foreground", _foreground);
    visitor("background", _background);
    visitor("surface", _surface);
    visitor("border", _border);
    visitor("main", _main);
    visitor("accent", _accent);
    visitor("warning", _warning);
  };

  // Helper lambda to attempt loading the JSON (early returns on failure)
  auto tryLoad = [&]() -> std::optional<nlohmann::ordered_json> {
    if (!fs::exists(styleJsonPath)) { return std::nullopt; }

    std::ifstream ifs(styleJsonPath);
    if (!ifs.is_open()) {
      juce::Logger::writeToLog("Uhhyou: Failed to open existing style.json for reading.");
      return std::nullopt;
    }

    try {
      nlohmann::ordered_json data;
      ifs >> data;
      return data;
    } catch (const nlohmann::ordered_json::parse_error& e) {
      juce::Logger::writeToLog(juce::String("Uhhyou: Failed to parse style.json: ") + e.what());
      return std::nullopt;
    }
  };

  auto saveDefault = [&]() {
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

    std::error_code ec;
    fs::create_directories(styleDir, ec);
    if (ec && !fs::is_directory(styleDir)) {
      juce::Logger::writeToLog("Uhhyou: Failed to create directories: "
                               + juce::String(ec.message()));
      return;
    }

    auto tmpPath = styleJsonPath;
    tmpPath += ".tmp";

    std::ofstream ofs(tmpPath);
    if (!ofs.is_open()) {
      juce::Logger::writeToLog("Uhhyou: Failed to open tmp file for writing: "
                               + juce::String(tmpPath.string()));
      return;
    }

    ofs << data.dump(2) << "\n";
    ofs.close();

    fs::rename(tmpPath, styleJsonPath, ec);
    if (ec) {
      juce::Logger::writeToLog("Uhhyou: Failed to finalize style.json rename: "
                               + juce::String(ec.message()));
    }
  };

  // Main flow
  if (auto loadedData = tryLoad()) {
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
    saveDefault();
  }
}

template<typename T> inline void updateSettingImpl(const std::string& key, T value) {
  fs::path home = getConfigHome();
  if (home.empty()) { return; }

  auto styleDir = home / fs::path("UhhyouPlugins/style");
  auto styleJsonPath = styleDir / "style.json";

  nlohmann::ordered_json data;

  if (fs::exists(styleJsonPath)) {
    std::ifstream ifs(styleJsonPath);
    if (ifs.is_open()) {
      try {
        ifs >> data;
      } catch (const nlohmann::ordered_json::parse_error& e) {
        juce::Logger::writeToLog("Uhhyou: Refusing to update setting. style.json is corrupted: "
                                 + juce::String(e.what()));
        return;
      }
    }
  }

  data[key] = value;

  std::error_code ec;
  fs::create_directories(styleDir, ec);
  if (ec && !fs::is_directory(styleDir)) {
    juce::Logger::writeToLog("Uhhyou: Failed to create directories: " + juce::String(ec.message()));
    return;
  }

  auto tmpPath = styleJsonPath;
  tmpPath += ".tmp";

  std::ofstream ofs(tmpPath);
  if (!ofs.is_open()) {
    juce::Logger::writeToLog("Uhhyou: Failed to open tmp file for writing: "
                             + juce::String(tmpPath.string()));
    return;
  }

  ofs << data.dump(2) << "\n";
  ofs.close();

  fs::rename(tmpPath, styleJsonPath, ec);
  if (ec) {
    juce::Logger::writeToLog("Uhhyou: Failed to finalize style.json rename: "
                             + juce::String(ec.message()));
  }
}

void Palette::updateSetting(const std::string& key, bool value) {
  std::lock_guard<std::mutex> lock(styleMutex);
  if (key == "keyboardFocusEnabled") { _keyboardFocusEnabled = value; }
  updateSettingImpl(key, value);
}

void Palette::updateSetting(const std::string& key, float value) {
  std::lock_guard<std::mutex> lock(styleMutex);
  if (key == "windowScale") { _windowScale = value; }
  updateSettingImpl(key, value);
}

} // namespace Uhhyou
