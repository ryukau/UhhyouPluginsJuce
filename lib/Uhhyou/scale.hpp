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

  int32_t min{};
  int32_t max{};
  int32_t diff{};

public:
  IntScale(int32_t min, int32_t max) : min(min), max(max), diff(max - min) {
    assert(min < max);
    assert(-maxFloat32 < min && min < maxFloat32);
    assert(-maxFloat32 < max && max < maxFloat32);
  }

  int32_t map(T normalized) const { return std::clamp(int32_t(normalized * diff) + min, min, max); }

  int32_t reverseMap(T input) const { return map(T(1) - input); }
  T invmap(T raw) const { return std::clamp((raw - T(min)) / T(diff), T(0), T(1)); }

  int32_t getMin() const { return min; }
  int32_t getMax() const { return max; }

  T toDisplay(T normalized) { return T(map(normalized)); }
  T fromDisplay(T display) { return invmap(display); }
};

// If there are elements of 0, 1, 2, then max is 2.
template<typename T> class UIntScale {
private:
  uint32_t max;

public:
  UIntScale(uint32_t max) : max(max) { assert(max < (uint32_t(1) << uint32_t(24))); }
  uint32_t map(T input) const { return std::min(max, uint32_t(input * (max + 1))); }
  uint32_t reverseMap(T input) const { return map(T(1) - input); }
  T invmap(T input) const { return input / T(max); }
  uint32_t getMin() const { return 0; }
  uint32_t getMax() const { return max; }

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
  T scale{};
  T min{};
  T max{};
  T power{};
  T powerInv{};

public:
  SPolyScale(T min, T max, T power = T(2.0)) { set(min, max, power); }

  void set(T min, T max, T power) {
    this->min = min;
    this->max = max;
    this->power = power;
    powerInv = T(1.0) / power;
    scale = (max - min);
  }

  T map(T input) const {
    if (input < T(0.0)) { return min; }
    if (input > T(1.0)) { return max; }
    T value = input <= T(0.5) ? T(0.5) * std::pow(T(2.0) * input, power)
                              : T(1.0) - T(0.5) * std::pow(T(2.0) - T(2.0) * input, power);
    return value * scale + min;
  }

  T reverseMap(T input) const { return map(T(1.0) - input); }

  T invmap(T input) const {
    if (input < min) { return T(0.0); }
    if (input > max) { return T(1.0); }
    T value = (input - min) / scale;
    return value <= T(0.5) ? T(0.5) * std::pow(T(2.0) * value, powerInv)
                           : T(1.0) - T(0.5) * std::pow(T(2.0) - T(2.0) * value, powerInv);
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min; }
  T getMax() { return max; }
};

// Based on superellipse. min < max. power > 0.
template<typename T> class EllipticScale {
private:
  static constexpr T pi = std::numbers::pi_v<T>;

  T scale{};
  T min{};
  T max{};
  T power{};
  T powerInv{};

public:
  EllipticScale(T min, T max, T power = T(2.0)) { set(min, max, power); }

  void set(T min, T max, T power) {
    this->min = min;
    this->max = max;
    this->power = power;
    powerInv = T(1.0) / power;
    scale = (max - min);
  }

  T map(T value) const {
    if (value < T(0.0)) { return min; }
    if (value > T(1.0)) { return max; }
    value = value <= T(0.5) ? T(0.5) * (T(1.0) - std::pow(std::cos(value * pi), power))
                            : T(0.5) + T(0.5) * std::pow(std::cos((T(1.0) - value) * pi), power);
    return value * scale + min;
  }

  T reverseMap(T input) const { return map(T(T(1.0)) - input); }

  T invmap(T value) const {
    if (value < min) { return T(0.0); }
    if (value > max) { return T(1.0); }
    value = (value - min) / scale;
    return value <= T(0.5) ? std::acos(std::pow(T(1.0) - value * T(2.0), powerInv)) / pi
                           : T(1.0) - std::acos(std::pow(T(2.0) * value - T(1.0), powerInv)) / pi;
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min; }
  T getMax() { return max; }
};

