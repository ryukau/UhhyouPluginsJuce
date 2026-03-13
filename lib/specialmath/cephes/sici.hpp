/*							sici.c
 *
 *	Sine and cosine integrals
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, Ci, Si, sici();
 *
 * sici( x, &Si, &Ci );
 *
 *
 * DESCRIPTION:
 *
 * Evaluates the integrals
 *
 *                          x
 *                          -
 *                         |  cos t - 1
 *   Ci(x) = eul + ln x +  |  --------- dt,
 *                         |      t
 *                        -
 *                         0
 *             x
 *             -
 *            |  sin t
 *   Si(x) =  |  ----- dt
 *            |    t
 *           -
 *            0
 *
 * where eul = 0.57721566490153286061 is Euler's constant.
 * The integrals are approximated by rational functions.
 * For x > 8 auxiliary functions f(x) and g(x) are employed
 * such that
 *
 * Ci(x) = f(x) sin(x) - g(x) cos(x)
 * Si(x) = pi/2 - f(x) cos(x) - g(x) sin(x)
 *
 *
 * ACCURACY:
 *    Test interval = [0,50].
 * Absolute error, except relative when > 1:
 * arithmetic   function   # trials      peak         rms
 *    IEEE        Si        30000       4.4e-16     7.3e-17
 *    IEEE        Ci        30000       6.9e-16     5.1e-17
 *    DEC         Si         5000       4.4e-17     9.0e-18
 *    DEC         Ci         5300       7.9e-17     5.2e-18
 */

/*
Cephes Math Library Release 2.1:  January, 1989
Copyright 1984, 1987, 1989 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

/*							sicif.c
 *
 *	Sine and cosine integrals
 *
 *
 *
 * SYNOPSIS:
 *
 * float x, Ci, Si;
 *
 * sicif( x, &Si, &Ci );
 *
 *
 * DESCRIPTION:
 *
 * Evaluates the integrals
 *
 *                          x
 *                          -
 *                         |  cos t - 1
 *   Ci(x) = eul + ln x +  |  --------- dt,
 *                         |      t
 *                        -
 *                         0
 *             x
 *             -
 *            |  sin t
 *   Si(x) =  |  ----- dt
 *            |    t
 *           -
 *            0
 *
 * where eul = 0.57721566490153286061 is Euler's constant.
 * The integrals are approximated by rational functions.
 * For x > 8 auxiliary functions f(x) and g(x) are employed
 * such that
 *
 * Ci(x) = f(x) sin(x) - g(x) cos(x)
 * Si(x) = pi/2 - f(x) cos(x) - g(x) sin(x)
 *
 *
 * ACCURACY:
 *    Test interval = [0,50].
 * Absolute error, except relative when > 1:
 * arithmetic   function   # trials      peak         rms
 *    IEEE        Si        30000       2.1e-7      4.3e-8
 *    IEEE        Ci        30000       3.9e-7      2.2e-8
 */

