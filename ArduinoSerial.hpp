#pragma once

#include <filesystem>
#include <termios.h>

class ArduinoSerial {
private:
  int fd;
  termios tty;

public:
  ArduinoSerial();
  ArduinoSerial(std::filesystem::path p);
  ~ArduinoSerial();
  const int & getFd() { return fd; };
  static std::filesystem::path findArduino();
};