// map(inValue) == outValue.
// min < max, inValue > 0, outValue > min.
template<typename T> class LogScale {
private:
  T scale{};
  T expo{};
  T expoInv{};
  T min{};
  T max{};

public:
  LogScale(T min, T max, T inValue = T(0.5), T outValue = T(0.1)) {
    set(min, max, inValue, outValue);
  }

  void set(T min, T max, T inValue, T outValue) {
    this->min = min;
    this->max = max;
    scale = max - min;
    expo = std::log((outValue - min) / scale) / std::log(inValue);
    expoInv = T(1.0) / expo;
  }

  T map(T input) const {
    if (input < T(0.0)) { return min; }
    if (input > T(1.0)) { return max; }
    T value = std::pow(input, expo) * scale + min;
    return value;
  }

  T reverseMap(T input) const { return map(T(1.0) - input); }

  T invmap(T input) const {
    if (input < min) { return T(0.0); }
    if (input > max) { return T(1.0); }
    T value = std::pow((input - min) / scale, expoInv);
    return value;
  }

  T toDisplay(T normalized) { return map(normalized); }
  T fromDisplay(T display) { return invmap(display); }

  T getMin() { return min; }
  T getMax() { return max; }
};

// `min` and `max` is MIDI note number.
// 69 is A4, 440Hz.
// Maps nomalized value to frequency.
template<typename T> class SemitoneScale {
private:
  bool minToZero{};
  T minNote{};
  T maxNote{};
  T minFreq{};
  T maxFreq{};
  T scaleNote{};

public:
  SemitoneScale(T minNote, T maxNote, bool minToZero) { set(minNote, maxNote, minToZero); }

  void set(T minNote, T maxNote, bool minToZero) {
    this->minToZero = minToZero;
    this->minNote = minNote;
    this->maxNote = maxNote;
    this->minFreq = noteToFreq(minNote);
    this->maxFreq = noteToFreq(maxNote);
    scaleNote = maxNote - minNote;
  }

  T map(T normalized) const { return noteToFreq(toDisplay(normalized)); }
  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T hz) const {
    if (hz <= T(0)) { return T(0); }
    T normalized = (freqToNote(hz) - minNote) / scaleNote;
    return std::clamp(normalized, T(0), T(1));
  }

  T toDisplay(T normalized) {
    // normalized to MIDI note number conversion.
    if (minToZero && normalized <= T(0)) { return T(0); }
    return std::clamp(normalized * scaleNote + minNote, minNote, maxNote);
  }

  T fromDisplay(T display) {
    T normalized = (display - minNote) / scaleNote;
    return std::clamp(normalized, T(0), T(1));
  }

  T getMin() { return minToZero ? 0 : minFreq; }
  T getMax() { return maxFreq; }
};

// Maps value normalized in [0, 1] -> dB -> amplitude.
template<typename T> class DecibelScale {
private:
  bool minToZero{};
  T scaleDB{};
  T minDB{};
  T maxDB{};
  T minAmp{};
  T maxAmp{};

public:
  DecibelScale(T minDB_, T maxDB_, bool minToZero_) { set(minDB_, maxDB_, minToZero_); }

  void set(T minDB_, T maxDB_, bool minToZero_) {
    minToZero = minToZero_;
    minDB = minDB_;
    maxDB = maxDB_;
    minAmp = minToZero ? T(0) : ScaleTools::dbToAmp(minDB);
    maxAmp = ScaleTools::dbToAmp(maxDB);
    scaleDB = (maxDB - minDB);
  }

  T map(T normalized) const { return ScaleTools::dbToAmp(toDisplay(normalized)); }
  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T amplitude) const {
    if (amplitude <= T(0)) { return T(0); }
    T normalized = (ScaleTools::ampToDB(amplitude) - minDB) / scaleDB;
    return std::clamp(normalized, T(0), T(1));
  }

  T invmapDB(T dB) const {
    T normalized = (dB - minDB) / scaleDB;
    return std::clamp(normalized, T(0), T(1));
  }

  T toDisplay(T normalized) const {
    // normalized to decibel conversion.
    if (minToZero && normalized <= T(0)) { return -std::numeric_limits<T>::infinity(); }
    return std::clamp(normalized * scaleDB + minDB, minDB, maxDB);
  }

  T fromDisplay(T decibel) const { return invmapDB(decibel); }

  T getMin() const { return minToZero ? 0 : minAmp; }
  T getMax() const { return maxAmp; }

  T getMinDB() const { return minDB; }
  T getMaxDB() const { return maxDB; }
  T getRangeDB() const { return scaleDB; }
};

