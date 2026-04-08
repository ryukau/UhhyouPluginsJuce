// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "specialmath/cephes/sici.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <limits>
#include <numbers>
#include <type_traits>

namespace Uhhyou {

template<std::floating_point T> class HalfwaveAdaa1 {
private:
  T x1_ = T(0);

public:
  void reset() { x1_ = T(0); }

  T process(T input) {
    const T x_a = x1_;
    const T x_b = input;
    x1_ = input;

    if (x_a <= T(0) && x_b <= T(0)) { return T(0); }
    if (x_a >= T(0) && x_b >= T(0)) { return T(0.5) * (x_a + x_b); }

    const T min_x = std::min(x_a, x_b);
    const T max_x = std::max(x_a, x_b);
    const T abs_diff = max_x - min_x;

    const T integral = T(0.5) * max_x * max_x;
    return integral / abs_diff;
  }
};

template<std::floating_point T> class HalfwaveAdaa2 {
private:
  T out_buffer_{T(0)};
  T prev_input_{T(0)};

public:
  void reset() {
    out_buffer_ = T(0);
    prev_input_ = T(0);
  }

  T process(T input) {
    const T xa = prev_input_;
    const T xb = input;

    const bool a_pos = xa >= T(0);
    const bool b_pos = xb >= T(0);

    T i0, i1;

    if (a_pos == b_pos) {
      if (a_pos) {
        static constexpr T inv6 = T(1) / T(6);
        static constexpr T inv3 = T(1) / T(3);

        i0 = xa * inv6 + xb * inv3;
        i1 = xa * inv3 + xb * inv6;
      } else {
        i0 = T(0);
        i1 = T(0);
      }
    } else {
      static constexpr T inv6 = T(1) / T(6);
      static constexpr T half = T(0.5);

      if (b_pos) {
        const T p = xb / (xb - xa);
        const T p_xb = p * xb;

        i1 = p_xb * (p * inv6);
        i0 = p_xb * half - i1;
      } else {
        const T p = xa / (xa - xb);
        const T p_xa = p * xa;

        i0 = p_xa * (p * inv6);
        i1 = p_xa * half - i0;
      }
    }

    const T value = out_buffer_ + i1;
    out_buffer_ = i0;
    prev_input_ = input;
    return value;
  }
};

template<std::floating_point T> class FullwaveAdaa1 {
private:
  T x1_ = T(0);

public:
  void reset() { x1_ = T(0); }

  T process(T input) {
    const T x_a = x1_;
    const T x_b = input;
    x1_ = input;

    if (x_a >= T(0) && x_b >= T(0)) { return T(+0.5) * (x_a + x_b); }
    if (x_a <= T(0) && x_b <= T(0)) { return T(-0.5) * (x_a + x_b); }

    const T min_x = std::min(x_a, x_b);
    const T max_x = std::max(x_a, x_b);
    const T abs_diff = max_x - min_x;

    const T integral = T(0.5) * (min_x * min_x + max_x * max_x);
    return integral / abs_diff;
  }
};

template<std::floating_point T> class FullwaveAdaa2 {
private:
  T out_buffer_{T(0)};
  T prev_input_{T(0)};

public:
  void reset() {
    out_buffer_ = T(0);
    prev_input_ = T(0);
  }

  T process(T input) {
    const T xa = prev_input_;
    const T xb = input;

    const bool a_pos = (xa >= T(0));
    const bool b_pos = (xb >= T(0));

    const T base_xb = std::abs(xb);
    const T base_xa = b_pos ? xa : -xa;

    static constexpr T inv6 = T(1) / T(6);
    static constexpr T inv3 = T(1) / T(3);

    const T base_xa_2 = base_xa + base_xa;
    const T base_xb_2 = base_xb + base_xb;

    T i0 = (base_xa + base_xb_2) * inv6;
    T i1 = (base_xa_2 + base_xb) * inv6;

    if (a_pos != b_pos) {
      const T tz = xa / (xa - xb);
      const T abs_xa = std::abs(xa);

      const T abs_xa_tz = abs_xa * tz;
      const T P0 = abs_xa_tz * tz * inv3;
      const T P1 = abs_xa_tz - P0;

      i0 += P0;
      i1 += P1;
    }

    const T value = out_buffer_ + i1;
    out_buffer_ = i0;
    prev_input_ = input;
    return value;
  }
};

template<std::floating_point T> class HardclipAdaa1 {
private:
  T x1_ = T(0);

public:
  void reset() { x1_ = T(0); }

  T process(T input) {
    const T x_a = x1_;
    const T x_b = input;
    x1_ = input;

    if (std::abs(x_a) <= T(1) && std::abs(x_b) <= T(1)) { return T(0.5) * (x_a + x_b); }
    if (x_a >= T(1) && x_b >= T(1)) { return T(1); }
    if (x_a <= T(-1) && x_b <= T(-1)) { return T(-1); }

    const T min_x = std::min(x_a, x_b);
    const T max_x = std::max(x_a, x_b);
    const T abs_diff = max_x - min_x;

    const T lin_start = std::max(min_x, T(-1));
    const T lin_end = std::min(max_x, T(1));

    const T lin_integral = T(0.5) * (lin_end - lin_start) * (lin_end + lin_start);
    const T integral = (max_x - lin_end) - (lin_start - min_x) + lin_integral;
    return integral / abs_diff;
  }
};

template<std::floating_point T> class HardclipAdaa2 {
private:
  T out_buffer_{T(0)};
  T prev_input_{T(0)};

  static constexpr T half = T(1) / T(2);
  static constexpr T third = T(1) / T(3);
  static constexpr T sixth = T(1) / T(6);

  inline void evaluate_penalty(T v, T& p_end, T& p_start) const {
    const T v2 = v * v;
    const T v3_6 = v2 * v * sixth;
    p_start = v3_6;
    p_end = v2 * half - v3_6;
  }

  inline void compute_interval(T x_a, T x_b, T& i0, T& i1) const {
    const bool a_is_above = x_a >= T(1);
    const bool b_is_above = x_b >= T(1);
    const bool a_is_below = x_a <= T(-1);
    const bool b_is_below = x_b <= T(-1);

    if (a_is_above && b_is_above) {
      i0 = half;
      i1 = half;
      return;
    }

    if (a_is_below && b_is_below) {
      i0 = -half;
      i1 = -half;
      return;
    }

    const T diff = x_b - x_a;

    if (!a_is_above && !b_is_above && !a_is_below && !b_is_below) {
      const T xa_half = x_a * half;
      i0 = xa_half + diff * third;
      i1 = xa_half + diff * sixth;
      return;
    }

    if (std::abs(diff) < std::numeric_limits<T>::epsilon()) {
      const T val = std::clamp(x_a, T(-1), T(1)) * half;
      i0 = val;
      i1 = val;
      return;
    }

    const T xa_half = x_a * half;
    i0 = xa_half + diff * third;
    i1 = xa_half + diff * sixth;

    const T inv_diff = T(1) / diff;

    if (diff > T(0)) {
      const T t_pos = (T(1) - x_a) * inv_diff;
      if (t_pos > T(0) && t_pos < T(1)) {
        T p_end, p_start;
        evaluate_penalty(T(1) - t_pos, p_end, p_start);
        i0 -= diff * p_end;
        i1 -= diff * p_start;
      }

      const T t_neg = (T(-1) - x_a) * inv_diff;
      if (t_neg > T(0) && t_neg < T(1)) {
        T p_end, p_start;
        evaluate_penalty(t_neg, p_end, p_start);
        i0 += diff * p_start;
        i1 += diff * p_end;
      }
    } else {
      const T t_pos = (T(1) - x_a) * inv_diff;
      if (t_pos > T(0) && t_pos < T(1)) {
        T p_end, p_start;
        evaluate_penalty(t_pos, p_end, p_start);
        i0 += diff * p_start;
        i1 += diff * p_end;
      }

      const T t_neg = (T(-1) - x_a) * inv_diff;
      if (t_neg > T(0) && t_neg < T(1)) {
        T p_end, p_start;
        evaluate_penalty(T(1) - t_neg, p_end, p_start);
        i0 -= diff * p_end;
        i1 -= diff * p_start;
      }
    }
  }

public:
  void reset() {
    out_buffer_ = T(0);
    prev_input_ = T(0);
  }

  T process(T input) {
    T i0, i1;
    compute_interval(prev_input_, input, i0, i1);

    const T value = out_buffer_ + i1;
    out_buffer_ = i0;
    prev_input_ = input;
    return value;
  }
};

template<std::floating_point T> class HardclipAdaa4 {
private:
  T out_buffer_[3]{};
  T prev_input_{T(0)};

  static constexpr T area0 = T(1) / T(24);
  static constexpr T area1 = T(11) / T(24);

  static constexpr T mom0 = T(1) / T(30);
  static constexpr T mom1 = T(11) / T(40);
  static constexpr T mom2 = T(11) / T(60);
  static constexpr T mom3 = T(1) / T(120);

  inline void evaluate_H(T c, T& h0, T& h1, T& h2, T& h3) const {
    const T c2 = c * c;

    static constexpr T inv120 = T(1) / T(120);
    static constexpr T inv40 = T(1) / T(40);
    static constexpr T inv24 = T(1) / T(24);
    static constexpr T inv12 = T(1) / T(12);
    static constexpr T inv3 = T(1) / T(3);

    h0 = c2 * c2 * c * inv120;
    h1 = c2 * (inv12 + c * (inv12 + c * (inv24 - c * inv40)));
    h2 = c2 * (inv3 - c2 * (inv12 - c * inv40));
    const T one_minus_c = T(1) - c;
    h3 = c2 * (inv12 * one_minus_c + c2 * (inv24 - c * inv120));
  }

  inline void compute_interval(T x_a, T x_b, T& i0, T& i1, T& i2, T& i3) const {
    const bool a_is_above = x_a >= T(1);
    const bool b_is_above = x_b >= T(1);
    const bool a_is_below = x_a <= T(-1);
    const bool b_is_below = x_b <= T(-1);

    const T diff = x_b - x_a;

    // Fully Saturated Positive
    if (a_is_above && b_is_above) {
      i0 = area0;
      i3 = area0;
      i1 = area1;
      i2 = area1;
      return;
    }

    // Fully Saturated Negative
    if (a_is_below && b_is_below) {
      i0 = -area0;
      i3 = -area0;
      i1 = -area1;
      i2 = -area1;
      return;
    }

    // Fully Contained / Linear
    if (!a_is_above && !b_is_above && !a_is_below && !b_is_below) {
      i0 = x_a * area0 + diff * mom0;
      i1 = x_a * area1 + diff * mom1;
      i2 = x_a * area1 + diff * mom2;
      i3 = x_a * area0 + diff * mom3;
      return;
    }

    if (std::abs(diff) < std::numeric_limits<T>::epsilon()) {
      const T val = std::clamp(x_a, T(-1), T(1));
      const T val_area0 = val * area0;
      const T val_area1 = val * area1;
      i0 = val_area0;
      i3 = val_area0;
      i1 = val_area1;
      i2 = val_area1;
      return;
    }

    i0 = x_a * area0 + diff * mom0;
    i1 = x_a * area1 + diff * mom1;
    i2 = x_a * area1 + diff * mom2;
    i3 = x_a * area0 + diff * mom3;

    const T inv_diff = T(1) / diff;

    if (diff > T(0)) {
      const T t_pos = (T(1) - x_a) * inv_diff;
      if (t_pos > T(0) && t_pos < T(1)) {
        T h0, h1, h2, h3;
        evaluate_H(T(1) - t_pos, h0, h1, h2, h3);
        i0 -= diff * h3;
        i1 -= diff * h2;
        i2 -= diff * h1;
        i3 -= diff * h0;
      }

      const T t_neg = (T(-1) - x_a) * inv_diff;
      if (t_neg > T(0) && t_neg < T(1)) {
        T h0, h1, h2, h3;
        evaluate_H(t_neg, h0, h1, h2, h3);
        i0 += diff * h0;
        i1 += diff * h1;
        i2 += diff * h2;
        i3 += diff * h3;
      }
    } else {
      const T t_pos = (T(1) - x_a) * inv_diff;
      if (t_pos > T(0) && t_pos < T(1)) {
        T h0, h1, h2, h3;
        evaluate_H(t_pos, h0, h1, h2, h3);
        i0 += diff * h0;
        i1 += diff * h1;
        i2 += diff * h2;
        i3 += diff * h3;
      }

      const T t_neg = (T(-1) - x_a) * inv_diff;
      if (t_neg > T(0) && t_neg < T(1)) {
        T h0, h1, h2, h3;
        evaluate_H(T(1) - t_neg, h0, h1, h2, h3);
        i0 -= diff * h3;
        i1 -= diff * h2;
        i2 -= diff * h1;
        i3 -= diff * h0;
      }
    }
  }

public:
  void reset() {
    out_buffer_[0] = T(0);
    out_buffer_[1] = T(0);
    out_buffer_[2] = T(0);
    prev_input_ = T(0);
  }

  T process(T input) {
    T i0, i1, i2, i3;
    compute_interval(prev_input_, input, i0, i1, i2, i3);

    const T value = out_buffer_[0] + i3;
    out_buffer_[0] = out_buffer_[1] + i2;
    out_buffer_[1] = out_buffer_[2] + i1;
    out_buffer_[2] = i0;

    prev_input_ = input;
    return value;
  }
};

template<std::floating_point T> inline T sum_of_sqrt(uint64_t N) {
  alignas(64) static constexpr std::array<T, 64> table{
    T(0.00000000000000000e00), T(1.00000000000000000e00), T(2.41421356237309492e00),
    T(4.14626436994197256e00), T(6.14626436994197256e00), T(8.38233234744176237e00),
    T(1.08318220902249394e01), T(1.34775734012895310e01), T(1.63060005260357208e01),
    T(1.93060005260357208e01), T(2.24682781862040990e01), T(2.57849029765595006e01),
    T(2.92490045916972541e01), T(3.28545558671612454e01), T(3.65962132539351828e01),
    T(4.04691966001426024e01), T(4.44691966001426024e01), T(4.85923022257602639e01),
    T(5.28349429128795478e01), T(5.71938418564202209e01), T(6.16659778114198005e01),
    T(6.62485535063756430e01), T(7.09389692661990665e01), T(7.57348007895117945e01),
    T(8.06337802750781520e01), T(8.56337802750781520e01), T(9.07327997886709312e01),
    T(9.59289522113775632e01), T(1.01220454833506750e02), T(1.06605619640641251e02),
    T(1.12082845215692913e02), T(1.17650609578522932e02), T(1.23307463828015315e02),
    T(1.29052026474553344e02), T(1.34882978369398643e02), T(1.40799058152498247e02),
    T(1.46799058152498247e02), T(1.52881820682796473e02), T(1.59046234685765455e02),
    T(1.65291232684163845e02), T(1.71615788004500615e02), T(1.78018912241933464e02),
    T(1.84499652940341321e02), T(1.91057091464643321e02), T(1.97690341045354131e02),
    T(2.04398544977853476e02), T(2.11180874960978770e02), T(2.18036529561379808e02),
    T(2.24964732791655308e02), T(2.31964732791655308e02), T(2.39035800603520784e02),
    T(2.46177229032063622e02), T(2.53388331582991611e02), T(2.60668441472272150e02),
    T(2.68016910700621679e02), T(2.75433109187717321e02), T(2.82916423961265195e02),
    T(2.90466258396535977e02), T(2.98082031502399843e02), T(3.05763177250268484e02),
    T(3.13509143942683295e02), T(3.21319393618589970e02), T(3.29193401492601765e02),
    T(3.37130655425795567e02),
  };
  if (N < static_cast<uint64_t>(table.size())) { return table[N]; }

  T n = static_cast<T>(N);
  T sqrt_n = std::sqrt(n);

  constexpr T zeta_term = T(-0.207886224977354566); // zeta(-0.5).
  T base = ((T(2) / T(3)) * n + T(0.5)) * sqrt_n + zeta_term;

  constexpr T C3 = T(1) / T(9216);
  constexpr T C2 = T(-1) / T(1920);
  constexpr T C1 = T(1) / T(24);
  if constexpr (std::is_same_v<T, float>) {
    return base + C1 / sqrt_n;
  } else {
    T m = T(1) / (n * n);
    return base + (((C3 * m + C2) * m + C1) / sqrt_n);
  }
}

// `ModuloQuadAdaa4` uses triangular kernel and Gauss-Legendre quadrature.
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
  T out_buffer_[3]{};
  T x1_{T(0)};
  T prev_f0_{T(0)};
  bool prev_is_odd_{false};

