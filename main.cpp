#include <bitset>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unistd.h>

#include "ArduinoSerial.hpp"
#include "NowWeather.hpp"
#include "Weather.hpp"

// shortcut to use std::chrono, minutes
using chrono_minutes =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>;
using chrono_date =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::days>;

inline chrono_minutes str2minutes(const std::string &s) {
  std::istringstream iss{s};
  chrono_minutes cm;

  // parse into minutes
  iss >> std::chrono::parse("%Y-%m-%dT%H:%M", cm);
  if (iss.fail())
    throw std::runtime_error("Parsing failed for: " + s);

  return cm;
}
/*inline std::string minutes2str(const chrono_minutes &cm) {
  return std::format(std::locale("C"), "%H:%M", cm);
}
inline std::string date2str(const chrono_date &cd) {
  return std::format(std::locale("C"), "%Y-%m-%d", cd);
}*/

int main() {
  std::string responseString{};
  { // Fetch Weather Data
    CURL *curl = curl_easy_init();
    std::ostringstream ossurl; // request
    // Szczecin latitude, longiture
    const double latitude{50.500202}, longitude{14.603476};
    const std::string hourly_params{
        "temperature_2m,apparent_temperature,precipitation,"
        "precipitation_probability,relative_humidity_2m,"
        "cloud_cover_mid,wind_speed_10m"};
    const std::string current_params{
        "temperature_2m,apparent_temperature,precipitation,"
        "precipitation_probability,relative_humidity_2m,"
        "cloud_cover,wind_speed_10m,"
        "rain,showers,snowfall,weather_code"}; // for later...
    ossurl << "https://api.open-meteo.com/v1/forecast"
           << "?latitude=" << std::fixed << std::setprecision(6) << latitude
           << "&longitude=" << longitude << "&hourly=" << hourly_params
           << "&current=" << current_params << "&timezone=Europe%2FBerlin";
    //    std::cout << ossurl.str() << "\n\n";

    if (!curl) {
      std::cerr << "Error: can't get curl connection!\n";
      return 1;
    }

    std::ostringstream osresponse{};
    curl_easy_setopt(curl, CURLOPT_URL, ossurl.str().c_str());
    // lambda callback for CURL to write received data into a stringstream
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[](char *d, size_t s, size_t n, std::ostringstream *oss) -> size_t {
          oss->write(d, s * n);
          return s * n;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &osresponse);
    int perform_curl = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // check whether curl query has performed well
    if (perform_curl != CURLE_OK) {
      std::cerr << "Error while performing curl request!\n";
      return 1;
    }
    responseString = osresponse.str();
  }

  NowWeather nw; // this is current weather
  // this is our forecast
  //  map chrono_date --> (map hour --> weather)
  std::map<chrono_date, std::map<std::chrono::hours, Weather>> hourWeather;

  { // Parse JSON using nlohmann/json and fill our Weather classes
    using json = nlohmann::json;
    auto data = json::parse(responseString);

    // get units
    std::string currTempUnit = data["current_units"]["temperature_2m"],
                currTempFeelUnit =
                    data["current_units"]["apparent_temperature"],
                currHumidUnit = data["current_units"]["relative_humidity_2m"],
                currWindUnit = data["current_units"]["wind_speed_10m"],
                currPrecipUnit = data["current_units"]["precipitation"],
                currCloudUnit = data["current_units"]["cloud_cover"];
    // get values
    nw.nowTime = str2minutes(data["current"]["time"].get<std::string>());
    nw.temp = data["current"]["temperature_2m"];
    nw.tempFeel = data["current"]["apparent_temperature"];
    nw.humid = data["current"]["relative_humidity_2m"];
    nw.precip = data["current"]["precipitation"];
    nw.precipProb = data["current"]["precipitation_probability"];
    nw.wind = data["current"]["wind_speed_10m"];
    nw.cloud = data["current"]["cloud_cover"];
    [[maybe_unused]] int code = data["current"]["weather_code"];
    [[maybe_unused]] double rain = data["current"]["rain"];
    [[maybe_unused]] double showers = data["current"]["showers"];
    [[maybe_unused]] double snowfall = data["current"]["snowfall"];
    nw.to_bitset();
#ifdef DEBUG
    std::cout << "Teraz: " << nw.temp << currTempUnit << ", odczuwalnie "
              << nw.tempFeel << currTempFeelUnit << ", wilgotność: " << nw.humid
              << currHumidUnit << ",\n"
              << "wiatr: " << nw.wind << currWindUnit
              << ", opady: " << nw.precip << currPrecipUnit
              << ", zachmurzenie: " << nw.cloud << currCloudUnit << "\n\n";

#endif
    for (auto &el : data["hourly"]["time"]) {
      static int counter = 0;
      auto tt = str2minutes(el.get<std::string>());
      auto hourDate = std::chrono::time_point_cast<std::chrono::days>(tt);
      auto hourNum = std::chrono::hh_mm_ss(tt - hourDate).hours();

      // check if we already have this date in our map
      auto search = hourWeather.find(hourDate);
      if (search == hourWeather.end())
        hourWeather[hourDate] = std::map<std::chrono::hours, Weather>();

      auto w = Weather();
      // read hourly params
      w.temp = data["hourly"]["temperature_2m"][counter];
      w.tempFeel = data["hourly"]["apparent_temperature"][counter];
      w.humid = data["hourly"]["relative_humidity_2m"][counter];
      w.precip = data["hourly"]["precipitation"][counter];
      w.precipProb = data["hourly"]["precipitation_probability"][counter];
      w.wind = data["hourly"]["wind_speed_10m"][counter];
      w.cloud = data["hourly"]["cloud_cover_mid"][counter];
      // add new element
      hourWeather[hourDate][hourNum] = w;
#ifdef DEBUG
      std::cout << hourDate << "->" << hourNum << "->";
      std::cout << w.temp << "|" << w.tempFeel << "|" << w.humid << "|"
                << w.precip << "|" << w.precipProb << "|" << w.wind << "|"
                << w.cloud << "\n";
#endif
      counter++;
    }
  }

  std::vector<unsigned char> outv_array[4]{};
  { // formulate our data for Arduino
    // 4 blocks of info
    // current weather, today's, tomorrow's,
    // and the next day's weather
    // Data structure: first char: block type ['0', '1', '2', '3']
    // second char - string block length,
    // then string, then bits of info
    // First block: current weather
    outv_array[0].clear();
    outv_array[0].push_back('0');
    std::string s =
        std::format(std::locale("C"), "{:%Y-%m-%d %H:%M}", nw.nowTime);
    outv_array[0].push_back(static_cast<unsigned char>(s.size()));
    std::copy(s.begin(), s.end(), std::back_inserter(outv_array[0]));
    auto b = nw.to_bitset();
    unsigned char out_char = 0, out_char_bit = 0;
    for (unsigned int i = 0; i < b.size(); i++) {
      out_char = out_char | b[i] << out_char_bit;
      if (out_char_bit == 7) {
        outv_array[0].push_back(out_char);
        out_char = out_char_bit = 0;
      } else
        out_char_bit++;
    }
    if (out_char_bit)
      outv_array[0].push_back(out_char);

    // 2nd, 3rd and 4th blocks
    for (int i = 1; i < 4; i++) {
      static auto it = hourWeather.begin();
      outv_array[i].clear();
      outv_array[i].push_back('0' + i);
      std::string s = std::format(std::locale("C"), "{:%Y-%m-%d}", it->first);
      outv_array[i].push_back(static_cast<unsigned char>(s.size()));
      std::copy(s.begin(), s.end(), std::back_inserter(outv_array[i]));
      out_char = out_char_bit = 0;
      for (auto &j : it->second) {
        auto b = j.second.to_bitset();
        for (unsigned int k = 0; k < b.size(); k++) {
          out_char = out_char | b[k] << out_char_bit;
          if (out_char_bit == 7) {
            outv_array[i].push_back(out_char);
            out_char = out_char_bit = 0;
          } else
            out_char_bit++;
        }
      }
      if (out_char_bit)
        outv_array[i].push_back(out_char);
      it++;
    }

    for (int i = 0; i < 4; i++) {
      std::cout << "\n\noutv_array[" << i << "]:size " << std::dec
                << outv_array[i].size() << "\n";
      for (unsigned int j = 0; j < outv_array[i].size(); j++) {
        std::cout << outv_array[i][j] << " 0x" << std::hex
                  << static_cast<unsigned int>(outv_array[i][j])
                  << (j % 8 == 7 ? "\n" : "\t");
      }
    }
    std::cout << "\n\n\n";
  }

  try {
    ArduinoSerial arduino {};
    // Wait a moment for the bootloader to finish.
    std::cout << "Waiting for Arduino to initialize...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // 3. Send the 4 Packets
    for (int i = 0; i < 4; ++i) {
      std::cout << "Sending packet " << i << " (" << outv_array[i].size()
                << " bytes)... ";

      // Write packet
      ssize_t written =
          write(arduino.getFd(), outv_array[i].data(), outv_array[i].size());
      if (written < 0) {
        std::cerr << "\nError writing to serial!\n";
        break;
      }

      // Wait for "OK i" acknowledgment
      char read_buf[32];
      std::string response;
      bool received_ok = false;
      int retry_count = 0;

      while (retry_count < 5 && !received_ok) {
        memset(read_buf, 0, sizeof(read_buf));
        int n = read(arduino.getFd(), read_buf, sizeof(read_buf) - 1);

        if (n > 0) {
          response += std::string(read_buf, n);
          std::string expected = "OK " + std::to_string(i);
          if (response.find(expected) != std::string::npos) {
            std::cout << "Success: " << expected << "\n";
            received_ok = true;
          }
        }
        usleep(100000); // Wait 100ms before checking again
        retry_count++;
      }

      if (!received_ok) {
        std::cerr << "Timeout or wrong response. Received: " << response
                  << "\n";
        // Decide if you want to break or continue here
      }
    }
  } catch (std::runtime_error &e) {
    std::cout << "Runtime error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Done. Press any key to continue. ";
  std::cin.get();
  return 0;
}
