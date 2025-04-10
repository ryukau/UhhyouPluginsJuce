/*
The comments on the funcions are copied from original Fortran90 implementation
(special_functions.f90) found in the link below.

- https://people.sc.fsu.edu/~jburkardt/f_src/special_functions/special_functions.html
*/

#pragma once
#include <array>
#include <cmath>
#include <vector>

namespace Uhhyou {

//*****************************************************************************80
//
//! JYNDD: Bessel functions Jn(x) and Yn(x), first and second derivatives.
//
//  Licensing:
//
//    This routine is copyrighted by Shanjie Zhang and Jianming Jin.  However,
//    they give permission to incorporate this routine into a user program
//    provided that the copyright is acknowledged.
//
//  Modified:
//
//    02 August 2012
//
//  Author:
//
//    Shanjie Zhang, Jianming Jin
//
//  Reference:
//
//    Shanjie Zhang, Jianming Jin,
//    Computation of Special Functions,
//    Wiley, 1996,
//    ISBN: 0-471-11963-6,
//    LC: QA351.C45.
//
//  Parameters:
//
//    Input, integer N, the order.
//
//    Input, real ( kind = rk ) X, the argument.
//
//    Output, real ( kind = rk ) BJN, DJN, FJN, BYN, DYN, FYN, the values of
//    Jn(x), Jn'(x), Jn"(x), Yn(x), Yn'(x), Yn"(x).
//
std::array<double, 6> jyndd(int n, double x)
{
  double f;

  int nt;
  for (nt = 1; nt <= 900; ++nt) {
    int mt = int(0.5 * std::log10(6.28 * nt) - nt * std::log10(1.36 * std::abs(x) / nt));
    if (20 < mt) break;
  }

  std::vector<double> bj(nt, 0.0);
  std::vector<double> by(nt, 0.0);

  double bs = 0.0;
  double f0 = 0.0;
  double f1 = 1.0e-35;
  double su = 0.0;
  for (int k = nt; k >= 0; --k) {
    f = 2.0 * (k + 1.0) * f1 / x - f0;
    if (k <= n + 1) bj[k] = f;
    if (k == 2 * (k / 2)) {
      bs = bs + 2.0 * f;
      if (k != 0) su += (k / 2) % 2 == 0 ? f / k : -f / k;
    }
    f0 = f1;
    f1 = f;
  }

  for (int k = 0; k <= n + 1; ++k) bj[k] = bj[k] / (bs - f);

  double bjn = bj[n];
  double ec = 0.5772156649015329;
  double e0 = 0.3183098861837907;
  double s1 = 2.0 * e0 * (std::log(x / 2.0) + ec) * bj[0];
  f0 = s1 - 8.0 * e0 * su / (bs - f);
  f1 = (bj[1] * f0 - 2.0 * e0 / x) / bj[0];

  by[0] = f0;
  by[1] = f1;
  for (int k = 2; k <= n + 1; ++k) {
    f = 2.0 * (k - 1.0) * f1 / x - f0;
    by[k] = f;
    f0 = f1;
    f1 = f;
  }

  double byn = by[n];
  double djn = -bj[n + 1] + n * bj[n] / x;
  double dyn = -by[n + 1] + n * by[n] / x;
  double fjn = (n * n / (x * x) - 1.0) * bjn - djn / x;
  double fyn = (n * n / (x * x) - 1.0) * byn - dyn / x;

  return {bjn, djn, fjn, byn, dyn, fyn};
}

//*****************************************************************************80
//
//! JYZO computes the zeros of Bessel functions Jn(x), Yn(x) and derivatives.
//
//  Licensing:
//
//    This routine is copyrighted by Shanjie Zhang and Jianming Jin.  However,
//    they give permission to incorporate this routine into a user program
//    provided that the copyright is acknowledged.
//
//  Modified:
//
//    28 July 2012
//
//  Author:
//
//    Shanjie Zhang, Jianming Jin
//
//  Reference:
//
//    Shanjie Zhang, Jianming Jin,
//    Computation of Special Functions,
//    Wiley, 1996,
//    ISBN: 0-471-11963-6,
//    LC: QA351.C45.
//
//  Parameters:
//
//    Input, integer N, the order of the Bessel functions.
//
//    Input, integer NT, the number of zeros.
//
//    Output, real ( kind = rk ) RJ0(NT), RJ1(NT), RY0(NT), RY1(NT), the zeros
//    of Jn(x), Jn'(x), Yn(x), Yn'(x).
//
std::array<std::vector<double>, 4> jyzo(int n, int nt)
{
  std::vector<double> rj0(nt, 0.0);
  std::vector<double> rj1(nt, 0.0);
  std::vector<double> ry0(nt, 0.0);
  std::vector<double> ry1(nt, 0.0);

  int n_r8 = n;

  double n_r8_pow_1_3 = std::pow(double(n_r8), double(0.33333));
  int n_r8_pow_2 = n_r8 * n_r8;

  double x = n <= 20 ? 2.82141 + 1.15859 * n_r8
                     : n + 1.85576 * n_r8_pow_1_3 + 1.03315 / n_r8_pow_1_3;

  int l = 0;
  while (true) {
    double x0 = x;
    auto [bjn, djn, fjn, byn, dyn, fyn] = jyndd(n, x);
    x = x - bjn / djn;

    if (1.0e-09 < std::abs(x - x0)) continue;

    rj0[l++] = x;
    x = x + 3.1416 + (0.0972 + 0.0679 * n_r8 - 0.000354 * n_r8_pow_2) / l;

    if (nt <= l) break;
  }

  x = n <= 20 ? 0.961587 + 1.07703 * n_r8
              : n_r8 + 0.80861 * n_r8_pow_1_3 + 0.07249 / n_r8_pow_1_3;

  if (n == 0) x = 3.8317;

  l = 0;
  while (true) {
    double x0 = x;
    auto [bjn, djn, fjn, byn, dyn, fyn] = jyndd(n, x);
    x = x - djn / fjn;
    if (1.0e-09 < std::abs(x - x0)) continue;
    rj1[l++] = x;
    x = x + 3.1416 + (0.4955 + 0.0915 * n_r8 - 0.000435 * n_r8_pow_2) / l;

    if (nt <= l) break;
  }

  x = n <= 20 ? 1.19477 + 1.08933 * n_r8
              : n_r8 + 0.93158 * n_r8_pow_1_3 + 0.26035 / n_r8_pow_1_3;

  l = 0;
  while (true) {
    double x0 = x;
    auto [bjn, djn, fjn, byn, dyn, fyn] = jyndd(n, x);
    x = x - byn / dyn;

    if (1.0e-09 < std::abs(x - x0)) continue;

    ry0[l++] = x;
    x = x + 3.1416 + (0.312 + 0.0852 * n_r8 - 0.000403 * n_r8_pow_2) / l;

    if (nt <= l) break;
  }

  x = n <= 20 ? 2.67257 + 1.16099 * n_r8
              : n_r8 + 1.8211 * n_r8_pow_1_3 + 0.94001 / n_r8_pow_1_3;

  l = 0;
  while (true) {
    double x0 = x;
    auto [bjn, djn, fjn, byn, dyn, fyn] = jyndd(n, x);
    x = x - dyn / fyn;

    if (1.0e-09 < std::abs(x - x0)) continue;

    ry1[l++] = x;
    x = x + 3.1416 + (0.197 + 0.0643 * n_r8 - 0.000286 * n_r8_pow_2) / l;

    if (nt <= l) break;
  }

  return {rj0, rj1, ry0, ry1};
}

} // namespace Uhhyou
