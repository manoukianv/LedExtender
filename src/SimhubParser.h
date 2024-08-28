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

#pragma once

#include <Arduino.h>

#define PROTOCOLVERSION "SIMHUB_1.2"

#define ENABLE_UPLOAD_PROTECTION 0
#define UPLOAD_AVAILABLE_DELAY 15000

#define DEVICE_AUTODETECT_ALLOWED 1
#define DEVICE_TYPE 1
#define DEVICE_BRAND "Acapeo"
#define DEVICE_NAME "LedExtender"
#define DEVICE_PICTURE_URL "https://simhubdash.com/devices/nopicture.png"

#define STRIP1_RGBLEDCOUNT 18
#define STRIP1_RIGHTTOLEFT false

#define LEDS_LAYOUT "L0,L1,L2,L3,L4,L5,L6,L7,L8,L9,L10,L11,L12,L13,L14,L15,L16,L17"
#define DEFAULT_BUTTONS_COLORS ""
#define MATRIX_ENABLED 0
#define MATRIX_DATAPIN 6
#define MATRIX_LAYOUT MATRIX_LEFTTORIGHT
#define MATRIX_MAX_BRIGHTNESS 255

#define FANS_CONTROLLER 0
#define PWMFANS_COUNT 0

#define uniqueid_sig1  0x55
#define uniqueid_sig2  0x58
#define uniqueid_sigstart_length 2
#define uniqueid_length 20

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} PixelColor;

enum SimHubAction {
	SIMHUB_NO_ACTION,
	SIMHUB_RESET_SERIAL,
	SIMHUB_SEND_LED_COLOR
};


class SimhubParser {
	private :
		Adafruit_USBD_CDC *serial;		// serial port to listen
		String serialNumber = "";	// serialNumber registered
		PixelColor stripColor[STRIP1_RGBLEDCOUNT];

		int messageend = 0;			// internal parameters to parse flow
		String command;
		bool uploadUnlocked = 1;
		bool is_waiting_header = true;
		bool is_waiting_command = false;

		const uint8_t stop_header[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		const uint8_t end_check[3] = {0xFF, 0xFE, 0xFD};

		void readStrip();
		void getLedsCount();
		void getProtocolVersion();
		String resetSerialNumber();
		bool readLeds();
        void onCustomSerialData(int nbMessage);

	public:
		SimhubParser();
		
		void begin(Adafruit_USBD_CDC *inputserial);
        SimHubAction processCommands();

        String getSerialNumber(bool autoGenEmpty = false);

		PixelColor getLedColor(uint8_t ledNumber);
};