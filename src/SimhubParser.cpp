#include "SimhubParser.h"

SimhubParser::SimhubParser(){};

void SimhubParser::begin(Adafruit_USBD_CDC *inputserial) {
	serial = inputserial;
}


/// <summary>
/// Read rgb strip data
/// </summary>
void SimhubParser::readStrip() {
	uint8_t buffer[STRIP1_RGBLEDCOUNT*3];

	// wait the data_flow is incoming
	while (serial->available() < (int)sizeof(buffer)) {
		yield();
	}

	serial->readBytes(buffer, sizeof(buffer));
	for (int i = 0; i < STRIP1_RGBLEDCOUNT; i++) {
		uint8_t led_index = !STRIP1_RIGHTTOLEFT ? i : STRIP1_RGBLEDCOUNT - 1 - i;
		stripColor[led_index] = {
				buffer[(i*3)], 
				buffer[(i*3) + 1], 
				buffer[(i*3) + 2]
		};
	}

}

PixelColor SimhubParser::getLedColor(uint8_t ledNumber) {
	return stripColor[ledNumber];
}


/// <summary>
/// Sends leds count to the serial port.
/// </summary>
void SimhubParser::getLedsCount() {
	serial->println(STRIP1_RGBLEDCOUNT);
}

/// <summary>
/// Sends the protocol version.
/// </summary>
void SimhubParser::getProtocolVersion() {
	serial->println(PROTOCOLVERSION);
}
/// <summary>
/// Sends the protocol version.
/// </summary>
String SimhubParser::getSerialNumber(bool autoGenEmpty) {
	if (serialNumber == "" && autoGenEmpty) {
		resetSerialNumber();
		}
	
	return serialNumber;
}

/// <summary>
/// Sends the protocol version.
/// </summary>
String SimhubParser::resetSerialNumber() {
	char data[uniqueid_length + 1];

	randomSeed(analogRead(A1));

	for (int i=0; i<uniqueid_length; i++) {
		data[i] = (char)random('A', 'Z');
	}
	data[uniqueid_length] = '\0';


	serialNumber = String(data);
	return serialNumber;
}

/// <summary>
/// Read leds data from serial port.
/// </summary>
bool SimhubParser::readLeds() {

	// Parse the incoming Flow
	readStrip();

	// 3 bytes end of message, it must be (0xFF)(0xFE)(0xFD) to be taken into account, it's a super simple way to be sure we reached the correct end.
	uint8_t buffer[3];
	
	while (serial->available()<3) {
		yield();
	}
	serial->readBytes(buffer, 3);
	const bool valid = memcmp(buffer, end_check, 3) == 0;

	return valid;
}

/// <summary>
/// Callback when custom serial data is incoming
/// </summary>
/// <param name="messageLength">The length of the incoming message</param>
/// read it using usual Serial.read() etc,
/// make sure to read exactly and completely your whole message
/// (messageLength is provided so you know exactly how much bytes must be read.)
void SimhubParser::onCustomSerialData(int messageLength) {
	// Not implemented, but clean the incoming buffer
	int nb_left_to_clean = messageLength;
	while ( (nb_left_to_clean > 0) && serial->available()) {
		// check how many read are available
		const int max_char_to_read = min(nb_left_to_clean, serial->available());
		// Clear the buffer
		for (uint8_t i=0; i<max_char_to_read; i++) 
			serial->read();
		// Count how many left
		nb_left_to_clean -= max_char_to_read;
	}
}


