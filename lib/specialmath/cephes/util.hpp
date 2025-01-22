#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace cephes {
namespace internal {

template<typename T, size_t N> inline T polevl(T x, const std::array<T, N>& co) {
  static_assert(N > 0, "Polynomial coefficients array cannot be empty");

  T y = co[0];
  for (size_t i = 1; i < N; ++i) { y = y * x + co[i]; }
  return y;
}

template<typename T, size_t N> inline T p1evl(T x, const std::array<T, N>& co) {
  static_assert(N > 0, "Polynomial coefficients array cannot be empty");

  T y = x + co[0];
  for (size_t i = 1; i < N; ++i) { y = y * x + co[i]; }
  return y;
}

// Generic Chebyshev evaluator (replaces chbevlf/chbevl)
// Evaluates sum of c[i] * T_i(x)
template<typename T, size_t N> inline T chbevl(T x, const std::array<T, N>& co) {
  static_assert(N > 0, "Polynomial coefficients array cannot be empty");

  T b0 = co[0];
  T b1 = T(0);
  T b2 = T(0);
  for (size_t i = 1; i < N; ++i) {
    b2 = b1;
    b1 = b0;
    b0 = x * b1 - b2 + co[i];
  }
  return T(0.5) * (b0 - b2);
}

} // namespace internal
} // namespace cephes