  static constexpr T inv6 = T(1) / T(6);
  static constexpr T half = T(0.5);
  static constexpr T quarter = T(0.25);

public:
  void reset() {
    out_buffer_[0] = T(0);
    out_buffer_[1] = T(0);
    out_buffer_[2] = T(0);

    x1_ = T(0);
    prev_is_odd_ = false;
    prev_f0_ = T(0);
  }

  T process(T x0) {
    const T w = x0 - std::floor((x0 + T(1)) * quarter) * T(4);
    const bool is_odd = w > T(1);

    const T f0_curr = std::abs(w - T(1)) - T(1);

    const T d0 = x0 - x1_;
    const T abs_d0 = std::abs(d0);

    T t_fall, t_rise;

    if (abs_d0 <= T(1)) {
      if (is_odd == prev_is_odd_) {
        t_fall = (f0_curr + T(2) * prev_f0_) * inv6;
        t_rise = (T(2) * f0_curr + prev_f0_) * inv6;
      } else {
        const T K = std::floor((std::max(x0, x1_) - T(1)) * half) * T(2) + T(1);
        const T f0_K = ((x0 > x1_) == prev_is_odd_) ? T(1) : T(-1);

        const T inv_d0 = T(1) / d0;
        const T u = (K - x1_) * inv_d0;
        const T v = T(1) - u;

        t_rise = (u * u * prev_f0_ + (u + T(1)) * f0_K + (T(2) + u) * v * f0_curr) * inv6;
        t_fall = (u * (T(2) + v) * prev_f0_ + (T(1) + v) * f0_K + v * v * f0_curr) * inv6;
      }
    } else {
      const T prev_f0_2 = prev_f0_ * prev_f0_;
      const T prev_F0 = (prev_is_odd_ ? -half : half) * (T(1) - prev_f0_2);
      const T prev_G0 = prev_f0_ * (prev_f0_2 * inv6 - half);

      const T curr_f0_2 = f0_curr * f0_curr;
      const T F0_curr = (is_odd ? -half : half) * (T(1) - curr_f0_2);
      const T G0_curr = f0_curr * (curr_f0_2 * inv6 - half);

      const T dG = G0_curr - prev_G0;
      const T inv_d2 = T(1) / (d0 * d0);

      t_fall = (dG - d0 * prev_F0) * inv_d2;
      t_rise = (d0 * F0_curr - dG) * inv_d2;
    }

    x1_ = x0;
    prev_is_odd_ = is_odd;
    prev_f0_ = f0_curr;

    const T quarter_t_fall = quarter * t_fall;
    const T half_t_fall = half * t_fall;
    const T quarter_t_rise = quarter * t_rise;
    const T half_t_rise = half * t_rise;

    const T output = out_buffer_[0] + quarter_t_fall;
    out_buffer_[0] = out_buffer_[1] + half_t_fall + quarter_t_rise;
    out_buffer_[1] = out_buffer_[2] + quarter_t_fall + half_t_rise;
    out_buffer_[2] = quarter_t_rise;

    return output;
  }
};

