#include "./jyzo.hpp"

#include <format>
#include <fstream>
#include <iostream>
#include <string>

template<typename T> std::string strFromVec(std::vector<T> &vec)
{
  std::string text = "[";
  for (const auto &x : vec) text += std::format("{},", x);
  text.pop_back();
  text += "]";
  return text;
}

int main()
{
  constexpr int nt = 8;
  std::string text = "[\n";
  for (int n = 0; n < 16; ++n) {
    auto [rj0, rj1, ry0, ry1] = Uhhyou::jyzo(n, nt);
    text += std::format(
      "{{\"n\": {},\n\"nt\": {},\n\"rj0\": {},\n\"rj1\": {},\n\"ry0\": {},\n\"ry1\": "
      "{}\n}},\n",
      n, nt, strFromVec(rj0), strFromVec(rj1), strFromVec(ry0), strFromVec(ry1));
  }
  text.erase(text.end() - 2);
  std::cout << text << "]\n";
  return 0;
}
