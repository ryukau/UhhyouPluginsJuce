# Uhhyou Plugins on JUCE
Audio plugins written on [JUCE](https://github.com/juce-framework/JUCE).

- [Get the plugins (Release page)](https://github.com/ryukau/UhhyouPluginsJuce/releases)
- [Read the manuals (Documentations)](https://ryukau.github.io/UhhyouPluginsJuce)

## Build Instructions

<details>

Following tools are required.

- C++ compiler
  - [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) on Windows. ["Desktop development with C++"](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170#step-4---choose-workloads) package is required.
  - [Xcode](https://developer.apple.com/xcode/) on macOS.
  - `g++` on Linux. Refer to [JUCE documentation](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) for other required packages.
- [CMake](https://cmake.org/)
- [Git](https://git-scm.com/)

After the installations of the above tools, run following commands on your terminal.

```bash
git clone --recursive $URL_of_this_repository
cd UhhyouPluginsJuce

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Plugins will be built into following paths:

- `build/experimental/<PluginName>/<PluginName>_artefacts/Release/<VST3 or AU>`
- `build/plugins/<PluginName>/<PluginName>_artefacts/Release/<VST3 or AU>`

</details>

## Directory Structure

<details>

- `.github/workflows` and `ci`: CI scripts for GitHub Actions.
- `assets`: Factory presets and appearances.
- `docs`: User manuals. Hugo is used.
- `experimental`: Experimental plugins which may break backward compatibilities.
- `lib`: External libraries except `lib/Uhhyou`.
  - `lib/Uhhyou`: Internal library.
- `plugins`: Main plugins.
- `tools`: Development tools.

</details>

## License
AGPL-3.0-only to comply JUCE 8 license.

`lib/README.md` provides the licenses from the libraries used in this project.
