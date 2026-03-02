struct DayWeather {
  static const int maxStringLength = 20;
  static const int dataLength = 66;

  char stringLength; // tbr
  char string[maxStringLength + 1];
  unsigned char data[dataLength];

  int getValue(int n, int processBits, int bitShift) {
    int byteNum = (n * 22 + bitShift) / 8; // our first byte
    int bitNum = (n * 22 + bitShift) % 8; // our first bit

    int res = data[byteNum]; // here we collect our value
    if (processBits + bitNum > 8) { // 2-byte value
      res += ((unsigned int)data[byteNum + 1] << 8);
    }

    // align our info with bitNum, then collect only necessary bits 
    return (res >> bitNum) & (0xFFFF >> (16 - processBits));
  }

  float getTemp(int n) {
    int t = getValue(n, 7, 15); // get bin data 
    return float(t) / 2. - 22.;   
  }
};
