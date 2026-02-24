#include "NowWeather.hpp"

std::bitset<48> NowWeather::to_bitset() {
    // temperature: -51.2 is our min, 51.1 is max, step 0.1
    uint64_t processed_t = static_cast<int>((temp + 51.2) * 10 + 0.5);
    if (processed_t > 1023)
      processed_t = 1023; // 10 bits

    // tempFeel: -51.2 is our min, 51.1 is max, step 0.1
    uint64_t processed_tf = static_cast<int>((tempFeel + 51.2) * 10 + 0.5);
    if (processed_tf > 1023)
      processed_tf = 1023; // 10 bits

    // humidity: 0__100
    uint32_t processed_h = humid;
    if (processed_h > 100)
      processed_h = 100; // 7 bits

    // precipitation 0__25.5 step 0.1
    uint32_t processed_p = static_cast<int>(precip * 10. + 0.5);
    if (processed_p > 31)
      processed_p = 31; // 5 bits

    // precipitation probability: 0__100 step 10
    uint32_t processed_pp = static_cast<int>(precipProb / 10. + 0.5);
    if (processed_pp > 10)
      processed_pp = 10; // 3.5 bits

    uint32_t processed_w = static_cast<int>(wind * 10.);
    if (processed_w > 511)
      processed_w = 511; // 9 bits

    // cloud: 0__100 step 10
    uint32_t processed_c = static_cast<int>(cloud / 10. + 0.5);
    if (processed_c > 10)
      processed_c = 10; // 3.5 bits

    // now pack that all into one uint64_t value
    int64_t res = processed_t << (7 + 9 + 5 + 7 + 10) | //
                  processed_tf << (7 + 9 + 5 + 7) |     //
                  processed_h << (7 + 9 + 5) |          //
                  processed_p << (7 + 9) |              //
                  processed_w << 7 |                    //
                  (processed_pp * 11 + processed_c);
    return std::bitset<48>(res);
  }
  