/*
Cephes Math Library Release 2.1:  January, 1989
Copyright 1984, 1987, 1989 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

#pragma once

#include "util.hpp"

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <numbers>

namespace cephes {
namespace internal {

template<typename T> struct sici_data;

template<> struct sici_data<float> {
  static constexpr std::array<float, 6> SN = {
    -8.39167827910303881427E-11f, 4.62591714427012837309E-8f,  -9.75759303843632795789E-6f,
    9.76945438170435310816E-4f,   -4.13470316229406538752E-2f, 1.00000000000000000302E0f,
  };
  static constexpr std::array<float, 6> SD = {
    2.03269266195951942049E-12f, 1.27997891179943299903E-9f, 4.41827842801218905784E-7f,
    9.96412122043875552487E-5f,  1.42085239326149893930E-2f, 9.99999999999999996984E-1f,
  };
  static constexpr std::array<float, 6> CN = {
    2.02524002389102268789E-11f, -1.35249504915790756375E-8f, 3.59325051419993077021E-6f,
    -4.74007206873407909465E-4f, 2.89159652607555242092E-2f,  -1.00000000000000000080E0f,
  };
  static constexpr std::array<float, 6> CD = {
    4.07746040061880559506E-12f, 3.06780997581887812692E-9f, 1.23210355685883423679E-6f,
    3.17442024775032769882E-4f,  5.10028056236446052392E-2f, 4.00000000000000000080E0f,
  };
  static constexpr std::array<float, 7> FN4 = {
    4.23612862892216586994E0f,  5.45937717161812843388E0f,  1.62083287701538329132E0f,
    1.67006611831323023771E-1f, 6.81020132472518137426E-3f, 1.08936580650328664411E-4f,
    5.48900223421373614008E-7f,
  };
  static constexpr std::array<float, 7> FD4 = {
    8.16496634205391016773E0f,  7.30828822505564552187E0f,  1.86792257950184183883E0f,
    1.78792052963149907262E-1f, 7.01710668322789753610E-3f, 1.10034357153915731354E-4f,
    5.48900252756255700982E-7f,
  };
  static constexpr std::array<float, 9> FN8 = {
    4.55880873470465315206E-1f, 7.13715274100146711374E-1f,  1.60300158222319456320E-1f,
    1.16064229408124407915E-2f, 3.49556442447859055605E-4f,  4.86215430826454749482E-6f,
    3.20092790091004902806E-8f, 9.41779576128512936592E-11f, 9.70507110881952024631E-14f,
  };
  static constexpr std::array<float, 8> FD8 = {
    9.17463611873684053703E-1f,  1.78685545332074536321E-1f,  1.22253594771971293032E-2f,
    3.58696481881851580297E-4f,  4.92435064317881464393E-6f,  3.21956939101046018377E-8f,
    9.43720590350276732376E-11f, 9.70507110881952025725E-14f,
  };
  static constexpr std::array<float, 8> GN4 = {
    8.71001698973114191777E-2f, 6.11379109952219284151E-1f, 3.97180296392337498885E-1f,
    7.48527737628469092119E-2f, 5.38868681462177273157E-3f, 1.61999794598934024525E-4f,
    1.97963874140963632189E-6f, 7.82579040744090311069E-9f,
  };
  static constexpr std::array<float, 7> GD4 = {
    1.64402202413355338886E0f,  6.66296701268987968381E-1f, 9.88771761277688796203E-2f,
    6.22396345441768420760E-3f, 1.73221081474177119497E-4f, 2.02659182086343991969E-6f,
    7.82579218933534490868E-9f,
  };
  static constexpr std::array<float, 9> GN8 = {
    6.97359953443276214934E-1f, 3.30410979305632063225E-1f,  3.84878767649974295920E-2f,
    1.71718239052347903558E-3f, 3.48941165502279436777E-5f,  3.47131167084116673800E-7f,
    1.70404452782044526189E-9f, 3.85945925430276600453E-12f, 3.14040098946363334640E-15f,
  };
  static constexpr std::array<float, 9> GD8 = {
    1.68548898811011640017E0f,  4.87852258695304967486E-1f,  4.67913194259625806320E-2f,
    1.90284426674399523638E-3f, 3.68475504442561108162E-5f,  3.57043223443740838771E-7f,
    1.72693748966316146736E-9f, 3.87830166023954706752E-12f, 3.14040098946363335242E-15f,
  };
};

template<> struct sici_data<double> {
  static constexpr std::array<double, 6> SN = {
    -8.39167827910303881427E-11, 4.62591714427012837309E-8,  -9.75759303843632795789E-6,
    9.76945438170435310816E-4,   -4.13470316229406538752E-2, 1.00000000000000000302E0,
  };
  static constexpr std::array<double, 6> SD = {
    2.03269266195951942049E-12, 1.27997891179943299903E-9, 4.41827842801218905784E-7,
    9.96412122043875552487E-5,  1.42085239326149893930E-2, 9.99999999999999996984E-1,
  };
  static constexpr std::array<double, 6> CN = {
    2.02524002389102268789E-11, -1.35249504915790756375E-8, 3.59325051419993077021E-6,
    -4.74007206873407909465E-4, 2.89159652607555242092E-2,  -1.00000000000000000080E0,
  };
  static constexpr std::array<double, 6> CD = {
    4.07746040061880559506E-12, 3.06780997581887812692E-9, 1.23210355685883423679E-6,
    3.17442024775032769882E-4,  5.10028056236446052392E-2, 4.00000000000000000080E0,
  };
  static constexpr std::array<double, 7> FN4 = {
    4.23612862892216586994E0,  5.45937717161812843388E0,  1.62083287701538329132E0,
    1.67006611831323023771E-1, 6.81020132472518137426E-3, 1.08936580650328664411E-4,
    5.48900223421373614008E-7,
  };
  static constexpr std::array<double, 7> FD4 = {
    8.16496634205391016773E0,  7.30828822505564552187E0,  1.86792257950184183883E0,
    1.78792052963149907262E-1, 7.01710668322789753610E-3, 1.10034357153915731354E-4,
    5.48900252756255700982E-7,
  };
  static constexpr std::array<double, 9> FN8 = {
    4.55880873470465315206E-1, 7.13715274100146711374E-1,  1.60300158222319456320E-1,
    1.16064229408124407915E-2, 3.49556442447859055605E-4,  4.86215430826454749482E-6,
    3.20092790091004902806E-8, 9.41779576128512936592E-11, 9.70507110881952024631E-14,
  };
  static constexpr std::array<double, 8> FD8 = {
    9.17463611873684053703E-1,  1.78685545332074536321E-1,  1.22253594771971293032E-2,
    3.58696481881851580297E-4,  4.92435064317881464393E-6,  3.21956939101046018377E-8,
    9.43720590350276732376E-11, 9.70507110881952025725E-14,
  };
  static constexpr std::array<double, 8> GN4 = {
    8.71001698973114191777E-2, 6.11379109952219284151E-1, 3.97180296392337498885E-1,
    7.48527737628469092119E-2, 5.38868681462177273157E-3, 1.61999794598934024525E-4,
    1.97963874140963632189E-6, 7.82579040744090311069E-9,
  };
  static constexpr std::array<double, 7> GD4 = {
    1.64402202413355338886E0,  6.66296701268987968381E-1, 9.88771761277688796203E-2,
    6.22396345441768420760E-3, 1.73221081474177119497E-4, 2.02659182086343991969E-6,
    7.82579218933534490868E-9,
  };
  static constexpr std::array<double, 9> GN8 = {
    6.97359953443276214934E-1, 3.30410979305632063225E-1,  3.84878767649974295920E-2,
    1.71718239052347903558E-3, 3.48941165502279436777E-5,  3.47131167084116673800E-7,
    1.70404452782044526189E-9, 3.85945925430276600453E-12, 3.14040098946363334640E-15,
  };
  static constexpr std::array<double, 9> GD8 = {
    1.68548898811011640017E0,  4.87852258695304967486E-1,  4.67913194259625806320E-2,
    1.90284426674399523638E-3, 3.68475504442561108162E-5,  3.57043223443740838771E-7,
    1.72693748966316146736E-9, 3.87830166023954706752E-12, 3.14040098946363335242E-15,
  };
};

} // namespace internal

/**
 * @brief Sine and cosine integral (templated)
 * @tparam T float or double
 */
