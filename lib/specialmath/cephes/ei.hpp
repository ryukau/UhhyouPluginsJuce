// C++17 or later provides `std::expint`. This is ported only because Apple Clang doesn't
// provide special math functions. boost.math also has Ei.

/*							ei.c
 *
 *	Exponential integral
 *
 *
 * SYNOPSIS:
 *
 * double x, y, ei();
 *
 * y = ei( x );
 *
 *
 *
 * DESCRIPTION:
 *
 *               x
 *                -     t
 *               | |   e
 *    Ei(x) =   -|-   ---  dt .
 *             | |     t
 *              -
 *             -inf
 *
 * Not defined for x <= 0.
 * See also expn.c.
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE       0,100       50000      8.6e-16     1.3e-16
 *
 */
/*
Cephes Math Library Release 2.8:  May, 1999
Copyright 1999 by Stephen L. Moshier
*/

#pragma once

#include "util.hpp"

#include <array>
#include <cmath>
#include <concepts>
#include <limits>
#include <numbers>

namespace cephes {
namespace internal {

template<std::floating_point T> struct ei_data {
  static constexpr std::array<double, 6> A = {
    T(-5.350447357812542947283E0), T(2.185049168816613393830E2),  T(-4.176572384826693777058E3),
    T(5.541176756393557601232E4),  T(-3.313381331178144034309E5), T(1.592627163384945414220E6),
  };
  static constexpr std::array<double, 6> B = {
    T(-5.250547959112862969197E1), T(1.259616186786790571525E3),  T(-1.756549581973534652631E4),
    T(1.493062117002725991967E5),  T(-7.294949239640527645655E5), T(1.592627163384945429726E6),
  };

  static constexpr std::array<double, 8> A6 = {
    T(1.981808503259689673238E-2),  T(-1.271645625984917501326E0),  T(-2.088160335681228318920E0),
    T(2.755544509187936721172E0),   T(-4.409507048701600257171E-1), T(4.665623805935891391017E-2),
    T(-1.545042679673485262580E-3), T(7.059980605299617478514E-5),
  };
  static constexpr std::array<double, 7> B6 = {
    T(1.476498670914921440652E0),  T(5.629177174822436244827E-1), T(1.699017897879307263248E-1),
    T(2.291647179034212017463E-2), T(4.450150439728752875043E-3), T(1.727439612206521482874E-4),
    T(3.953167195549672482304E-5),
  };

  static constexpr std::array<double, 8> A5 = {
    T(-1.373215375871208729803E0),  T(-7.084559133740838761406E-1), T(1.580806855547941010501E0),
    T(-2.601500427425622944234E-1), T(2.994674694113713763365E-2),  T(-1.038086040188744005513E-3),
    T(4.371064420753005429514E-5),  T(2.141783679522602903795E-6),
  };
  static constexpr std::array<double, 8> B5 = {
    T(8.585231423622028380768E-1),  T(4.483285822873995129957E-1), T(7.687932158124475434091E-2),
    T(2.449868241021887685904E-2),  T(8.832165941927796567926E-4), T(4.590952299511353531215E-4),
    T(-4.729848351866523044863E-6), T(2.665195537390710170105E-6),
  };

  static constexpr std::array<double, 10> A2 = {
    T(-2.106934601691916512584E0),  T(1.732733869664688041885E0),   T(-2.423619178935841904839E-1),
    T(2.322724180937565842585E-2),  T(2.372880440493179832059E-4),  T(-8.343219561192552752335E-5),
    T(1.363408795605250394881E-5),  T(-3.655412321999253963714E-7), T(1.464941733975961318456E-8),
    T(6.176407863710360207074E-10),
  };
  static constexpr std::array<double, 9> B2 = {
    T(-2.298062239901678075778E-1), T(1.105077041474037862347E-1),  T(-1.566542966630792353556E-2),
    T(2.761106850817352773874E-3),  T(-2.089148012284048449115E-4), T(1.708528938807675304186E-5),
    T(-4.459311796356686423199E-7), T(1.394634930353847498145E-8),  T(6.150865933977338354138E-10),
  };

  static constexpr std::array<double, 8> A4 = {
    T(-2.458119367674020323359E-1), T(-1.483382253322077687183E-1), T(7.248291795735551591813E-2),
    T(-1.348315687380940523823E-2), T(1.342775069788636972294E-3),  T(-7.942465637159712264564E-5),
    T(2.644179518984235952241E-6),  T(-4.239473659313765177195E-8),
  };
  static constexpr std::array<double, 8> B4 = {
    T(-1.044225908443871106315E-1), T(-2.676453128101402655055E-1), T(9.695000254621984627876E-2),
    T(-1.601745692712991078208E-2), T(1.496414899205908021882E-3),  T(-8.462452563778485013756E-5),
    T(2.728938403476726394024E-6),  T(-4.239462431819542051337E-8),
  };

  static constexpr std::array<double, 6> A7 = {
    T(1.212561118105456670844E-1),  T(-5.823133179043894485122E-1), T(2.348887314557016779211E-1),
    T(-3.040034318113248237280E-2), T(1.510082146865190661777E-3),  T(-2.523137095499571377122E-5),
  };
  static constexpr std::array<double, 5> B7 = {
    T(-1.002252150365854016662E0), T(2.928709694872224144953E-1),  T(-3.337004338674007801307E-2),
    T(1.560544881127388842819E-3), T(-2.523137093603234562648E-5),
  };

  static constexpr std::array<double, 9> A3 = {
    T(-7.657847078286127362028E-1), T(6.886192415566705051750E-1),  T(-2.132598113545206124553E-1),
    T(3.346107552384193813594E-2),  T(-3.076541477344756050249E-3), T(1.747119316454907477380E-4),
    T(-6.103711682274170530369E-6), T(1.218032765428652199087E-7),  T(-1.086076102793290233007E-9),
  };
  static constexpr std::array<double, 9> B3 = {
    T(-1.888802868662308731041E0),  T(1.066691687211408896850E0),   T(-2.751915982306380647738E-1),
    T(3.930852688233823569726E-2),  T(-3.414684558602365085394E-3), T(1.866844370703555398195E-4),
    T(-6.345146083130515357861E-6), T(1.239754287483206878024E-7),  T(-1.086076102793126632978E-9),
  };
};

} // namespace internal

/**
 * @brief Exponential integral Ei(x)
 * * Computes the exponential integral defined as:
 * $$ Ei(x) = \int_{-\infty}^{x} \frac{e^t}{t} dt $$
 * * @tparam T floating point type (float or double)
 * @param x Input argument
 * @return Exponential integral value. Returns NaN for x < 0.
 */
template<std::floating_point T> T ei(T x) {
  using namespace internal;

  if (x == T(0)) { return -std::numeric_limits<T>::infinity(); }
  if (x < T(0)) { return std::numeric_limits<T>::quiet_NaN(); }

  constexpr T EUL = std::numbers::egamma_v<T>;

  if (x < T(2)) {
    T f = polevl(x, ei_data<T>::A) / p1evl(x, ei_data<T>::B);
    return (EUL + std::log(x) + x * f);
  }

  T w = T(1) / x;
  T f;
  if (x < T(4)) {
    f = polevl(w, ei_data<T>::A6) / p1evl(w, ei_data<T>::B6);
  } else if (x < T(8)) {
    f = polevl(w, ei_data<T>::A5) / p1evl(w, ei_data<T>::B5);
  } else if (x < T(16)) {
    f = polevl(w, ei_data<T>::A2) / p1evl(w, ei_data<T>::B2);
  } else if (x < T(32)) {
    f = polevl(w, ei_data<T>::A4) / p1evl(w, ei_data<T>::B4);
  } else if (x < T(64)) {
    f = polevl(w, ei_data<T>::A7) / p1evl(w, ei_data<T>::B7);
  } else {
    f = polevl(w, ei_data<T>::A3) / p1evl(w, ei_data<T>::B3);
  }
  return (std::exp(x) * w * (T(1) + w * f));
}

} // namespace cephes
