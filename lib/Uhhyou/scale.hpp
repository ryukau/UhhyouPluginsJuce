// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <limits>
#include <numbers>
#include <utility>

namespace Uhhyou {

namespace ScaleTools {

template<typename T> inline T dbToAmp(T dB) {
  return std::exp(dB * static_cast<T>(0.1151292546497022842));
}

template<typename T> inline T ampToDB(T amp) { return T(20) * std::log10(amp); }

} // namespace ScaleTools

// Range is [min, max]. Beware that `max` is inclusive.
template<typename T> class IntScale {
private:
  static constexpr int32_t maxFloat32 = int32_t(1) << int32_t(24);

  int32_t min_{};
  int32_t max_{};
  int32_t diff_{};

public:
  IntScale(int32_t min, int32_t max) : min_(min), max_(max), diff_(max - min) {
    assert(min_ < max_);
    assert(-maxFloat32 < min_ && min_ < maxFloat32);
    assert(-maxFloat32 < max_ && max_ < maxFloat32);
  }

  int32_t map(T normalized) const {
    return std::clamp(int32_t(normalized * diff_) + min_, min_, max_);
  }

  int32_t reverseMap(T input) const { return map(T(1) - input); }
  T invmap(T raw) const { return std::clamp((raw - T(min_)) / T(diff_), T(0), T(1)); }

  int32_t getMin() const { return min_; }
  int32_t getMax() const { return max_; }

  T toDisplay(T normalized) { return T(map(normalized)); }
  T fromDisplay(T display) { return invmap(display); }
};

// If there are elements of 0, 1, 2, then max is 2.
template<typename T> class UIntScale {
private:
  uint32_t max_;

public:
  UIntScale(uint32_t max) : max_(max) { assert(max_ < (uint32_t(1) << uint32_t(24))); }
  uint32_t map(T input) const { return std::min(max_, uint32_t(input * (max_ + 1))); }
  uint32_t reverseMap(T input) const { return map(T(1) - input); }
  T invmap(T input) const { return input / T(max_); }
  uint32_t getMin() const { return 0; }
  uint32_t getMax() const { return max_; }

  T toDisplay(T normalized) { return T(map(normalized)); }
  T fromDisplay(T display) { return invmap(display); }
};

// Maps a value in [0, 1] to [min, max].
// min < max.
template<typename T> class LinearScale {
private:
  T scale_{};
  T min_{};
  T max_{};

public:
  LinearScale(T min, T max) { set(min, max); }

  void set(T min, T max) {
    min_ = min;
    max_ = max;
    scale_ = (max - min);
  }

  T map(T input) const {
    T value = input * scale_ + min_;
    return std::clamp(value, min_, max_);
  }

  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T input) const {
    T value = (input - min_) / scale_;
    return std::clamp(value, T(0), T(1));
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min_; }
  T getMax() { return max_; }
};

// min < max. power > 0.
template<typename T> class SPolyScale {
private:
  T scale_{};
  T min_{};
  T max_{};
  T power_{};
  T powerInv_{};

public:
  SPolyScale(T min, T max, T power = T(2.0)) { set(min, max, power); }

  void set(T min, T max, T power) {
    min_ = min;
    max_ = max;
    power_ = power;
    powerInv_ = T(1.0) / power;
    scale_ = (max - min);
  }

  T map(T input) const {
    if (input < T(0.0)) { return min_; }
    if (input > T(1.0)) { return max_; }
    T value = input <= T(0.5) ? T(0.5) * std::pow(T(2.0) * input, power_)
                              : T(1.0) - T(0.5) * std::pow(T(2.0) - T(2.0) * input, power_);
    return value * scale_ + min_;
  }

  T reverseMap(T input) const { return map(T(1.0) - input); }

  T invmap(T input) const {
    if (input < min_) { return T(0.0); }
    if (input > max_) { return T(1.0); }
    T value = (input - min_) / scale_;
    return value <= T(0.5) ? T(0.5) * std::pow(T(2.0) * value, powerInv_)
                           : T(1.0) - T(0.5) * std::pow(T(2.0) - T(2.0) * value, powerInv_);
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min_; }
  T getMax() { return max_; }
};

// Based on superellipse. min < max. power > 0.
template<typename T> class EllipticScale {
private:
  static constexpr T pi = std::numbers::pi_v<T>;

  T scale_{};
  T min_{};
  T max_{};
  T power_{};
  T powerInv_{};

public:
  EllipticScale(T min, T max, T power = T(2.0)) { set(min, max, power); }

  void set(T min, T max, T power) {
    min_ = min;
    max_ = max;
    power_ = power;
    powerInv_ = T(1.0) / power;
    scale_ = (max - min);
  }

  T map(T value) const {
    if (value < T(0.0)) { return min_; }
    if (value > T(1.0)) { return max_; }
    value = value <= T(0.5) ? T(0.5) * (T(1.0) - std::pow(std::cos(value * pi), power_))
                            : T(0.5) + T(0.5) * std::pow(std::cos((T(1.0) - value) * pi), power_);
    return value * scale_ + min_;
  }

  T reverseMap(T input) const { return map(T(T(1.0)) - input); }

  T invmap(T value) const {
    if (value < min_) { return T(0.0); }
    if (value > max_) { return T(1.0); }
    value = (value - min_) / scale_;
    return value <= T(0.5) ? std::acos(std::pow(T(1.0) - value * T(2.0), powerInv_)) / pi
                           : T(1.0) - std::acos(std::pow(T(2.0) * value - T(1.0), powerInv_)) / pi;
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min_; }
  T getMax() { return max_; }
};

