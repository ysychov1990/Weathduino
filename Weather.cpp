#include "Weather.hpp"

// double temp;       // -22.0__41.5 step 0.5 - 7 bits
// double tempFeel;   // -64.0...63.5 step 0.5 - 8 bits
// int humid;         // 0__100 step 1 - 7 bits
// double precip;     // 0__25.6 - 8 bits
// double precipProb; // 0___100 - 7 bits
// double wind;       // 0___51.2 - 9 bits
// int cloud;         // 0___100 - 7 bits
//  total 8+8+7+8+7+9+7=24+21+9=54 bits
//  too much info, we should reduce bit count to something
//  manageable, 3 days of info per 24 hours is 72,
//  3 bytes of info per hour gives us:
//  temp: 7 bits, precip: 5 bits, precipProb: 4 bits,
//  cloud: 4 bits of info
std::bitset<22> Weather::to_bitset() {
  // temperature: -22 is our min, 41.5 is max, step 0.5
  uint32_t processed_t = static_cast<int>((temp + 22.) * 2 + 0.5);
  if (processed_t > 127)
    processed_t = 127; // 7 bits

  // precipitation 0__25.5, step 0.1
  uint32_t processed_p = static_cast<int>(precip * 10. + 0.5);
  if (processed_p > 255)
    processed_p = 255; // 8 bits

  // precipitation probability: 0__100 step 10
  uint32_t processed_pp = static_cast<int>(precipProb / 10. + 0.5);
  if (processed_pp > 10)
    processed_pp = 10; // 3.5 bits

  // cloud: 0__100 step 10
  uint32_t processed_c = static_cast<int>(cloud / 10. + 0.5);
  if (processed_c > 10)
    processed_c = 10; // 3.5 bits

  // now pack that all into one uint32_t value
  // 0...6 | 7...14 | 15...21
  uint32_t res = processed_t << (8 + 7) /*last bits*/ |
                 processed_p << 7 /*middle bits*/ |
                 (processed_pp * 11 + processed_c) /*first bits*/;
  //        std::cout << std::hex << res << std::dec << "\n";
  return std::bitset<22>(res);
}

// short string representation of HourlyWeather for Arduino
std::ostream &operator<<(std::ostream &outs, const Weather &w) {
  return outs << "[" << static_cast<int>(w.temp * 10) << "|"
              << static_cast<int>(w.tempFeel * 10) << "|" << w.humid << "|"
              << static_cast<int>(w.precip * 10) << "|"
              << static_cast<int>(w.precipProb * 10) << "|"
              << static_cast<int>(w.wind * 10) << "|" << w.cloud << "]";
}