/// <summary>
/// Read commands from serial port.
/// </summary>
SimHubAction SimhubParser::processCommands() {
	SimHubAction result = SIMHUB_NO_ACTION;

	// If there is serial data, get the size of the buffer, 
	//to process only message when received is complete for an action
	if (int size = serial->available()) {
		//DEBUGV("Incoming serial header/command/size : %d, %d, %d", is_waiting_header, is_waiting_command, size);

		uint8_t buffer[6];

		// If we are waiting an header check it
		if (is_waiting_header && size>=6) {
			//ESP_LOGD(TAG_LOG, "check header, size %d", size);
			uint8_t buffer[6];
			int number_read = serial->readBytes(buffer, 6);			// read the first 6 bytes
			size -= number_read;								// remove the first 6 byte to continue with the command if there is one after
			//ESP_LOG_BUFFER_HEXDUMP(TAG_LOG, buffer, 6, ESP_LOG_VERBOSE);

			const bool is_header_detected = memcmp(buffer, stop_header, 6) == 0 ;	// check if header is OK
			is_waiting_command = is_header_detected; 
			is_waiting_header = !is_header_detected;

			// if we don't received a welformated command, continue to parce the message, byte by byte until a command arrive			
			if (is_waiting_header) {
				int cpt = 0;
				while (serial->available()) {
					int c = serial->read();
					//ESP_LOGV(TAG_LOG, "header error, clean bytes %X / %d", c, messageend);

						if (c==0xFF) {
							messageend++;
							if (messageend >= 6) {
								is_waiting_header = false;
								is_waiting_command = true;
								return result;
							}
						}
						else
							messageend=0;
			
					cpt++;
				}
				//ESP_LOGW(TAG_LOG, "header error, clean %d bytes", cpt);		
			}
		}

		if (is_waiting_command && size>=5) {
			//ESP_LOGD(TAG_LOG, "check command, size %d", size);
			serial->readBytes(buffer, 5);
			command = String(buffer, 5);

			//ESP_LOG_BUFFER_HEXDUMP(TAG_LOG, buffer, 6, ESP_LOG_VERBOSE);
			//ESP_LOGI(TAG_LOG, "Incoming command : %s", buffer);

			// *** DEVICE GENERAL INFORMATIONS  ***


			// Get protocol version
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)proto
			//	FFFFFFFFFFFF70726F746F
			if (command == F("proto")) {
				getProtocolVersion();
			}

			// Get device name
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)dname
			else if (command == F("dname")) {
				serial->println(DEVICE_NAME);
			}

			// Get brand name
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)dname
			else if (command == F("bname")) {
				serial->println(DEVICE_BRAND);
			}

			// Get device picture
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)dname
			else if (command == F("dpict")) {
				serial->println(DEVICE_PICTURE_URL);
			}

			// Get device auto detect state
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)dname
			else if (command == F("ddete")) {
				serial->println((int)DEVICE_AUTODETECT_ALLOWED);
			}

			// Get device type
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)dtype
			else if (command == F("dtype")) {
				serial->println((int)DEVICE_TYPE);
			}

			// Get upload protection informations
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)ulock
			else if (command == F("ulock")) {
				serial->print(ENABLE_UPLOAD_PROTECTION);
				serial->print(",");
				serial->println(UPLOAD_AVAILABLE_DELAY);
			}

			// *** SERIAL NUMBER  ***

			// Get serial number
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)snumb
			// FFFFFFFFFFFF736E756D62
			if (command == "snumb") {
				serial->println(getSerialNumber());
			}

			// Reset serial number
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)rnumb
			// FFFFFFFFFFFF726E756D62
			if (command == "rnumb") {
				serial->println(resetSerialNumber());
				result = SIMHUB_RESET_SERIAL;
			}

			// *** LEDS ***

			// Get leds count
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)ledsc
			// FFFFFFFFFFFF6C65647363
			if (command == "ledsc") {
				getLedsCount();
			}

			// Get leds Layout
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)ledsl
			else if (command == F("ledsl")) {
				serial->println(LEDS_LAYOUT);
			}

			// Get default buttons profile colors
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)butdc
			else if (command == F("butdc")) {
				serial->println(DEFAULT_BUTTONS_COLORS);
			}

			// Send leds data (in binary) terminated by (0xFF)(0xFE)(0xFD)
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)sleds(RL1)(GL1)(BL1)(RL2)(GL2)(BL2) .... (0xFF)(0xFE)(0xFD)
			// FFFFFFFFFFFF
			// sleds = 736C656473
			// 21 * RGB  = FF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FF
			// eof = FFFEFD
			//FFFFFFFFFFFF736C656473FF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFFFEFD
			//FFFFFFFFFFFF736C656473000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000FFFEFD
			else if (command == "sleds") {
				if (readLeds()) {
					result = SIMHUB_SEND_LED_COLOR;
				}
			}

			// *** MATRIX ***

			// Get 8x8 matrix count
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)matxc
			else if (command == F("matxc")) {
				serial->println(MATRIX_ENABLED > 0 ? 1 : 0);
			}

			// Set 8x8 matrix content
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)sleds(RL1)(GL1)(BL1)(RL2)(GL2)(BL2) .... (0xFF)(0xFE)(0xFD)
			else if (command == F("smatx")) {
				// TODO if matrix on wheel
				//readMatrix();
			}

			// *** FANS ***
			
			// Get fans count
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)fansc
			else if (command == F("fansc")) {
				// if fan on the wheel, update it
				serial->println(0);
			}

			// Set fans values (16bits)
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)sfans(FAN1 MSB)(FAN1 LSB)(FAN2 MSB)(FAN2 LSB)(FAN3 MSB)(FAN3 LSB) .... (0xFF)(0xFE)(0xFD)
			else if (command == F("sfans")) {
				// if fan on the wheel, update it
			}

			// Vendor message
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)vendo
			else if (command == F("vendo")) {
				while (serial->available()<2) {
					yield();
				}
				uint8_t buffer[2];
				serial->readBytes(buffer, 2);
				int messageLength = ((int) buffer[0] << 8) + (int)buffer[1];
				onCustomSerialData(messageLength);
			}

			// Unlock upload
			// (0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)unloc
			else if (command == "unloc") {
				// Lock-Unlock is not available on rp2040
			}

			messageend = 0;
			command = "";
			is_waiting_header = true;
			is_waiting_command = false;
		}
	}

	return result;
}