template<std::floating_point T> inline void sici(T x, T& si, T& ci) {
  using namespace internal;
  using Data = sici_data<T>;

  constexpr T PIO2 = std::numbers::pi_v<T> / T(2);
  constexpr T EUL = std::numbers::egamma_v<T>;
  constexpr T INF = std::numeric_limits<T>::infinity();

  T abs_x = std::abs(x);

  if (abs_x == T(0)) {
    si = std::copysign(T(0), x);
    ci = -INF;
    return;
  }

  if (abs_x > T(1e7)) {
    if (std::isfinite(abs_x)) {
      T inv_x = T(1) / abs_x;
      T s = std::sin(abs_x);
      T c = std::cos(abs_x);
      si = PIO2 - inv_x * (c + s * inv_x);
      ci = inv_x * (s - c * inv_x);
    } else {
      si = PIO2;
      ci = T(0);
    }
    si = std::copysign(si, x);
    return;
  }

  if (abs_x <= T(4)) {
    T z = abs_x * abs_x;
    T s = abs_x * polevl(z, Data::SN) / polevl(z, Data::SD);
    T c = z * polevl(z, Data::CN) / polevl(z, Data::CD);

    si = std::copysign(s, x);
    ci = EUL + std::log(abs_x) + c;
    return;
  }

  T s = std::sin(abs_x);
  T c = std::cos(abs_x);
  T z = T(1) / (abs_x * abs_x);
  T f, g;

  if (abs_x < T(8)) {
    f = polevl(z, Data::FN4) / (abs_x * p1evl(z, Data::FD4));
    g = z * polevl(z, Data::GN4) / p1evl(z, Data::GD4);
  } else {
    f = polevl(z, Data::FN8) / (abs_x * p1evl(z, Data::FD8));
    g = z * polevl(z, Data::GN8) / p1evl(z, Data::GD8);
  }

  si = std::copysign(PIO2 - f * c - g * s, x);
  ci = f * s - g * c;
}

