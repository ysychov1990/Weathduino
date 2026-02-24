#pragma once

#include <bitset>
#include <cstdint>
#include <ostream>

class Weather {
public:
  double temp;       // -22.0__41.5 step 0.5 - 7 bits
  double tempFeel;   // -64.0...63.5 step 0.5 - 8 bits
  int humid;         // 0__100 step 1 - 7 bits
  double precip;     // 0__25.6 - 8 bits
  double precipProb; // 0___100 - 7 bits
  double wind;       // 0___51.2 - 9 bits
  int cloud;         // 0___100 - 7 bits
  // total 8+8+7+8+7+9+7=24+21+9=54 bits
  // too much info, we should reduce bit count to something
  // manageable, 3 days of info per 24 hours is 72,
  // 3 bytes of info per hour gives us:
  // temp: 7 bits, precip: 5 bits, precipProb: 4 bits,
  // cloud: 4 bits of info
  std::bitset<22> to_bitset();

  // short string representation of HourlyWeather for Arduino
  // to be removed, not used anymore
  friend std::ostream &operator<<(std::ostream &outs, const Weather &w);
};
