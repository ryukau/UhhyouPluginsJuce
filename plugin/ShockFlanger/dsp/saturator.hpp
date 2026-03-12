// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

/*
Note on the higher order antiderivative antialiasing (ADAA):

ADAA requires to compute definite integral (Parker 2016). Finite difference formulation
(Bilbao 2017) can't be used for higher order ADAA because the cascade of `F(a) - F(b) / (a
- b)`, where `F` is some higher order antiderivative, is numerically unstable. This is the
reason Gauss-Legendre quadrature is used for higher order ADAA in this code.

References:

- Parker, J. D., Zavalishin, V., & Le Bivic, E. (2016, September). "Reducing the aliasing
  of nonlinear waveshaping using continuous-time convolution." In Proc. Int. Conf. Digital
  Audio Effects (DAFx-16), Brno, Czech Republic (pp. 137-144).
- Bilbao, S., Esqueda, F., Parker, J. D., & Välimäki, V. (2017). "Antiderivative
  antialiasing for memoryless nonlinearities." IEEE Signal Processing Letters, 24(7),
  1049-1053.
*/

#pragma once

#include "specialmath/cephes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <limits>
#include <numbers>
#include <type_traits>

namespace Uhhyou {

template<typename Real> inline Real sum_of_sqrt(uint64_t N) {
  alignas(64) static constexpr std::array<Real, 64> table{
    Real(0.00000000000000000e00), Real(1.00000000000000000e00), Real(2.41421356237309492e00),
    Real(4.14626436994197256e00), Real(6.14626436994197256e00), Real(8.38233234744176237e00),
    Real(1.08318220902249394e01), Real(1.34775734012895310e01), Real(1.63060005260357208e01),
    Real(1.93060005260357208e01), Real(2.24682781862040990e01), Real(2.57849029765595006e01),
    Real(2.92490045916972541e01), Real(3.28545558671612454e01), Real(3.65962132539351828e01),
    Real(4.04691966001426024e01), Real(4.44691966001426024e01), Real(4.85923022257602639e01),
    Real(5.28349429128795478e01), Real(5.71938418564202209e01), Real(6.16659778114198005e01),
    Real(6.62485535063756430e01), Real(7.09389692661990665e01), Real(7.57348007895117945e01),
    Real(8.06337802750781520e01), Real(8.56337802750781520e01), Real(9.07327997886709312e01),
    Real(9.59289522113775632e01), Real(1.01220454833506750e02), Real(1.06605619640641251e02),
    Real(1.12082845215692913e02), Real(1.17650609578522932e02), Real(1.23307463828015315e02),
    Real(1.29052026474553344e02), Real(1.34882978369398643e02), Real(1.40799058152498247e02),
    Real(1.46799058152498247e02), Real(1.52881820682796473e02), Real(1.59046234685765455e02),
    Real(1.65291232684163845e02), Real(1.71615788004500615e02), Real(1.78018912241933464e02),
    Real(1.84499652940341321e02), Real(1.91057091464643321e02), Real(1.97690341045354131e02),
    Real(2.04398544977853476e02), Real(2.11180874960978770e02), Real(2.18036529561379808e02),
    Real(2.24964732791655308e02), Real(2.31964732791655308e02), Real(2.39035800603520784e02),
    Real(2.46177229032063622e02), Real(2.53388331582991611e02), Real(2.60668441472272150e02),
    Real(2.68016910700621679e02), Real(2.75433109187717321e02), Real(2.82916423961265195e02),
    Real(2.90466258396535977e02), Real(2.98082031502399843e02), Real(3.05763177250268484e02),
    Real(3.13509143942683295e02), Real(3.21319393618589970e02), Real(3.29193401492601765e02),
    Real(3.37130655425795567e02),
  };
  if (N < static_cast<uint64_t>(table.size())) { return table[N]; }

  Real n = static_cast<Real>(N);
  Real sqrt_n = std::sqrt(n);

  constexpr Real zeta_term = Real(-0.207886224977354566); // zeta(-0.5).
  Real base = ((Real(2) / Real(3)) * n + Real(0.5)) * sqrt_n + zeta_term;

  constexpr Real C3 = Real(1) / Real(9216);
  constexpr Real C2 = Real(-1) / Real(1920);
  constexpr Real C1 = Real(1) / Real(24);
  if constexpr (std::is_same_v<Real, float>) {
    return base + C1 / sqrt_n;
  } else {
    Real m = Real(1) / (n * n);
    return base + (((C3 * m + C2) * m + C1) / sqrt_n);
  }
}

/**
`HardclipAdaa4` uses uniform cubic B-spline kernel and Gauss-Legendre quadrature.
*/
template<std::floating_point T> class HardclipAdaa4 {
private:
  static constexpr int nPoints = 3;
  static constexpr int nSegment = 4;

  // 3-point Gauss-Legendre nodes and weights for [-1, 1]
  static constexpr std::array<T, nPoints> nodes
    = {T(-7.74596669241483404e-01), T(0.00000000000000000e+00), T(7.74596669241483404e-01)};
  static constexpr std::array<T, nPoints> weights
    = {T(5.55555555555555691e-01), T(8.88888888888888840e-01), T(5.55555555555555691e-01)};

  // Exact B-Spline Areas (M0) and First Moments (M1)
  // M0 = Integral B(t) dt
  // M1 = Integral t*B(t) dt
  static constexpr std::array<T, nSegment> basisAreas
    = {T(1) / T(24), T(11) / T(24), T(11) / T(24), T(1) / T(24)};
  static constexpr std::array<T, nSegment> basisMoments
    = {T(1) / T(30), T(11) / T(40), T(11) / T(60), T(1) / T(120)};

  static constexpr T tolerance = std::numeric_limits<T>::epsilon();
  std::array<T, nSegment + 1> buffer_{};

  enum class Region { Linear, SatPos, SatNeg };

  template<int seg_idx> static inline T b_spline(T u) {
    constexpr T sixth = T(1) / T(6);
    if constexpr (seg_idx == 0) {
      return sixth * u * u * u;
    } else if constexpr (seg_idx == 1) {
      return sixth * (u * (u * (T(-3) * u + T(3)) + T(3)) + T(1));
    } else if constexpr (seg_idx == 2) {
      const T u2 = u * u;
      return sixth * ((T(3) * u - T(6)) * u2 + T(4));
    } else {
      const T a = T(1) - u;
      return sixth * (a * a * a);
    }
  }

  template<int seg_idx, Region region> inline T integrate(T t0, T t1, T x_a, T diff) {
    const T width = t1 - t0;
    if (width < tolerance) { return T(0); }

    const T half_width = T(0.5) * width;
    const T center = t0 + half_width;

    T sum = T(0);
    for (size_t k = 0; k < nPoints; ++k) {
      const T t_node = center + half_width * nodes[k];

      T value;
      if constexpr (region == Region::Linear) {
        value = x_a + diff * t_node;
      } else if constexpr (region == Region::SatPos) {
        value = T(1);
      } else {
        value = T(-1);
      }

      sum += weights[k] * value * b_spline<seg_idx>(t_node);
    }
    return half_width * sum;
  }

  template<int seg_idx> inline void process_segment(T x_a, T x_b, T& value) {
    // `process_segment` computes a definite integral from `x_a` to `x_b` on hardcilp function.
    // Gauss-Legendre quadrature is used.
    //
    // Gauss-Legendre quadrature is only valid for polynomials. Hardclip is a piecewise function
    // that has discontinuities at -1 and 1. If the segment between `x_a` and `x_b` crosses the
    // discontinuity, the quadrature must be computed separately for each region.

    // Segment is in constant region above +1.
    const bool a_is_above = x_a > T(1);
    const bool b_is_above = x_b > T(1);
    if (a_is_above && b_is_above) {
      value += basisAreas[seg_idx];
      return;
    }

    // Segment is in constant region below -1.
    const bool a_is_below = x_a < T(-1);
    const bool b_is_below = x_b < T(-1);
    if (a_is_below && b_is_below) {
      value -= basisAreas[seg_idx];
      return;
    }

    // Segment is in linear region in [-1, 1].
    const T diff = x_b - x_a;
    if (!a_is_above && !b_is_above && !a_is_below && !b_is_below) {
      value += x_a * basisAreas[seg_idx] + diff * basisMoments[seg_idx];
      return;
    }

    // Fallback to avoid 0 division.
    if (std::abs(diff) < tolerance) {
      value += std::clamp(x_a, T(-1), T(1)) * basisAreas[seg_idx];
      return;
    }

    // Integration.
    const T inv_diff = T(1) / diff;
    const T t_pos = (T(+1) - x_a) * inv_diff;
    const T t_neg = (T(-1) - x_a) * inv_diff;
    T t_curr = T(0);
    if (diff > 0) {
      if (x_a < T(-1)) {
        const T t_next = (t_neg > T(0) && t_neg < T(1)) ? t_neg : T(1);
        value += integrate<seg_idx, Region::SatNeg>(t_curr, t_next, x_a, diff);
        t_curr = t_next;
      }

      if (t_curr < T(1)) {
        const T t_next = (t_pos > t_curr && t_pos < T(1)) ? t_pos : T(1);
        value += integrate<seg_idx, Region::Linear>(t_curr, t_next, x_a, diff);
        t_curr = t_next;
      }

      if (t_curr < T(1)) { value += integrate<seg_idx, Region::SatPos>(t_curr, T(1), x_a, diff); }
    } else {
      if (x_a > T(1)) {
        const T t_next = (t_pos > T(0) && t_pos < T(1)) ? t_pos : T(1);
        value += integrate<seg_idx, Region::SatPos>(t_curr, t_next, x_a, diff);
        t_curr = t_next;
      }

      if (t_curr < T(1)) {
        const T t_next = (t_neg > t_curr && t_neg < T(1)) ? t_neg : T(1);
        value += integrate<seg_idx, Region::Linear>(t_curr, t_next, x_a, diff);
        t_curr = t_next;
      }

      if (t_curr < T(1)) { value += integrate<seg_idx, Region::SatNeg>(t_curr, T(1), x_a, diff); }
    }
  }

public:
  void reset() { buffer_.fill(T(0)); }

  T process(T input) {
    std::copy_backward(buffer_.begin(), buffer_.end() - 1, buffer_.end());
    buffer_[0] = input;

    T value = T(0);
    process_segment<0>(buffer_[4], buffer_[3], value);
    process_segment<1>(buffer_[3], buffer_[2], value);
    process_segment<2>(buffer_[2], buffer_[1], value);
    process_segment<3>(buffer_[1], buffer_[0], value);

    return value;
  }
};

/**
`ModuloQuadAdaa4` uses triangular kernel and Gauss-Legendre quadrature.
*/
template<std::floating_point T> class ModuloQuadAdaa4 {
private:
  static constexpr T offset_factor = T(5.77350269189625731e-01); // 1 / sqrt(3).
  static constexpr T epsilon = std::numeric_limits<T>::epsilon();

  T h1_fall_{};
  T h1_rise_{};
  T h2_fall_{};
  T h2_rise_{};
  T h3_fall_{};
  T h3_rise_{};
  T x1_{};

  T f0(T x) const {
    T z = x * x;
    return std::copysign(z - std::floor(z), x);
  }

  T f1(T n, T sqrt_n) const {
    uint64_t n_int = static_cast<uint64_t>(n);
    if (n_int < 64) { return sum_of_sqrt<T>(n_int) - (T(2) / T(3) * n * sqrt_n); }

    constexpr T zeta_term = T(-0.207886224977354566);
    T y = T(0.5) * sqrt_n + zeta_term;

    constexpr T C3 = T(1) / T(9216);
    constexpr T C2 = T(-1) / T(1920);
    constexpr T C1 = T(1) / T(24);

    if constexpr (std::is_same_v<T, float>) {
      y += C1 / sqrt_n;
    } else {
      T m = T(1) / (n * n);
      y += (((C3 * m + C2) * m + C1) / sqrt_n);
    }
    return y;
  }

  std::pair<T, T> integrate_positive(T a, T b) const {
    T diff = b - a;
    if (diff < epsilon) {
      const T mid = T(0.5) * (a + b);
      const T z = mid * mid;
      const T v = z - std::floor(z);
      return {v * diff, v * mid * diff};
    }

    const T k0 = std::floor(a * a);
    const T k1 = std::floor(b * b);

    auto integrate_gl2 = [&](T u, T v, T k) -> std::pair<T, T> {
      T h = T(0.5) * (v - u);
      T m = T(0.5) * (u + v);
      T offset = h * offset_factor;
      T p1 = m - offset;
      T p2 = m + offset;
      T y1 = p1 * p1 - k;
      T y2 = p2 * p2 - k;
      return {h * (y1 + y2), h * (y1 * p1 + y2 * p2)};
    };

    if (k0 == k1) { return integrate_gl2(a, b, k0); }

    const T s0 = std::sqrt(k0 + T(1));
    auto [A_total, M_total] = integrate_gl2(a, s0, k0);

    if (k1 == k0 + T(1)) {
      auto [A_b, M_b] = integrate_gl2(s0, b, k1);
      A_total += A_b;
      M_total += M_b;
    } else {
      const T s1 = std::sqrt(k1);
      A_total += f1(k1, s1) - f1(k0 + 1, s0);
      M_total += T(0.25) * (k1 - k0 - T(1));

      auto [A_b, M_b] = integrate_gl2(s1, b, k1);
      A_total += A_b;
      M_total += M_b;
    }

    return {A_total, M_total};
  }

  std::pair<T, T> integrate(T a, T b) const {
    T sign = 1;
    if (a > b) {
      std::swap(a, b);
      sign = -1;
    }

    T m0 = 0;
    T m1 = 0;
    if (a < 0) {
      const T upper = (b < 0) ? b : T(0);
      auto [p0, p1] = integrate_positive(-upper, -a);
      m0 -= p0;
      m1 += p1;
    }
    if (b > 0) {
      const T lower = (a > 0) ? a : T(0);
      auto [p0, p1] = integrate_positive(lower, b);
      m0 += p0;
      m1 += p1;
    }

    return {m0 * sign, m1 * sign};
  }

public:
  void reset() {
    h1_fall_ = 0;
    h1_rise_ = 0;
    h2_fall_ = 0;
    h2_rise_ = 0;
    h3_fall_ = 0;
    h3_rise_ = 0;
    x1_ = 0;
  }

  T process(T x0) {
    const T d0 = x0 - x1_;

    T t_fall, t_rise;
    if (std::abs(d0) < epsilon) {
      const T mid = T(0.5) * f0(T(0.5) * (x0 + x1_));
      t_fall = mid;
      t_rise = mid;
    } else {
      auto [m0, m1] = integrate(x1_, x0);
      const T inv_d2 = T(1.0) / (d0 * d0);
      t_fall = (x0 * m0 - m1) * inv_d2;
      t_rise = (m1 - x1_ * m0) * inv_d2;
    }

    const T y0 = T(0.25) * t_fall;
    const T y1 = T(0.5) * h1_fall_ + T(0.25) * h1_rise_;
    const T y2 = T(0.25) * h2_fall_ + T(0.5) * h2_rise_;
    const T y3 = T(0.25) * h3_rise_;

    const T output = y0 + y1 + y2 + y3;

    h3_fall_ = h2_fall_;
    h3_rise_ = h2_rise_;
    h2_fall_ = h1_fall_;
    h2_rise_ = h1_rise_;
    h1_fall_ = t_fall;
    h1_rise_ = t_rise;
    x1_ = x0;

    return output;
  }
};

template<std::floating_point T> class TriangleAdaa4 {
private:
  static constexpr T offset_factor = T(0.577350269189625731); // 1 / sqrt(3)

  T h1_fall_{};
  T h1_rise_{};
  T h2_fall_{};
  T h2_rise_{};
  T h3_fall_{};
  T h3_rise_{};
  T x1_{};

  struct Moments {
    T m0;
    T m1;
  };

  inline T f0(T x) const {
    const T t = T(0.25) * x - T(0.25);
    const T u = T(2) * (t - std::floor(t)) - T(1);
    return T(1) - T(2) * std::abs(u);
  }

  inline Moments integrate_segment(T u, T v) const {
    const T h = T(0.5) * (v - u);
    const T m = T(0.5) * (u + v);
    const T offset = h * offset_factor;

    const T p1 = m - offset;
    const T p2 = m + offset;

    const T y1 = f0(p1);
    const T y2 = f0(p2);

    return {h * (y1 + y2), h * (y1 * p1 + y2 * p2)};
  }

  inline T compute_mid_integrals(T a, T b) const {
    return static_cast<int64_t>((b - a) * T(0.5)) & 1
      ? (static_cast<int64_t>(a) & 2) == 0 ? T(+2) / T(3) : T(-2) / T(3)
      : T(0);
  }

  inline Moments integrate(T a, T b) const {
    T sign = T(1);
    if (a > b) {
      std::swap(a, b);
      sign = T(-1);
    }

    const T k1 = std::floor((a + T(1)) * T(0.5)) * T(2) + T(1);
    const T k2 = std::floor((b - T(1)) * T(0.5)) * T(2) + T(1);
    if (k1 >= b) [[likely]] {
      const auto [m0, m1] = integrate_segment(a, b);
      return {m0 * sign, m1 * sign};
    }

    T total_m0 = T(0);
    T total_m1 = T(0);
    {
      const auto [seg_m0, seg_m1] = integrate_segment(a, k1);
      total_m0 += seg_m0;
      total_m1 += seg_m1;
    }

    T current{};
    if (k2 > k1) {
      total_m1 += compute_mid_integrals(k1, k2);
      current = k2;
    } else {
      current = k1;
    }

    if (b > current) {
      const auto [seg_m0, seg_m1] = integrate_segment(current, b);
      total_m0 += seg_m0;
      total_m1 += seg_m1;
    }

    return {total_m0 * sign, total_m1 * sign};
  }

public:
  void reset() {
    h1_fall_ = 0;
    h1_rise_ = 0;
    h2_fall_ = 0;
    h2_rise_ = 0;
    h3_fall_ = 0;
    h3_rise_ = 0;
    x1_ = 0;
  }

  T process(T x0) {
    const T d0 = x0 - x1_;
    T t_fall, t_rise;

    if (std::abs(d0) < std::numeric_limits<T>::epsilon()) {
      const T mid = T(0.5) * f0(T(0.5) * (x0 + x1_));
      t_fall = mid;
      t_rise = mid;
    } else {
      const auto [m0, m1] = integrate(x1_, x0);
      const T inv_d2 = T(1) / (d0 * d0);
      t_fall = (x0 * m0 - m1) * inv_d2;
      t_rise = (m1 - x1_ * m0) * inv_d2;
    }

    const T y0 = T(0.25) * t_fall;
    const T y1 = T(0.5) * h1_fall_ + T(0.25) * h1_rise_;
    const T y2 = T(0.25) * h2_fall_ + T(0.5) * h2_rise_;
    const T y3 = T(0.25) * h3_rise_;

    const T output = y0 + y1 + y2 + y3;

    h3_fall_ = h2_fall_;
    h3_rise_ = h2_rise_;
    h2_fall_ = h1_fall_;
    h2_rise_ = h1_rise_;
    h1_fall_ = t_fall;
    h1_rise_ = t_rise;
    x1_ = x0;

    return output;
  }
};

namespace Adaa1 {

template<typename Real> struct Hardclip {
  static inline Real f0(Real x) { return std::clamp(x, Real(-1), Real(1)); }

  static inline Real f1(Real x) {
    Real z = std::abs(x);
    return z < 1 ? x * x / Real(2) : z - Real(0.5);
  }
};

template<typename Real> struct Softsign {
  static inline Real f0(Real x) { return x / (Real(1) + std::abs(x)); }

  static inline Real f1(Real x) {
    // Basically `log1pmx`.
    Real z = std::abs(x);
    if (z < Real(0.15)) {
      if constexpr (std::is_same_v<Real, float>) {
        Real p = Real(1) / Real(8);
        p = Real(1) / Real(7) - z * p;
        p = Real(1) / Real(6) - z * p;
        p = Real(1) / Real(5) - z * p;
        p = Real(1) / Real(4) - z * p;
        p = Real(1) / Real(3) - z * p;
        p = Real(1) / Real(2) - z * p;
        return z * z * p;
      } else {
        Real p = Real(1) / Real(18);
        p = Real(1) / Real(17) - z * p;
        p = Real(1) / Real(16) - z * p;
        p = Real(1) / Real(15) - z * p;
        p = Real(1) / Real(14) - z * p;
        p = Real(1) / Real(13) - z * p;
        p = Real(1) / Real(12) - z * p;
        p = Real(1) / Real(11) - z * p;
        p = Real(1) / Real(10) - z * p;
        p = Real(1) / Real(9) - z * p;
        p = Real(1) / Real(8) - z * p;
        p = Real(1) / Real(7) - z * p;
        p = Real(1) / Real(6) - z * p;
        p = Real(1) / Real(5) - z * p;
        p = Real(1) / Real(4) - z * p;
        p = Real(1) / Real(3) - z * p;
        p = Real(1) / Real(2) - z * p;
        return z * z * p;
      }
    }
    return z - std::log1p(z);
  }
};

template<typename Real> struct Softsign3 {
  static inline Real f0(Real x) {
    // `a` is scaling factor to avoid blowing up feedback.
    constexpr Real a = Real(1.88988157484230967e+00);
    x *= a;
    Real x3 = x * x * x;
    return x3 / (Real(1) + std::abs(x3));
  }

  static inline Real f1(Real x) {
    constexpr Real a = Real(1.88988157484230967e+00);
    constexpr Real inv_sqrt3 = Real(5.77350269189625731e-01);
    constexpr Real sixth = Real(1) / Real(6);
    Real u = std::abs(a * x);
    Real v = u + Real(1);
    Real t1 = std::log1p(Real(-3) * u / (v * v));
    Real t2 = std::atan((Real(2) * u - Real(1)) * inv_sqrt3);
    return (u + t1 * sixth - t2 * inv_sqrt3) * (Real(1) / a);
  }
};

template<typename Real> struct Tanh {
  static inline Real f0(Real x) { return std::tanh(x); }

  static inline Real f1(Real x) {
    const Real ax = std::abs(x);
    if (ax > Real(20)) { return ax - std::numbers::ln2_v<Real>; }
    const Real s = std::sinh(Real(0.5) * ax);
    return std::log1p(Real(2) * s * s);
  }
};

template<typename Real> struct Atan {
  static inline Real f0(Real x) { return std::atan(x); }

  static inline Real f1(Real x) {
    Real z = std::abs(x);
    if (z < Real(0.25)) {
      Real w = z * z;
      if constexpr (std::is_same_v<Real, float>) {
        Real p = Real(1) / Real(132);
        p = Real(1) / Real(90) - w * p;
        p = Real(1) / Real(56) - w * p;
        p = Real(1) / Real(30) - w * p;
        p = Real(1) / Real(12) - w * p;
        p = Real(1) / Real(2) - w * p;
        return w * p;
      } else {
        Real p = Real(1) / Real(552);
        p = Real(1) / Real(462) - w * p;
        p = Real(1) / Real(380) - w * p;
        p = Real(1) / Real(306) - w * p;
        p = Real(1) / Real(240) - w * p;
        p = Real(1) / Real(182) - w * p;
        p = Real(1) / Real(132) - w * p;
        p = Real(1) / Real(90) - w * p;
        p = Real(1) / Real(56) - w * p;
        p = Real(1) / Real(30) - w * p;
        p = Real(1) / Real(12) - w * p;
        p = Real(1) / Real(2) - w * p;
        return w * p;
      }
    }
    return z * std::atan(z) - Real(0.5) * std::log1p(z * z);
  }
};

template<typename Real> struct Expm1 {
  static inline Real f0(Real x) { return std::copysign(-std::expm1(-std::abs(x)), x); }

  static inline Real f1(Real x) {
    Real z = std::abs(x);
    if (z < Real(0.25)) {
      if constexpr (std::is_same_v<Real, float>) {
        Real p = Real(1) / Real(5040);
        p = Real(1) / Real(720) - z * p;
        p = Real(1) / Real(120) - z * p;
        p = Real(1) / Real(24) - z * p;
        p = Real(1) / Real(6) - z * p;
        p = Real(1) / Real(2) - z * p;
        return z * z * p;
      } else {
        Real p = Real(1) / Real(479001600);
        p = Real(1) / Real(39916800) - z * p;
        p = Real(1) / Real(3628800) - z * p;
        p = Real(1) / Real(362880) - z * p;
        p = Real(1) / Real(40320) - z * p;
        p = Real(1) / Real(5040) - z * p;
        p = Real(1) / Real(720) - z * p;
        p = Real(1) / Real(120) - z * p;
        p = Real(1) / Real(24) - z * p;
        p = Real(1) / Real(6) - z * p;
        p = Real(1) / Real(2) - z * p;
        return z * z * p;
      }
    }
    return z + std::expm1(-z);
  }
};

template<typename Real> struct Log1p {
  static inline Real f0(Real x) { return std::copysign(std::log1p(std::abs(x)), x); }

  static inline Real f1(Real x) {
    const Real z = std::abs(x);
    if (z < Real(1e-4)) {
      return z * z * (Real(0.5) + z * (Real(-1) / Real(6) + z * (Real(1) / Real(12))));
    }
    return (z + Real(1)) * std::log1p(z) - z;
  }
};

template<typename Real> struct Triangle {
  static inline Real f0(Real x) {
    Real t = Real(0.25) * x - Real(0.25);
    Real u = Real(2) * (t - std::floor(t)) - Real(1);
    return Real(1) - Real(2) * std::abs(u);
  }

  static inline Real f1(Real x) {
    Real t = Real(0.25) * x - Real(0.25);
    Real u = Real(2) * (t - std::floor(t)) - Real(1);
    return Real(2) * u * (Real(1) - std::abs(u));
  }
};

template<typename Real> struct ModuloSqrt {
  static inline Real f0(Real x) {
    Real root = std::sqrt(std::abs(x));
    Real u = root - std::floor(root);
    return std::copysign(u * u, x);
  }

  static inline Real f1(Real x) {
    Real root = std::sqrt(std::abs(x));
    Real n = std::floor(root);
    Real u = root - n;
    Real u2 = u * u;
    Real u3 = u2 * u;
    Real u4 = u2 * u2;
    Real inner_term = (Real(4) * u3) + (Real(2) * n) + Real(1);
    Real numerator = (n * inner_term) + (Real(3) * u4);
    return numerator * (Real(1) / Real(6));
  }
};

template<typename Real> struct ModuloLinear {
  static inline Real f0(Real x) {
    Real z = std::abs(x);
    Real r = z - std::floor(z);
    return std::copysign(r, x);
  }

  static inline Real f1(Real x) {
    Real z = std::abs(x);
    Real n = std::floor(z);
    Real r = z - n;
    return Real(0.5) * (n + r * r);
  }
};

template<typename Real> struct ModuloLinear2 {
  static inline Real f0(Real x) {
    Real z = std::abs(x);
    Real n = std::floor(z);
    Real r = z - n;
    return std::copysign(r * r, x);
  }

  static inline Real f1(Real x) {
    Real z = std::abs(x);
    Real n = std::floor(z);
    Real r = z - n;
    return (r * r * r + n) / Real(3);
  }
};

template<typename Real> struct ModuloQuad {
  static inline Real f0(Real x) {
    Real z = x * x;
    return std::copysign(z - std::floor(z), x);
  }

  static inline Real f1(Real x) {
    Real z1 = std::abs(x);
    Real z2 = z1 * z1;
    Real n = std::floor(z2);
    return ((Real(1) / Real(3)) * z2 - n) * z1 + sum_of_sqrt<Real>(uint64_t(n));
  }
};

template<typename Real> struct SinExpm1 {
  static constexpr Real upperBound = Real(36.7000000000852253);

  static inline Real f0(Real x) {
    // The upper bound of `x` is set to be close to `f0(x) ~= 1`.
    x = std::min(x, upperBound);
    return std::sin(std::expm1(x));
  }

  static inline Real f1(Real x) {
    constexpr Real cs1 = Real(0.5403023058681398); // cos(1)
    constexpr Real sn1 = Real(0.8414709848078965); // sin(1)
    if (x > upperBound) { return x - Real(35.85129512266874); }
    if (x < Real(-6.0)) {
      Real ex = std::exp(x);
      Real ex2 = ex * ex;
      Real si_approx = ex;
      Real ci_approx = std::numbers::egamma_v<Real> + x - Real(0.25) * ex2;
      return cs1 * si_approx - sn1 * ci_approx;
    }
    // if (x > Real(709)) return cs1 * (pi / Real(2));
    double si, ci;
    cephes::sici(static_cast<double>(std::exp(x)), &si, &ci);
    return cs1 * si - sn1 * ci;
  }
};

template<typename Real> struct SinGrowing {
  static constexpr Real scale = Real(1) - 16 * std::numeric_limits<Real>::epsilon();

  static inline Real f0(Real x) { return scale * x * std::sin(x); }

  static inline Real f1(Real x) {
    if (std::abs(x) < Real(0.1)) {
      const Real z = x * x;
      Real p = Real(1) / Real(3991680);
      p = Real(1) / Real(45360) - z * p;
      p = Real(1) / Real(840) - z * p;
      p = Real(1) / Real(30) - z * p;
      p = Real(1) / Real(3) - z * p;
      return scale * x * z * p;
    }
    return scale * (std::sin(x) - x * std::cos(x));
  }
};

template<typename Real> struct SinGrowing2 {
  static constexpr Real scale = Real(0.5) - 16 * std::numeric_limits<Real>::epsilon();

  static inline Real f0(Real x) { return scale * (x + x * std::sin(x)); }

  static inline Real f1(Real x) {
    if (std::abs(x) < Real(0.1)) {
      const Real z = x * x;
      Real p = Real(1) / Real(45360);
      p = Real(1) / Real(840) - z * p;
      p = Real(1) / Real(30) - z * p;
      p = Real(1) / Real(3) - z * p;
      return scale * (z * (Real(0.5) + x * p));
    }
    const Real s = std::sin(x);
    const Real c = std::cos(x);
    return scale * ((Real(0.5) * x - c) * x + s);
  }
};

template<typename Real> struct SinStairs {
  static inline Real f0(Real x) { return Real(0.5) * (x + std::sin(x)); }
  static inline Real f1(Real x) { return Real(0.25) * x * x - Real(0.5) * std::cos(x); }
};

template<typename Real> struct Versinc {
  static inline Real f0(Real x) {
    if (std::abs(x) <= std::numeric_limits<Real>::epsilon()) { return Real(0); }
    auto sn = std::sin(x / Real(2));
    return Real(2) * sn * sn / x;
  }

  static inline Real f1(Real x) {
    Real z = std::abs(x);
    constexpr Real threshold = std::is_same_v<Real, float> ? 0.1f : Real(1.0e-4);
    if (z < threshold) {
      Real z2 = z * z;
      return z2 * (Real(0.25) + z2 * (Real(-1) / Real(96) + z2 * (Real(1) / Real(4320))));
    }
    return std::log(z) - cephes::Ci(z) + std::numbers::egamma_v<Real>;
  }
};

template<typename Real> struct ChebyshevTrig {
  static constexpr Real pi = std::numbers::pi_v<Real>;
  static constexpr Real A = Real(9);
  static constexpr Real scl = A; /* `A * A` for stable feedback. */

  static inline Real f0(Real x) {
    if constexpr (A == Real(0)) {
      return Real(1);
    } else {
      Real u = x / scl;
      if (std::abs(u) > 1) { return (u > 0) ? Real(1) : std::cos(A * pi); }
      return std::cos(A * std::acos(u));
    }
  }

  static inline Real f1(Real x) {
    if constexpr (A == Real(0)) {
      return x;
    } else if constexpr (A == Real(1)) {
      const Real z = std::abs(x);
      return z > Real(1) ? z - Real(0.5) : Real(0.5) * x * x;
    } else {
      Real u = x / scl;
      if (std::abs(u) > 1) {
        Real s = (u > 0) ? Real(1) : Real(-1);
        Real d = (u > 0) ? Real(1) : std::cos(A * pi);
        Real v = -scl / (A * A - 1);
        if (u < 0) { v *= -d; }
        return v + d * (x - s * scl);
      }
      constexpr Real c1 = Real(+0.5) / (A + 1);
      constexpr Real c2 = Real(-0.5) / (A - 1);
      Real t = std::acos(u);
      return scl * (c1 * std::cos((A + 1) * t) + c2 * std::cos((A - 1) * t));
    }
  }
};

template<typename Real> struct ChebyshevClenshaw {
  static constexpr int order = 9;

  static inline Real f0(Real x) {
    constexpr int N = order < 0 ? -order : order;
    if constexpr (N == 0) {
      return Real(1);
    } else if constexpr (N == 1) {
      return std::clamp(x, Real(-1), Real(1));
    } else {
      x /= Real(N); // Scaling to not blow up the feedback.
      if (x >= Real(1)) { return Real(1); }
      if (x <= Real(-1)) { return N % 2 == 0 ? Real(1) : Real(-1); }
      const Real k = Real(2) * x;
      Real t2 = Real(1);
      Real t1 = x;
      Real t0 = x;
      for (int i = 2; i <= N; ++i) {
        t0 = k * t1 - t2;
        t2 = t1;
        t1 = t0;
      }
      return t0;
    }
  }

  static inline Real f1(Real x) {
    constexpr int N = order < 0 ? -order : order;
    if constexpr (N == 0) {
      return x;
    } else if constexpr (N == 1) {
      const Real z = std::abs(x);
      return z > Real(1) ? z - Real(0.5) : Real(0.5) * x * x;
    } else {
      x /= Real(N); // Scaling to not blow up the feedback.
      constexpr Real at_boundary = Real(1) / Real(N * N - 1);
      constexpr Real offset = []() {
        if constexpr (N % 2 != 0) {
          Real sign = N % 4 == 3 ? Real(1) : Real(-1);
          return sign * Real(N) * at_boundary;
        }
        return Real(0);
      }();
      const Real z = std::abs(x);
      if (z > Real(1)) {
        const Real val = Real(N) * (z - Real(1) - at_boundary - offset);
        if constexpr (N % 2 == 0) {
          return x < Real(0) ? -val : val;
        } else {
          return val;
        }
      }
      const Real k = Real(2) * x;
      Real t2 = Real(1);
      Real t1 = x;
      Real t0 = x;
      for (int i = 2; i <= N; ++i) {
        t0 = k * t1 - t2;
        t2 = t1;
        t1 = t0;
      }
      return Real(N) * (Real(0.5) * ((k * t1 - t2) / Real(N + 1) - t2 / Real(N - 1)) - offset);
    }
  }
};

} // namespace Adaa1

// Using 1st order antiderivative anti-aliasing (ADAA).
template<typename Real> class Saturator {
private:
  HardclipAdaa4<Real> hardclip4_;
  ModuloQuadAdaa4<Real> modulo_quad4_;
  TriangleAdaa4<Real> triangle4_;

  static constexpr Real eps = std::numeric_limits<float>::epsilon();
  Real x1_ = 0;
  Real s1_ = 0;

  template<typename Fn> inline Real processInternal(Real input) {
    const auto d0 = input - x1_;
    const auto s0 = Fn::f1(input);
    const auto output = std::abs(d0) < eps ? Fn::f0(Real(0.5) * (input + x1_)) : (s0 - s1_) / d0;
    s1_ = s0;
    x1_ = input;
    return output;
  }

public:
// TODO: C++26 static reflection can be used to replace X-Macros below.
#define UHHYOU_SATURATOR_FUNCTIONS(X)                                                              \
  X(hardclip_cleaner)    /* Hardclip. 4th order ADAA. Default. */                                  \
  X(hardclip)            /* Hardclip. 1st order ADAA. */                                           \
  X(softsign)            /* Softsign. */                                                           \
  X(softsign3)           /* Softsign modified. x^3 / (1 + |x^3|). */                               \
  X(chebyshev_trig)      /* Chebyshev polynomial using trig functions. accepts fractional `A`. */  \
  X(tanh)                /* tanh. */                                                               \
  X(atan)                /* atan. */                                                               \
  X(expm1)               /* expm1 sigmoid. */                                                      \
  X(log1p)               /* log1p. Mildly unbounded. */                                            \
  X(triangle_cleaner)    /* Triangle */                                                            \
  X(triangle)            /* Triangle */                                                            \
  X(modulo_sqrt)         /* (sqrt(x) mod 1). */                                                    \
  X(modulo_linear)       /* (x mod 1). */                                                          \
  X(modulo_linear2)      /* (x mod 1)^2. */                                                        \
  X(modulo_quad_cleaner) /* (x^2 mod 1). 4th order ADAA. */                                        \
  X(modulo_quad)         /* (x^2 mod 1). 1st order ADAA. */                                        \
  X(sin_expm1)           /* sin(expm1(x)). */                                                      \
  X(sin_growing)         /* Growing sine: x * sin(x). Unbounded. */                                \
  X(sin_growing2)        /* x + x * sin(x). Unbounded. */                                          \
  X(sin_stairs)          /* Soft stairs: x + sin(x). Unbounded. */                                 \
  X(versinc)             /* versinc. */

  enum class Function : unsigned {
#define X(name) name,
    UHHYOU_SATURATOR_FUNCTIONS(X)
#undef X
  };

  void reset() {
    hardclip4_.reset();
    modulo_quad4_.reset();
    triangle4_.reset();

    x1_ = 0;
    s1_ = 0;
  }

  Real process(Real input, Function fn = Function::hardclip_cleaner) {
    using namespace Adaa1;
    switch (fn) {
      default:
      case Function::hardclip_cleaner:
        return hardclip4_.process(input);
      case Function::hardclip:
        return processInternal<Hardclip<Real>>(input);
      case Function::softsign:
        return processInternal<Softsign<Real>>(input);
      case Function::softsign3:
        return processInternal<Softsign3<Real>>(input);
      case Function::chebyshev_trig:
        return processInternal<ChebyshevTrig<Real>>(input);
      case Function::tanh:
        return processInternal<Tanh<Real>>(input);
      case Function::atan:
        return processInternal<Atan<Real>>(input);
      case Function::expm1:
        return processInternal<Expm1<Real>>(input);
      case Function::log1p:
        return processInternal<Log1p<Real>>(input);
      case Function::triangle_cleaner:
        return triangle4_.process(input);
      case Function::triangle:
        return processInternal<Triangle<Real>>(input);
      case Function::modulo_sqrt:
        return processInternal<ModuloSqrt<Real>>(input);
      case Function::modulo_linear:
        return processInternal<ModuloLinear<Real>>(input);
      case Function::modulo_linear2:
        return processInternal<ModuloLinear2<Real>>(input);
      case Function::modulo_quad_cleaner:
        return modulo_quad4_.process(input);
      case Function::modulo_quad:
        return processInternal<ModuloQuad<Real>>(input);
      case Function::sin_expm1:
        return processInternal<SinExpm1<Real>>(input);
      case Function::sin_growing:
        return processInternal<SinGrowing<Real>>(input);
      case Function::sin_growing2:
        return processInternal<SinGrowing2<Real>>(input);
      case Function::sin_stairs:
        return processInternal<SinStairs<Real>>(input);
      case Function::versinc:
        return processInternal<Versinc<Real>>(input);
    }
  }
};

} // namespace Uhhyou