// Maps value normalized in [0, 1] to dB, then add or subtract the value from `offset`.
//
// Added to use for feedback or resonance. Increasing normalized value makes the raw value
// to be close to `offset`.
//
// `toDisplay` returns decibel value before subtracting from offset.
template<typename T> class NegativeDecibelScale {
private:
  DecibelScale<T> scale;
  T offset{};

public:
  NegativeDecibelScale(T minDB, T maxDB, T offset, bool minToZero)
      : scale(minDB, maxDB, minToZero) {
    this->offset = offset;
  }

  void set(T minDB, T maxDB, T offset, bool minToZero) {
    this->offset = offset;
    scale.set(minDB, maxDB, minToZero);
  }

  T map(T normalized) const { return offset - scale.map(T(1) - normalized); }
  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T amplitude) const { return T(1) - scale.invmap(offset - amplitude); }
  T invmapDB(T dB) const { return T(1) - scale.invmapDB(dB); }

  T toDisplay(T normalized) const { return scale.toDisplay(T(1) - normalized); }
  T fromDisplay(T decibel) const { return invmapDB(decibel); }

  T getMin() const { return offset - scale.getMax(); }
  T getMax() const { return offset - scale.getMin(); }

  T getMinDB() const { return scale.getMinDB(); }
  T getMaxDB() const { return scale.getMaxDB(); }
  T getRangeDB() const { return scale.getScaleDB(); }
};

// Internal use only.
template<typename T, typename Scale> class BipolarDecibelBase {
protected:
  static constexpr T tolerance = std::numeric_limits<T>::epsilon();
  static constexpr T center = T(0.5);
  static constexpr T upperRangeStart = center * (T(1) + tolerance);
  static constexpr T lowerRangeEnd = center * (T(1) - tolerance);

  Scale scale;

public:
  static constexpr bool isBipolarDecibel = true;

  template<typename... Args>
  BipolarDecibelBase(Args&&... args) : scale(std::forward<Args>(args)...) {}

  T map(T normalized) const {
    if (normalized >= upperRangeStart) {
      return scale.map((normalized - upperRangeStart) / (T(1) - upperRangeStart));
    } else if (normalized <= lowerRangeEnd) {
      return -scale.map(T(1) - normalized / lowerRangeEnd);
    }
    return 0;
  }

  T reverseMap(T input) const { return map(T(1) - input); }

  T invmap(T amplitude) const {
    if (amplitude > 0) {
      return scale.invmap(amplitude) * (T(1) - upperRangeStart) + upperRangeStart;

    } else if (amplitude < 0) {
      return (T(1) - scale.invmap(-amplitude)) * lowerRangeEnd;
    }
    return center;
  }

  T invmapDB(T dB, T sign) const {
    if (sign == 0 || dB < scale.getMinDB()) { return center; }
    return invmap(std::copysign(ScaleTools::dbToAmp(dB), sign));
  }

  T toDisplay(T normalized) const {
    if (normalized >= upperRangeStart) {
      return scale.toDisplay((normalized - upperRangeStart) / (T(1) - upperRangeStart));
    } else if (normalized <= lowerRangeEnd) {
      return scale.toDisplay(T(1) - normalized / lowerRangeEnd);
    }
    return 0;
  }

  T fromDisplay(T decibel) const {
    return invmapDB(std::abs(decibel), std::copysign(T(1), decibel));
  }

  T getMin() const { return 0; }
  T getMax() const { return scale.getMax(); }
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

  void set(T minDB, T maxDB) { this->scale.set(minDB, maxDB, false); }
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

  void set(T minDB, T maxDB) { this->scale.set(minDB, maxDB, T(1), false); }
};

} // namespace Uhhyou
