#pragma once

#include "BinaryData.h"
#include <juce_core/juce_core.h>

namespace Uhhyou {

inline juce::String getLibraryLicenseText() {
  return juce::String::createStringFromData(::UhhyouLicense::README_md,
                                            ::UhhyouLicense::README_mdSize);
}

} // namespace Uhhyou