// map(inValue) == outValue.
// min < max, inValue > 0, outValue > min.
template<typename T> class LogScale {
private:
  T scale_{};
  T expo_{};
  T expoInv_{};
  T min_{};
  T max_{};

public:
  LogScale(T min, T max, T inValue = T(0.5), T outValue = T(0.1)) {
    set(min, max, inValue, outValue);
  }

  void set(T min, T max, T inValue, T outValue) {
    min_ = min;
    max_ = max;
    scale_ = max - min;
    expo_ = std::log((outValue - min) / scale_) / std::log(inValue);
    expoInv_ = T(1.0) / expo_;
  }

  T map(T input) const {
    if (input < T(0.0)) { return min_; }
    if (input > T(1.0)) { return max_; }
    T value = std::pow(input, expo_) * scale_ + min_;
    return value;
  }

  T reverseMap(T input) const { return map(T(1.0) - input); }

  T invmap(T input) const {
    if (input < min_) { return T(0.0); }
    if (input > max_) { return T(1.0); }
    T value = std::pow((input - min_) / scale_, expoInv_);
    return value;
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min_; }
  T getMax() { return max_; }
};

// `min` and `max` is MIDI note number.
// 69 is A4, 440Hz.
// Maps nomalized value to frequency.
template<typename T> class SemitoneScale {
private:
  bool minToZero_{};
  T minNote_{};
  T maxNote_{};
  T minFreq_{};
  T maxFreq_{};
  T scaleNote_{};

public:
  SemitoneScale(T minNote, T maxNote, bool minToZero) { set(minNote, maxNote, minToZero); }

  void set(T minNote, T maxNote, bool minToZero) {
    minToZero_ = minToZero;
    minNote_ = minNote;
    maxNote_ = maxNote;
    minFreq_ = noteToFreq(minNote);
    maxFreq_ = noteToFreq(maxNote);
    scaleNote_ = maxNote - minNote;
  }

  T map(T normalized) const { return noteToFreq(toDisplay(normalized)); }
  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T hz) const {
    if (hz <= T(0)) { return T(0); }
    T normalized = (freqToNote(hz) - minNote_) / scaleNote_;
    return std::clamp(normalized, T(0), T(1));
  }

  T toDisplay(T normalized) const {
    // normalized to MIDI note number conversion.
    if (minToZero_ && normalized <= T(0)) { return T(0); }
    return std::clamp(normalized * scaleNote_ + minNote_, minNote_, maxNote_);
  }

  T fromDisplay(T display) const {
    T normalized = (display - minNote_) / scaleNote_;
    return std::clamp(normalized, T(0), T(1));
  }

  T getMin() const { return minToZero_ ? 0 : minFreq_; }
  T getMax() const { return maxFreq_; }
};

// Maps value normalized in [0, 1] -> dB -> amplitude.
template<typename T> class DecibelScale {
private:
  bool minToZero_{};
  T scaleDB_{};
  T minDB_{};
  T maxDB_{};
  T minAmp_{};
  T maxAmp_{};

public:
  DecibelScale(T minDB, T maxDB, bool minToZero) { set(minDB, maxDB, minToZero); }

  void set(T minDB, T maxDB, bool minToZero) {
    minToZero_ = minToZero;
    minDB_ = minDB;
    maxDB_ = maxDB;
    minAmp_ = minToZero_ ? T(0) : ScaleTools::dbToAmp(minDB_);
    maxAmp_ = ScaleTools::dbToAmp(maxDB_);
    scaleDB_ = (maxDB_ - minDB_);
  }

  T map(T normalized) const { return ScaleTools::dbToAmp(toDisplay(normalized)); }
  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T amplitude) const {
    if (amplitude <= T(0)) { return T(0); }
    T normalized = (ScaleTools::ampToDB(amplitude) - minDB_) / scaleDB_;
    return std::clamp(normalized, T(0), T(1));
  }

  T invmapDB(T dB) const {
    T normalized = (dB - minDB_) / scaleDB_;
    return std::clamp(normalized, T(0), T(1));
  }

  T toDisplay(T normalized) const {
    // normalized to decibel conversion.
    if (minToZero_ && normalized <= T(0)) { return -std::numeric_limits<T>::infinity(); }
    return std::clamp(normalized * scaleDB_ + minDB_, minDB_, maxDB_);
  }

  T fromDisplay(T decibel) const { return invmapDB(decibel); }

  T getMin() const { return minToZero_ ? 0 : minAmp_; }
  T getMax() const { return maxAmp_; }

  T getMinDB() const { return minDB_; }
  T getMaxDB() const { return maxDB_; }
  T getRangeDB() const { return scaleDB_; }
};

