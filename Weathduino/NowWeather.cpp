struct NowWeather {
  static const int maxStringLength = 20;
  static const int dataLength = 6;

  char stringLength;
  char string[maxStringLength + 1];
  unsigned char data[dataLength];

  float getTemp() {
    // bits 38..47 of data
    int t = ((unsigned int)data[4] >> 6) + ((unsigned int)data[5] << 2) - 512;
    return float(t) / 10.;
  }
  
  // Write temperature to buff, from the right side, beggining with whitespaces
  // Buffer size should be at least 8 bytes.
  // Temperature is in the "+51.1°C", "-51.2°C", " +0.1°C" format.
  void writeTemp(char *buff, unsigned int buffSize) {
    if (buffSize < 8)
      return;
    int t = ((unsigned int)data[4] >> 6) + ((unsigned int)data[5] << 2) - 512;
    char sign = t < 0 ? '-' : '+';
    t = t > 0 ? t : -t;
    bool tens = (t / 100) > 0;

    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = 'C';
    buff[buffSize - 3] = 176; //'°'
    buff[buffSize - 4] = t % 10 + '0';
    buff[buffSize - 5] = '.';
    buff[buffSize - 6] = (t / 10) % 10 + '0';
    if (!tens) { // 1 digit before '.'
      buff[buffSize - 7] = sign;
      buff[buffSize - 8] = ' ';
    }
    else {
      buff[buffSize - 7] = (unsigned char)(t / 100) + '0';
      buff[buffSize - 8] = sign;
    }
    for (int i = buffSize - 9; i >= 0; i--) buff[i] = ' ';
  }

  float getTempFeel() {
    // bits 28..37 of data
    int t = ((unsigned int)data[3] >> 4) + ((unsigned int)(data[4] & 0x3F) << 4) - 512;
    return float(t) / 10.;
  }
  
  // Write feeling temperature to buff, from the right side, beggining with whitespaces
  // Buffer size should be at least 8 bytes.
  // Temperature is in the "+51.1°C", "-51.2°C", " +0.1°C" format.
  void writeTempFeel(char *buff, unsigned int buffSize) {
    if (buffSize < 8)
      return;
    int t = ((unsigned int)data[3] >> 4) + ((unsigned int)(data[4] & 0x3F) << 4) - 512;
    char sign = t < 0 ? '-' : '+';
    t = t > 0 ? t : -t;
    bool tens = (t / 100) > 0;

    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = 'C';
    buff[buffSize - 3] = 176; //'°'
    buff[buffSize - 4] = t % 10 + '0';
    buff[buffSize - 5] = '.';
    buff[buffSize - 6] = (t / 10) % 10 + '0';
    if (!tens) { // 1 digit before '.'
      buff[buffSize - 7] = sign;
      buff[buffSize - 8] = ' ';
    }
    else {
      buff[buffSize - 7] = (unsigned char)(t / 100) + '0';
      buff[buffSize - 8] = sign;
    }
    for (int i = buffSize - 9; i >= 0; i--) buff[i] = ' ';
  }

  int getHumid() {
    // bits 21..27 of data
    int t = ((unsigned int)data[2] >> 5) + ((unsigned int)(data[3] & 0x0F) << 3);
    return t;
  }

  void writeHumid(char *buff, unsigned int buffSize) {
    if (buffSize < 5)
      return;
    int t = ((unsigned int)data[2] >> 5) + ((unsigned int)(data[3] & 0x0F) << 3);
    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = '%';
    buff[buffSize - 3] = t % 10 + '0';
    buff[buffSize - 4] = t > 9 ? ((t / 10) % 10 + '0') : ' ';
    buff[buffSize - 5] = t > 99 ? '1' : ' ';
    for (int i = buffSize - 6; i >= 0; i--) buff[i] = ' ';
  }
  
  int getPrecip() {
    // bits 16..20 of data
    int t = (unsigned int)(data[2] & 0x1F);
    return t;
  }

  void writePrecip(char *buff, unsigned int buffSize) {
    if (buffSize < 6)
      return;
    int t = (unsigned int)(data[2] & 0x1F);
    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = 'm';
    buff[buffSize - 3] = 'm';
    buff[buffSize - 4] = t % 10 + '0';
    buff[buffSize - 5] = '.';
    buff[buffSize - 6] = (t / 10) + '0';
    for (int i = buffSize - 7; i >= 0; i--) buff[i] = ' ';
  }
  
  float getWind() {
    // bits 7..15 of data
    int t = ((unsigned int)data[0] >> 7) + ((unsigned int)data[1] << 1);
    return float(t) / 10.;
  }

  void writeWind(char *buff, unsigned int buffSize) {
    if (buffSize < 8)
      return;
    int t = ((unsigned int)data[0] >> 7) + ((unsigned int)data[1] << 1);
    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = 's';
    buff[buffSize - 3] = '/';
    buff[buffSize - 4] = 'M';
    buff[buffSize - 5] = t % 10  + '0';
    buff[buffSize - 6] = '.';
    buff[buffSize - 7] = (t / 10) % 10 + '0';
    buff[buffSize - 8] = t > 99 ? (t / 100 + '0') : ' ';
    for (int i = buffSize - 9; i >= 0; i--) buff[i] = ' ';    
  }
  
  int getPrecipProb() {
    int t = (unsigned int)(data[0] & 0x7F);
    return t / 11;
  }

  void writePrecipProb(char *buff, unsigned int buffSize) {
    if (buffSize < 7)
      return;
    int t = (unsigned int)(data[0] & 0x7F);
    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = ')';
    buff[buffSize - 3] = '%';
    buff[buffSize - 4] = '0';
    if (t == 0) {
      buff[buffSize - 5] = '(';
      buff[buffSize - 6] = ' ';
      buff[buffSize - 7] = ' ';
    }
    else if (t < 10) {
      buff[buffSize - 5] = t + '0';
      buff[buffSize - 6] = ')';
      buff[buffSize - 7] = ' ';      
    }
    else {
      buff[buffSize - 5] = t % 10 + '0';
      buff[buffSize - 6] = t / 10 + '0';
      buff[buffSize - 7] = ')';      
    }
    for (int i = buffSize - 8; i >= 0; i--) buff[i] = ' ';        
  }
  
  int getCloud() {
    int t = (unsigned int)(data[0] & 0x7F);
    return t % 11;
  }

  void writeCloud(char *buff, unsigned int buffSize) {
    if (buffSize < 5)
      return;
    int t = (unsigned int)(data[0] & 0x7F);
    buff[buffSize - 1] = '\0';
    buff[buffSize - 2] = '%';
    buff[buffSize - 3] = '0';
    if (t == 0) {
      buff[buffSize - 4] = ' ';
      buff[buffSize - 5] = ' ';
    }
    else if (t < 10) {
      buff[buffSize - 4] = t + '0';
      buff[buffSize - 5] = ' ';
    }
    else {
      buff[buffSize - 4] = t % 10 + '0';
      buff[buffSize - 5] = t / 10 + '0';
    }
    for (int i = buffSize - 6; i >= 0; i--) buff[i] = ' ';        
  }
};
