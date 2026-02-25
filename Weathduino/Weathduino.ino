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

static const int weatherStrLength = 20;
static const int weatherDataLength = 66;

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

void u8g2_box_title() {
  u8g2.drawStr( 10, 5, "U8g2");
  u8g2.drawStr( 10, 20, "GraphicsTest");
  u8g2.drawFrame(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight() );
}

void draw(void) {
  u8g2_prepare();
  u8g2_box_title();
}

State currentState = WAIT_FOR_TYPE;
byte packetType = 0;
byte payloadBuffer[256]; // Ensure this is large enough for your longest packet
int bytesReceived = 0;
int expectedPayloadSize = 0;

void processPacket(byte type, byte* data, int len) {
  // Process your data here (e.g., extract bits, update display)

  // Send the specific acknowledgment your C++ code expects
  Serial.print("OK ");
  Serial.println((char)type);
}

NowWeather nowWeather {};
Weather weather[3]{};

void setup(void) {
  u8g2.begin();
  Serial.begin(9600);
  u8g2_prepare();
}

void loop() {
  u8g2.firstPage();
  do {
    // Non-blocking serial read
    while (Serial.available() > 0) {
      byte incomingByte = Serial.read();
      //Serial.write(incomingByte);

      switch (currentState) {
        case WAIT_FOR_TYPE:
          //u8g2.drawStr(0, 15, "Waiting for data...");
          packetType = incomingByte;
          // packet types are '0', '1', '2', '3'
          if (packetType >= '0' && packetType <= '3') {
            currentState = WAIT_FOR_STRLEN;
            packetType -= '0';
          }
          else
            currentState = WAIT_FOR_TYPE; // unknown packet type
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
            weather[packetType-1].stringLength = incomingByte;
            currentState = WAIT_FOR_STR;
            bytesReceived = 0;
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
            weather[packetType-1].string[bytesReceived++] = incomingByte;
            if (bytesReceived >= weather[packetType-1].stringLength) {
              currentState = WAIT_FOR_DATA;
              weather[packetType-1].string[bytesReceived] = 0;
              bytesReceived = 0;
            }
          }
          break;

        case WAIT_FOR_DATA:
          //Serial.write('#');
          if (packetType == 0) {
            nowWeather.data[bytesReceived++] = incomingByte;
            if (bytesReceived >= NowWeather::dataLength) {
              Serial.print("OK 0\n");
              //u8g2.drawStr(0, 15, nowWeather.string);
              currentState = WAIT_FOR_TYPE;
              bytesReceived = 0;
            }
          }
          else {
            weather[packetType-1].data[bytesReceived++] = incomingByte;
            if (bytesReceived >= Weather::dataLength) {
              Serial.print("OK "); Serial.write(packetType + '0');
              currentState = WAIT_FOR_TYPE;
              bytesReceived = 0;
            }
          }
          break;
      }
    }

    //u8g2.nextPage();
    // You can do other things here without being blocked by Serial
    u8g2.drawFrame(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight() );
    u8g2.setCursor(1, 1);
    u8g2.println("Current state:");
    u8g2.println(currentState);

    // picture loop
  } while (u8g2.nextPage());
  // delay between each page
  //delay(150);
}
