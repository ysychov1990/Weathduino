#pragma once
#include <chrono>
#include "Weather.hpp"

// shortcut to use std::chrono, minutes
using chrono_minutes =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>;

class NowWeather : public Weather {
public:
  chrono_minutes nowTime;
  std::bitset<48> to_bitset();
};
