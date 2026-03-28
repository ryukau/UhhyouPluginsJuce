#pragma once

#include <algorithm>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace Uhhyou {

class FileMenu {
public:
  struct Options {
    juce::File rootDir;
    juce::String targetExtension;
    juce::File currentActiveFile;
    juce::Component* targetComponent = nullptr;

    std::function<void(const juce::File&)> onFileSelected;
    std::function<bool(const juce::File&)> fileFilter = nullptr;
    std::function<void(juce::PopupMenu&, const juce::File& currentDir)> injectHeaderItems = nullptr;
    std::function<void()> onMenuDismissed = nullptr;
  };

  static juce::PopupMenu buildHierarchical(const juce::File& targetDir, const Options& options) {
    juce::PopupMenu menu;
    setupBaseMenu(menu, targetDir, options, true);
    auto contents = scanAndSortDirectory(targetDir, options);
    for (const auto& d : contents.subDirs) {
      bool isParentOfActive
        = options.currentActiveFile.exists() && options.currentActiveFile.isAChildOf(d);
      menu.addSubMenu(d.getFileName(), buildHierarchical(d, options), true, nullptr,
                      isParentOfActive);
    }
    addFilesToMenu(menu, contents.files, options);
    return menu;
  }

  static void showDrillDown(const juce::File& targetDir, const Options& options) {
    juce::PopupMenu menu;
    setupBaseMenu(menu, targetDir, options, false);

    auto contents = scanAndSortDirectory(targetDir, options);
    for (const auto& d : contents.subDirs) {
      bool isParentOfActive
        = options.currentActiveFile.exists() && options.currentActiveFile.isAChildOf(d);
      menu.addItem(d.getFileName() + " >", true, isParentOfActive,
                   [d, options]() { showDrillDown(d, options); });
    }
    addFilesToMenu(menu, contents.files, options);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(options.targetComponent),
                       [options](int) {
                         if (options.onMenuDismissed) { options.onMenuDismissed(); }
                       });
  }

private:
  struct DirectoryContents {
    std::vector<juce::File> subDirs;
    std::vector<juce::File> files;
  };

  static void setupBaseMenu(juce::PopupMenu& menu, const juce::File& targetDir,
                            const Options& options, bool rootOnlyInjection) {
    if (options.targetComponent != nullptr) {
      menu.setLookAndFeel(&options.targetComponent->getLookAndFeel());
    }
    if (options.injectHeaderItems) {
      if (!rootOnlyInjection || targetDir == options.rootDir) {
        options.injectHeaderItems(menu, targetDir);
      }
    }
  }

  static DirectoryContents scanAndSortDirectory(const juce::File& targetDir,
                                                const Options& options) {
    DirectoryContents contents;

    if (targetDir.isDirectory()) {
      for (const juce::DirectoryEntry& entry : juce::RangedDirectoryIterator(
             targetDir, false, "*", juce::File::findFilesAndDirectories)) {
        auto f = entry.getFile();
        if (entry.isDirectory()) {
          contents.subDirs.push_back(f);
        } else if (f.hasFileExtension(options.targetExtension)) {
          if (!options.fileFilter || options.fileFilter(f)) { contents.files.push_back(f); }
        }
      }
    }

    auto naturalSort = [](const juce::File& a, const juce::File& b) {
      return a.getFileName().compareNatural(b.getFileName()) < 0;
    };

    std::ranges::sort(contents.subDirs, naturalSort);
    std::ranges::sort(contents.files, naturalSort);

    return contents;
  }

  static void addFilesToMenu(juce::PopupMenu& menu, const std::vector<juce::File>& files,
                             const Options& options) {
    for (const auto& f : files) {
      bool isActive = (f == options.currentActiveFile);
      menu.addItem(f.getFileNameWithoutExtension(), true, isActive, [f, options]() {
        if (options.onFileSelected) { options.onFileSelected(f); }
      });
    }
  }
};

} // namespace Uhhyou
