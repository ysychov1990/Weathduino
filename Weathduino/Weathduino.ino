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

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

struct NowWeather {
  static const int maxStringLength = 20;
  static const int dataLength = 6;

  char stringLength;
  char string[maxStringLength + 1];
  char data[dataLength];
};

struct Weather {
  static const int maxStringLength = 20;
  static const int dataLength = 66;

  char stringLength; // tbr
  char string[maxStringLength + 1];
  char data[dataLength];
};

enum State {
  WAIT_FOR_TYPE,
  WAIT_FOR_STRLEN,
  WAIT_FOR_STR,
  WAIT_FOR_DATA,
  DISPLAY_NOWWEATHER
};

void u8g2_prepare(void) {
  u8g2.setFont(unifont_polish2);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


State currentState = WAIT_FOR_TYPE;
byte packetType = 0;
int bytesReceived = 0;
int totalByteCount = 0;

void processPacket(byte type, byte* data, int len) {
  // Process your data here (e.g., extract bits, update display)

  // Send the specific acknowledgment your C++ code expects
  Serial.print("OK ");
  Serial.println((char)type);
}

NowWeather nowWeather {};
Weather weather[3] {};

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
        if (incomingByte > Weather::maxStringLength) {
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
        if (bytesReceived >= Weather::dataLength) {
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
  do {
    u8g2.drawFrame(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight() );
    u8g2.setCursor(0, 0);
    u8g2.drawStr(0, 0, nowWeather.string);
    u8g2.drawStr(0, 15, weather[0].string);
    u8g2.drawStr(0, 30, weather[1].string);
    u8g2.drawStr(0, 45, weather[2].string);
//    u8g2.println(currentState);
    // picture loop
  } while (u8g2.nextPage());
  // delay between each page
  //delay(150);
}