// Maps value normalized in [0, 1] to dB, then add or subtract the value from `offset`.
//
// Added to use for feedback or resonance. Increasing normalized value makes the raw value
// to be close to `offset`.
//
// `toDisplay` returns decibel value before subtracting from offset.
template<typename T> class NegativeDecibelScale {
private:
  DecibelScale<T> scale_;
  T offset_{};

public:
  NegativeDecibelScale(T minDB, T maxDB, T offset, bool minToZero)
      : scale_(minDB, maxDB, minToZero) {
    offset_ = offset;
  }

  void set(T minDB, T maxDB, T offset, bool minToZero) {
    offset_ = offset;
    scale_.set(minDB, maxDB, minToZero);
  }

  T map(T normalized) const { return offset_ - scale_.map(T(1) - normalized); }
  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T amplitude) const { return T(1) - scale_.invmap(offset_ - amplitude); }
  T invmapDB(T dB) const { return T(1) - scale_.invmapDB(dB); }

  T toDisplay(T normalized) const { return scale_.toDisplay(T(1) - normalized); }
  T fromDisplay(T decibel) const { return invmapDB(decibel); }

  T getMin() const { return offset_ - scale_.getMax(); }
  T getMax() const { return offset_ - scale_.getMin(); }

  T getMinDB() const { return scale_.getMinDB(); }
  T getMaxDB() const { return scale_.getMaxDB(); }
  T getRangeDB() const { return scale_.getRangeDB(); }
};

// Internal use only.
template<typename T, typename Scale> class BipolarDecibelBase {
protected:
  static constexpr T tolerance = std::numeric_limits<T>::epsilon();
  static constexpr T center = T(0.5);
  static constexpr T upperRangeStart = center * (T(1) + tolerance);
  static constexpr T lowerRangeEnd = center * (T(1) - tolerance);

  Scale scale_;

public:
  static constexpr bool isBipolarDecibel = true;

  template<typename... Args>
  BipolarDecibelBase(Args&&... args) : scale_(std::forward<Args>(args)...) {}

  T map(T normalized) const {
    if (normalized >= upperRangeStart) {
      return scale_.map((normalized - upperRangeStart) / (T(1) - upperRangeStart));
    } else if (normalized <= lowerRangeEnd) {
      return -scale_.map(T(1) - normalized / lowerRangeEnd);
    }
    return 0;
  }

  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T amplitude) const {
    if (amplitude > 0) {
      return scale_.invmap(amplitude) * (T(1) - upperRangeStart) + upperRangeStart;

    } else if (amplitude < 0) {
      return (T(1) - scale_.invmap(-amplitude)) * lowerRangeEnd;
    }
    return center;
  }

  T invmapDB(T dB, T sign) const {
    if (sign == 0 || dB < scale_.getMinDB()) { return center; }
    return invmap(std::copysign(ScaleTools::dbToAmp(dB), sign));
  }

  T toDisplay(T normalized) const {
    if (normalized >= upperRangeStart) {
      return scale_.toDisplay((normalized - upperRangeStart) / (T(1) - upperRangeStart));
    } else if (normalized <= lowerRangeEnd) {
      return scale_.toDisplay(T(1) - normalized / lowerRangeEnd);
    }
    return 0;
  }

  T fromDisplay(T decibel) const {
    return invmapDB(std::abs(decibel), std::copysign(T(1), decibel));
  }

  T getMin() const { return 0; }
  T getMax() const { return scale_.getMax(); }
};

// DecibelScale, but can have negative values when normalized value is below `center`.
//
// - `center` is fixed to 0.5.
// - When normalized value is in `center`, `map()` outputs 0.
// - Same decibel range is used for positive and negative values.
//
// This scale is added for FM or PM amount.
template<typename T> class BipolarDecibelScale : public BipolarDecibelBase<T, DecibelScale<T>> {
public:
  BipolarDecibelScale(T minDB, T maxDB)
      : BipolarDecibelBase<T, DecibelScale<T>>(minDB, maxDB, false) {}

  void set(T minDB, T maxDB) { this->scale_.set(minDB, maxDB, false); }
};

// DecibelScale, but can have negative values when normalized value is below `center`.
//
// - `center` is fixed to 0.5.
// - When normalized value is in `center`, `map()` outputs 0.
// - Same decibel range is used for positive and negative values.
//
// Added to use for feedback or resonance. Increasing normalized value makes the raw value
// to be close to `offset`.
template<typename T>
class BipolarNegativeDecibelScale : public BipolarDecibelBase<T, NegativeDecibelScale<T>> {
public:
  BipolarNegativeDecibelScale(T minDB, T maxDB)
      : BipolarDecibelBase<T, NegativeDecibelScale<T>>(minDB, maxDB, T(1), false) {}

  void set(T minDB, T maxDB) { this->scale_.set(minDB, maxDB, T(1), false); }
};

} // namespace Uhhyou
