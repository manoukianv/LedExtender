/*
  Copyright (c) 2024 Vincent Manoukian. All rights reserved.
  This file is part of the Acapeo SimRacing product for firmware developpement.

  This library is free software; you can redistribute it and/or
  modify it under the terms of mentions in Licences.md files. This
  libreary can't be exluded from the licences.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the licences by following links,
  and contacts shared in Licences.md
*/

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include "SimhubParser.h"

#define PIN_STRIP   29

//#define BOARD_VENDORID		0x1234
//#define BOARD_PRODUCTID		0x5678
//#define BOARD_NAME			"RaspberryPi Pico ACAPEO"

SimhubParser simhubParser;
Adafruit_NeoPixel pixels(STRIP1_RGBLEDCOUNT, PIN_STRIP, NEO_GRB + NEO_KHZ800);

void setup() {

  Serial.begin(115200);

  TinyUSBDevice.setManufacturerDescriptor("Acapeo (Vincent Manoukian)");
  TinyUSBDevice.setProductDescriptor("LedExtender");
  // TODO Temporary VID/PID, request pending
  TinyUSBDevice.setID(0x1209, 0x4E00);

  simhubParser.begin(&Serial);

  pinMode(PIN_STRIP, OUTPUT);
  pixels.begin();
  for (int i=0; i<STRIP1_RGBLEDCOUNT; i++) {
    pixels.setPixelColor(i,255,0,0);
  }
  pixels.show();
}

void loop() {
  switch (simhubParser.processCommands())
  {
  case SIMHUB_RESET_SERIAL:
    // TODO Store in flash the serial number
    break;
  
  case SIMHUB_SEND_LED_COLOR:
    // Update the strip led color with
    for (int i=0; i<STRIP1_RGBLEDCOUNT; i++) {
      PixelColor color = simhubParser.getLedColor(i);
      pixels.setPixelColor(i,color.red, color.green, color.blue);
    }
    pixels.show();
    break;
  
  default:
    break;
  }

}
