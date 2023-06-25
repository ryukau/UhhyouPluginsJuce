# Uhhyou Plugins on JUCE
Some audio plugins writtein on [JUCE](https://github.com/juce-framework/JUCE).

Currently, everything on this repository is experimental. Always render the result to audio file after using the plugins in this repository. Breaking changes might be introduced in future.

## Build Instruction
Following softwares are required.

- C++ compiler
  - [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) on Windows. ["Desktop development with C++"](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170#step-4---choose-workloads) package is required.
  - [Xcode](https://developer.apple.com/xcode/) on macOS.
  - `g++` on Linux. See [JUCE documentation](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) for other dependencies.
- [CMake](https://cmake.org/)
- [Git](https://git-scm.com/)

After installation, run following commands on terminal.

```bash
git clone --recursive $URL_of_this_repository
cd UhhyouPluginsJuce

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Plugins will be built into `build/JuceTestPlugin_artefacts/Release`.

## Directory Structure
- `.github/workflows` and `ci`: CI scripts for GitHub Actions.
- `common`: Common components across plugins.
- `lib`: External libraries.

Other directories are individual plugins.

## License
[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html). See `lib/README.md` for complete licenses which includes the ones from libraries.
