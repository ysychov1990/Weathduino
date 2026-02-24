#include <fcntl.h>
#include <fstream>
#include <iostream> // tbr
#include <unistd.h>

#include "ArduinoSerial.hpp"

ArduinoSerial::ArduinoSerial() : ArduinoSerial(findArduino()) {}

ArduinoSerial::ArduinoSerial(std::filesystem::path p) {
  // get file descriptor
  std::cout << "opening " << p.string() << "...\n";
  fd = open(p.c_str(), O_RDWR);
  std::cout << "fd = " << fd << "\n";
  std::cout << "this->fd = " << this->fd << "\n";

  if (fd < 0)
    throw std::runtime_error("Failed to open port!");

  // configure serial port
  if (tcgetattr(fd, &tty) != 0)
    throw std::runtime_error("Error from tcgetattr()");

  cfsetospeed(&tty, B9600); // out speed
  cfsetispeed(&tty, B9600); // in speed

  // binary terminal:
  // 8-bit bytes
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  // cancel break signal
  tty.c_iflag &= ~IGNBRK;
  // no signal chars
  tty.c_lflag = 0;
  // no remapping, no delays
  tty.c_oflag = 0;
  // read doesn't block
  tty.c_cc[VMIN] = 0;
  // 1.0 seconds read timeout
  tty.c_cc[VTIME] = 10;
  // shut off xon/xoff ctrl
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  // ignore modem controls, enable reading
  tty.c_cflag |= (CLOCAL | CREAD);
  // shut off parity
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(this->fd, TCSANOW, &tty))
    throw std::runtime_error(
        std::string("Error from tcsetattr() " + std::to_string(errno)));
}

ArduinoSerial::~ArduinoSerial() {
  if (fd >= 0) {
    tcdrain(fd); // Wait for data to transmit
    close(fd);
    std::cout << "Serial port closed safely.\n"; // tbr
  }
}

// Finds filename for specific Arduino device
// returns string filename, or throws runtime_error
// when device is not found
std::filesystem::path ArduinoSerial::findArduino() {
  namespace fs = std::filesystem;

  fs::path devPath = "/dev";

  for (const auto &entry : fs::directory_iterator(devPath)) {
    fs::path filename = entry.path().filename();
    fs::path fullname = entry.path();

    // Filter for ttyACM devices
    if (filename.string().find("ttyACM") != 0)
      continue;
    // Path to hardware properties in sysfs
    // Usually: /sys/class/tty/ttyACM0/device/../
    fs::path sysBase = fs::path("/sys/class/tty/") / filename / "device/..";

    // Resolve symlink to get actual hardware path
    // like /sys/devices/pci0000:00/.../usb1/.../
    // similar to `symlink -f /sys/class/tty/ttyACM0/device/../`
    fs::path sysPath = fs::canonical(sysBase);

    // lambda to read single string
    auto readFsLine = [&sysPath](std::string f) -> std::string {
      std::string value;
      std::ifstream(sysPath / f) >> value;
      return value;
    };

    if (readFsLine("idVendor") == "2341" && readFsLine("idProduct") == "0043" &&
        readFsLine("manufacturer") == "Arduino")
      return entry.path();
  }
  throw std::runtime_error("No Arduino tty found!");
}