template<std::floating_point T> class TruncAdaa4 {
private:
  T out_buffer_[3]{};
  T prev_input_{T(0)};

  T a_{T(1)};
  T inv_a_{T(1)};

  static constexpr T inv24 = T(1) / T(24);
  static constexpr T inv8 = T(1) / T(8);
  static constexpr T inv6 = T(1) / T(6);
  static constexpr T inv5 = T(1) / T(5);
  static constexpr T inv4 = T(1) / T(4);
  static constexpr T inv3 = T(1) / T(3);
  static constexpr T inv2 = T(1) / T(2);
  static constexpr T two_thirds = T(2) / T(3);
  static constexpr T three_fourths = T(3) / T(4);
  static constexpr T eleven_24ths = T(11) / T(24);

  inline void compute_interval(T xa_p, T xb_p, T& i0, T& i1, T& i2, T& i3) const {
    const T D = xb_p - xa_p;

    if (std::abs(D) < std::numeric_limits<T>::epsilon()) {
      const T mid_p = xa_p + D * T(0.5);
      const T val = a_ * std::trunc(mid_p);
      const T val_inv24 = val * inv24;
      const T val_11_24 = val * eleven_24ths;
      i0 = val_inv24;
      i3 = val_inv24;
      i1 = val_11_24;
      i2 = val_11_24;
      return;
    }

    T start_m, end_m, step, N;
    bool contains_zero;
    if (D > T(0)) {
      start_m = std::floor(xa_p) + T(1);
      end_m = std::floor(xb_p);
      step = T(1);
      N = (start_m <= end_m) ? (end_m - start_m + T(1)) : T(0);
      contains_zero = (start_m <= T(0) && end_m >= T(0));
    } else {
      start_m = std::ceil(xa_p) - T(1);
      end_m = std::ceil(xb_p);
      step = T(-1);
      N = (start_m >= end_m) ? (start_m - end_m + T(1)) : T(0);
      contains_zero = (start_m >= T(0) && end_m <= T(0));
    }

    if (contains_zero) {
      if (start_m == T(0)) {
        start_m += step;
        N -= T(1);
        contains_zero = false;
      } else if (end_m == T(0)) {
        end_m -= step;
        N -= T(1);
        contains_zero = false;
      }
    }

    if (N <= T(0)) {
      const T val = a_ * std::trunc(xb_p);
      const T val_inv24 = val * inv24;
      const T val_11_24 = val * eleven_24ths;
      i0 = val_inv24;
      i3 = val_inv24;
      i1 = val_11_24;
      i2 = val_11_24;
      return;
    }

    const T tA = (start_m - xa_p) / D;
    const T t2 = tA * tA;
    const T t3 = t2 * tA;
    const T t4 = t2 * t2;

    const T c_tA_6 = tA * inv6;
    const T c_t2_4 = t2 * inv4;
    const T c_t3_6 = t3 * inv6;
    const T c_t4_24 = t4 * inv24;
    const T c_t4_8 = t4 * inv8;

    const T term_A1 = c_tA_6 + c_t2_4;
    const T term_B1 = c_t3_6 - c_t4_8;
    const T term_A3 = c_tA_6 - c_t2_4;
    const T term_B3 = c_t3_6 - c_t4_24;

    const T D0_0 = c_t4_24;
    const T D0_1 = term_A1 + term_B1;
    const T D0_2 = (tA * two_thirds - t3 * inv3) + c_t4_8;
    const T D0_3 = term_A3 + term_B3;

    T S0, S1, S2, S3;

    if (N == T(1)) {
      S0 = D0_0;
      S1 = D0_1;
      S2 = D0_2;
      S3 = D0_3;
    } else {
      const T c_tA_2 = tA * inv2;
      const T c_t2_2 = t2 * inv2;
      const T c_t3_2 = t3 * inv2;
      const T c_t2_3_4 = t2 * three_fourths;

      const T D1_0 = c_t3_6;
      const T D1_1 = (inv6 + c_tA_2) + (c_t2_2 - c_t3_2);
      const T D1_2 = (two_thirds - t2) + c_t3_2;
      const T D1_3 = (inv6 - c_tA_2) + (c_t2_2 - c_t3_6);

      const T D2_0 = c_t2_4;
      const T D2_1 = (inv4 + c_tA_2) - c_t2_3_4;
      const T D2_2 = c_t2_3_4 - tA;
      const T D2_3 = (c_tA_2 - inv4) - c_t2_4;

      const T D3_0 = c_tA_6;
      const T D3_1 = inv6 - c_tA_2;
      const T D3_2 = c_tA_2 - inv3;
      const T D3_3 = inv6 - c_tA_6;

      const T u = step / D;
      const T P0 = N;
      const T N_N_minus_1 = N * (N - T(1));
      const T P1 = N_N_minus_1 * inv2;
      const T P2 = P1 * (T(2) * N - T(1)) * inv3;
      const T P3 = P1 * P1;
      const T P4 = P2 * (T(3) * N_N_minus_1 - T(1)) * inv5;

      const T U1 = u * P1;
      const T u2 = u * u;
      const T U2 = u2 * P2;
      const T u3 = u2 * u;
      const T U3 = u3 * P3;
      const T u4 = u2 * u2;
      const T U4 = u4 * P4;

      const T S0_A = D0_0 * P0 + D1_0 * U1;
      const T S0_B = D2_0 * U2 + D3_0 * U3;
      S0 = (S0_A + S0_B) + inv24 * U4;

      const T S1_A = D0_1 * P0 + D1_1 * U1;
      const T S1_B = D2_1 * U2 + D3_1 * U3;
      S1 = (S1_A + S1_B) - inv8 * U4;

      const T S2_A = D0_2 * P0 + D1_2 * U1;
      const T S2_B = D2_2 * U2 + D3_2 * U3;
      S2 = (S2_A + S2_B) + inv8 * U4;

      const T S3_A = D0_3 * P0 + D1_3 * U1;
      const T S3_B = D2_3 * U2 + D3_3 * U3;
      S3 = (S3_A + S3_B) - inv24 * U4;
    }

    if (contains_zero) {
      const T t_z = -xa_p / D;
      const T tz2 = t_z * t_z;

      S0 -= (tz2 * tz2) * inv24;
      S1 -= t_z * (inv6 + t_z * (inv4 + t_z * (inv6 - t_z * inv8)));
      S2 -= t_z * (two_thirds + tz2 * (-inv3 + t_z * inv8));
      S3 -= t_z * (inv6 + t_z * (-inv4 + t_z * (inv6 - t_z * inv24)));
    }

    const T jump = step * a_;
    const T C_last = a_ * std::trunc(xb_p);

    const T C_last_inv24 = C_last * inv24;
    const T C_last_11_24 = C_last * eleven_24ths;

    i0 = C_last_inv24 - jump * S0;
    i1 = C_last_11_24 - jump * S1;
    i2 = C_last_11_24 - jump * S2;
    i3 = C_last_inv24 - jump * S3;
  }

public:
  TruncAdaa4() = default;
  explicit TruncAdaa4(T a) { set_a(a); }

  void set_a(T a) {
    a_ = a;
    inv_a_ = T(1) / a;
  }

  void reset() {
    out_buffer_[0] = T(0);
    out_buffer_[1] = T(0);
    out_buffer_[2] = T(0);
    prev_input_ = T(0);
  }

  T process(T input) {
    T i0, i1, i2, i3;

    const T xa_p = prev_input_ * inv_a_;
    const T xb_p = input * inv_a_;

    compute_interval(xa_p, xb_p, i0, i1, i2, i3);

    const T value = out_buffer_[0] + i3;
    out_buffer_[0] = out_buffer_[1] + i2;
    out_buffer_[1] = out_buffer_[2] + i1;
    out_buffer_[2] = i0;

    prev_input_ = input;
    return value;
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

template<typename Real> struct Trunc {
  static constexpr Real a = Real(1) / Real(16);

  static inline Real f0(Real x) { return a * std::trunc(x / a); }

  static inline Real f1(Real x) {
    Real trunc_val = a * std::trunc(x / a);
    Real delta = x - trunc_val;
    return Real(0.5) * (x * x - delta * delta - a * std::abs(trunc_val));
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
    Real si, ci;
    cephes::sici(std::exp(x), si, ci);
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

template<typename Real> class Saturator {
private:
  HardclipAdaa1<Real> hardclip1_;
  HardclipAdaa4<Real> hardclip4_;
  ModuloQuadAdaa4<Real> modulo_quad4_;
  TriangleAdaa4<Real> triangle4_;
  HalfwaveAdaa2<Real> halfrect2_;
  FullwaveAdaa2<Real> fullrect2_;
  TruncAdaa4<Real> trunc4_{Real(1) / Real(16)};

  static constexpr Real eps = std::numeric_limits<float>::epsilon();
  Real x1_ = 0;
  Real s1_ = 0;

  // 1st order antiderivative anti-aliasing (ADAA).
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
  X(versinc)             /* versinc. */                                                            \
  X(halfrect)            /* half-wave rectifier. */                                                \
  X(fullrect)            /* full-wave rectifier. */                                                \
  X(trunc)               /* decimal truncaction, bit-reducer. */                                   \
  X(trunc_cleaner)       /* decimal truncaction, bit-reducer. */

  enum class Function : unsigned {
#define X(name) name,
    UHHYOU_SATURATOR_FUNCTIONS(X)
#undef X
  };

  void reset() {
    hardclip1_.reset();
    hardclip4_.reset();
    modulo_quad4_.reset();
    triangle4_.reset();
    halfrect2_.reset();
    fullrect2_.reset();
    trunc4_.reset();

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
        return hardclip1_.process(input);
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
      case Function::halfrect:
        return halfrect2_.process(input);
      case Function::fullrect:
        return fullrect2_.process(input);
      case Function::trunc:
        return processInternal<Trunc<Real>>(input);
      case Function::trunc_cleaner:
        return trunc4_.process(input);
    }
  }
};

} // namespace Uhhyou