template<std::floating_point T> inline T Si(T x) {
  using namespace internal;
  using Data = sici_data<T>;

  constexpr T PIO2 = std::numbers::pi_v<T> / T(2);

  T abs_x = std::abs(x);

  if (abs_x == T(0)) { return std::copysign(T(0), x); }

  if (abs_x > T(1e7)) {
    if (std::isfinite(abs_x)) {
      T inv_x = T(1) / abs_x;
      T s = std::sin(abs_x);
      T c = std::cos(abs_x);
      return std::copysign(PIO2 - inv_x * (c + s * inv_x), x);
    }
    // Infinite case: returns +/- PIO2 based on sign of x
    return std::copysign(PIO2, x);
  }

  if (abs_x <= T(4)) {
    T z = abs_x * abs_x;
    T s = abs_x * polevl(z, Data::SN) / polevl(z, Data::SD);
    return std::copysign(s, x);
  }

  T z = T(1) / (abs_x * abs_x);
  T f, g;
  if (abs_x < T(8)) {
    f = polevl(z, Data::FN4) / (abs_x * p1evl(z, Data::FD4));
    g = z * polevl(z, Data::GN4) / p1evl(z, Data::GD4);
  } else {
    f = polevl(z, Data::FN8) / (abs_x * p1evl(z, Data::FD8));
    g = z * polevl(z, Data::GN8) / p1evl(z, Data::GD8);
  }

  T si = PIO2 - f * std::cos(abs_x) - g * std::sin(abs_x);
  return std::copysign(si, x);
}

template<std::floating_point T> inline T Ci(T x) {
  using namespace internal;
  using Data = sici_data<T>;

  constexpr T EUL = std::numbers::egamma_v<T>;
  constexpr T INF = std::numeric_limits<T>::infinity();

  T abs_x = std::abs(x);

  if (abs_x == T(0)) { return -INF; }

  if (abs_x > T(1e7)) {
    if (std::isfinite(abs_x)) {
      T inv_x = T(1) / abs_x;
      T s = std::sin(abs_x);
      T c = std::cos(abs_x);
      return inv_x * (s - c * inv_x);
    }
    if (x < T(0)) { return std::numeric_limits<T>::quiet_NaN(); }
    return T(0);
  }

  if (abs_x <= T(4)) {
    T z = abs_x * abs_x;
    T c = z * polevl(z, Data::CN) / polevl(z, Data::CD);
    return EUL + std::log(abs_x) + c;
  }

  T z = T(1) / (abs_x * abs_x);
  T f, g;
  if (abs_x < T(8)) {
    f = polevl(z, Data::FN4) / (abs_x * p1evl(z, Data::FD4));
    g = z * polevl(z, Data::GN4) / p1evl(z, Data::GD4);
  } else {
    f = polevl(z, Data::FN8) / (abs_x * p1evl(z, Data::FD8));
    g = z * polevl(z, Data::GN8) / p1evl(z, Data::GD8);
  }
  return f * std::sin(abs_x) - g * std::cos(abs_x);
}

} // namespace cephes
