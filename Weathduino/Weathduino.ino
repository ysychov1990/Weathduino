/*

  GraphicsTest.ino

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include "unifont_polish2.h"
#include "NowWeather.cpp"
#include "DayWeather.cpp"

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);



enum SerialState {
  WAIT_FOR_TYPE,
  WAIT_FOR_STRLEN,
  WAIT_FOR_STR,
  WAIT_FOR_DATA
};

enum DisplayState {
  NOW_WEATHER_1,
  NOW_WEATHER_2,
  DAY1_WEATHER,
  DAY2_WEATHER,
  DAY3_WEATHER
};

void u8g2_prepare(void) {
  u8g2.setFont(unifont_polish2);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

SerialState currentState = WAIT_FOR_TYPE;
DisplayState displayState = DAY1_WEATHER;//NOW_WEATHER_2;
byte packetType = 0;
int bytesReceived = 0;
int totalByteCount = 0;

NowWeather nowWeather {};
DayWeather weather[3] {};

void setup(void) {
  u8g2.begin();
  Serial.begin(9600);
  u8g2_prepare();
}

void processByte(byte incomingByte) {
  totalByteCount++;
  switch (currentState) {
    case WAIT_FOR_TYPE:
      // packet types are '0', '1', '2', '3'
      if (incomingByte == '0') {
        // beginning of a transaction
        currentState = WAIT_FOR_STRLEN;
        packetType = incomingByte - '0';
        totalByteCount = 1; // reset everything recieved before that
      }
      if (incomingByte >= '1' && incomingByte <= '3') {
        currentState = WAIT_FOR_STRLEN;
        packetType = incomingByte - '0';
      }
      break;

    case WAIT_FOR_STRLEN:
      if (packetType == 0) {
        if (incomingByte > NowWeather::maxStringLength) {
          currentState = WAIT_FOR_TYPE;
        }
        else {
          nowWeather.stringLength = incomingByte;
          currentState = WAIT_FOR_STR;
          bytesReceived = 0;
        }
      }
      else {
        if (incomingByte > DayWeather::maxStringLength) {
          currentState = WAIT_FOR_TYPE;
        }
        else {
          weather[packetType - 1].stringLength = incomingByte;
          currentState = WAIT_FOR_STR;
          bytesReceived = 0;
        }
      }
      break;

    case WAIT_FOR_STR:
      if (packetType == 0) {
        nowWeather.string[bytesReceived++] = incomingByte;
        if (bytesReceived >= nowWeather.stringLength) {
          currentState = WAIT_FOR_DATA;
          nowWeather.string[bytesReceived] = 0;
          bytesReceived = 0;
        }
      }
      else {
        weather[packetType - 1].string[bytesReceived++] = incomingByte;
        if (bytesReceived >= weather[packetType - 1].stringLength) {
          currentState = WAIT_FOR_DATA;
          weather[packetType - 1].string[bytesReceived] = 0;
          bytesReceived = 0;
        }
      }
      break;

    case WAIT_FOR_DATA:
      if (packetType == 0) {
        nowWeather.data[bytesReceived++] = incomingByte;
        if (bytesReceived >= NowWeather::dataLength) {
          currentState = WAIT_FOR_TYPE;
          bytesReceived = 0;
        }
      }
      else {
        weather[packetType - 1].data[bytesReceived++] = incomingByte;
        if (bytesReceived >= DayWeather::dataLength) {
          currentState = WAIT_FOR_TYPE;
          bytesReceived = 0;
          if (packetType == 3) {
            // end of transmission
            Serial.print("OK ");
            Serial.write(((totalByteCount + 62) / 63) + '0');
            Serial.write('\n');
            totalByteCount = 1; // reset
          }
        }
      }
      break;
  }
  if (totalByteCount % 63 == 0) { // totalByteCount % 63 == 0
    Serial.print("OK ");
    Serial.write((totalByteCount / 63) + '0');
    Serial.write('\n');
  }
}

void loop() {
  while (Serial.available() > 0) {
    processByte(Serial.read());
  }

  u8g2.firstPage();
  char buff[16];
  do {
    switch (displayState) {
      case NOW_WEATHER_1:
        u8g2.drawFrame(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight() );

        u8g2.drawStr(12, 0, "Teraz:");
        nowWeather.writeTemp(buff, 8);
        u8g2.drawStr(60, 0, buff);

        u8g2.drawStr(12, 15, "Odcz.:");
        nowWeather.writeTempFeel(buff, 8);
        u8g2.drawStr(60, 15, buff);

        u8g2.drawStr(12, 30, "Wilg.:");
        nowWeather.writeHumid(buff, 5);
        u8g2.drawStr(60, 30, buff);

        u8g2.drawStr(12, 45, "Wiatr:");
        nowWeather.writeWind(buff, 8);
        u8g2.drawStr(60, 45, buff);
        break;

      case NOW_WEATHER_2:
        u8g2.drawFrame(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight() );

        u8g2.drawStr(12, 15, "Opady:");
        nowWeather.writePrecip(buff, 7);
        u8g2.drawStr(60, 8, buff);

        //u8g2.drawStr(12, 23, "Opady:");
        nowWeather.writePrecipProb(buff, 7);
        u8g2.drawStr(60, 23, buff);

        u8g2.drawStr(12, 38, "Chmury:");
        nowWeather.writeCloud(buff, 5);
        u8g2.drawStr(68, 38, buff);
        break;

      case DAY1_WEATHER:
        u8g2.drawStr(0, 0, "Temp. dzisiaj:");

        float arr[24], minv = 100., maxv = -100., difv;
        for (int i = 0; i < 24; i++) {
          arr[i] = weather[0].getTemp(i);
          if (arr[i] < minv) minv = arr[i];
          if (arr[i] > maxv) maxv = arr[i];
        }
        float lastPos = arr[0];
        difv = maxv - minv;
        if (difv == 0) difv = 0.01;
        for (int i = 0; i < 24; i++) {
          u8g2.drawLine(i * 5, 48 - 32 * (lastPos - minv) / difv , (i+1) * 5, 48 - 32 * (arr[i] - minv) / difv);
          lastPos = arr[i];
        }
        u8g2.drawStr(0, 50, "Min");
        u8g2.setCursor(28, 50);
        u8g2.print(int(minv));
        u8g2.drawStr(64, 50, "Max");
        u8g2.setCursor(92, 50);
        u8g2.print(int(maxv));
        break;
    }
    //    u8g2.println(currentState);
    // picture loop
  } while (u8g2.nextPage());
  // delay between each page
  //delay(150);
}
