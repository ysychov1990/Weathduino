#include <curl/curl.h>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <termios.h>
#include <chrono>
#include <ctime>
#include <unistd.h>

using json = nlohmann::json;

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
inline std::string minutes2str(const chrono_minutes &cm) {
  return std::format(std::locale("C"), "%H:%M", cm);
}
inline std::string date2str(const chrono_date &cd) {
  return std::format(std::locale("C"), "%Y-%m-%d", cd);
}

class Weather {
public:
  double temp;       // -51.2__51.1 step 0.1 - 10 bits
  double tempFeel;   // -64...63 step 1 - 7 bits
  int humid;         // 0__100 step 1 - 7 bits
  double precip;     // 0__25.6 - 8 bits
  double precipProb; // 0___100 - 7 bits
  double wind;       // 0___51.2 - 9 bits
  int cloud;         // 0___100 - 7 bits
  // total 35+14+9=49+9=58bits/8=8bytes
  // short string representation of HourlyWeather for Arduino
  friend std::ostream &operator<<(std::ostream &outs, const Weather &w) {
    return outs << "[" << static_cast<int>(w.temp * 10) << "|"
                << static_cast<int>(w.tempFeel * 10) << "|" << w.humid << "|"
                << static_cast<int>(w.precip * 10) << "|"
                << static_cast<int>(w.precipProb * 10) << "|"
                << static_cast<int>(w.wind * 10) << "|" << w.cloud << "]";
  }
};

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

  // this is current weather
  Weather nowWeather;
  chrono_minutes nowTime;
  // this is our forecast
  //  map std::chrono date --> (map hour --> weather)
  std::map<chrono_date, std::map<std::chrono::hours, Weather>> hourWeather;

  { // Parse JSON using nlohmann/json and fill our Weather classes
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
    //    nowTime = str2tm(data["current"]["time"].get<std::string>());
    nowTime = str2minutes(data["current"]["time"].get<std::string>());
    nowWeather.temp = data["current"]["temperature_2m"];
    nowWeather.tempFeel = data["current"]["apparent_temperature"];
    nowWeather.humid = data["current"]["relative_humidity_2m"];
    nowWeather.precip = data["current"]["precipitation"];
    nowWeather.precipProb = data["current"]["precipitation_probability"];
    nowWeather.wind = data["current"]["wind_speed_10m"];
    nowWeather.cloud = data["current"]["cloud_cover"];
    [[maybe_unused]] int code = data["current"]["weather_code"];
    [[maybe_unused]] double rain = data["current"]["rain"];
    [[maybe_unused]] double showers = data["current"]["showers"];
    [[maybe_unused]] double snowfall = data["current"]["snowfall"];

    std::cout << "Teraz: " << nowWeather.temp << currTempUnit
              << ", odczuwalnie " << nowWeather.tempFeel << currTempFeelUnit
              << ", wilgotność: " << nowWeather.humid << currHumidUnit << ",\n"
              << "wiatr: " << nowWeather.wind << currWindUnit
              << ", opady: " << nowWeather.precip << currPrecipUnit
              << ", zachmurzenie: " << nowWeather.cloud << currCloudUnit
              << "\n\n";

    for (auto &el : data["hourly"]["time"]) {
      static int counter = 0;
      //    std::cout << key << "->" << value << "\t";
      auto tt = str2minutes(el.get<std::string>());
      auto hourDate = std::chrono::time_point_cast<std::chrono::days>(tt);
      std::cout << tt << "|||" << hourDate << std::endl;
      auto hourNum = std::chrono::hh_mm_ss(tt - hourDate).hours();

      // check if we already have this date in our map
      auto search = hourWeather.find(hourDate);
      if (search == hourWeather.end()) {
        hourWeather[hourDate] = std::map<std::chrono::hours, Weather>();
        //        std::cout << "\n" << hourDate << ":\n";
      }
      Weather w = Weather();
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
      std::cout << hourNum << "->";
      std::cout << el.get<std::string>() << "->";
      std::cout << w.temp << "|" << w.tempFeel << "|" << w.humid << "|"
                << w.precip << "|" << w.precipProb << "|" << w.wind << "|";
      std::cout << "|||counter: " << counter << "\n" << w.cloud;
      counter++;
    }
  }

  std::string outputString{};
  { // formulate our data for Arduino
    std::ostringstream oss;
    // First block: current weather
    // DateTime:[Weather]
    oss << minutes2str(nowTime) << nowWeather << "\n";
    // Other blocks: today:24 weather blocks, and the same
    // for tomorrow and day after
    for (int i = 0; i < 3; i++) {
      static auto it = hourWeather.begin();
      if (i)
        oss << "\n";
      oss << it->first;
      for (auto j = it->second.begin(); j != it->second.end(); j++)
        oss << j->second;
      it++;
    }
    outputString = oss.str();
  }

  // 1. Setup Serial Connection to Arduino
  const char *port = "/dev/ttyACM0";
  int serial_dev = open(port, O_RDWR);

  write(serial_dev, outputString.c_str(), outputString.length());
  //    std::cout << "Sent to Arduino: " << payload << std::endl;

  // Simple Serial Configuration (Baud 9600)
  struct termios tty;
  tcgetattr(serial_dev, &tty);
  cfsetospeed(&tty, B9600);
  tcdrain(serial_dev);
  tcsetattr(serial_dev, TCSANOW, &tty);

  close(serial_dev);
  std::cin.get();
  return 0;
}