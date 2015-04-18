 /*
 V1.4 - upgraded to ESP8266 0.952 firmware version
 V1.3 - additional data types
 V1.1 - additional data types
 V1.0 - first version

 
 Created by Igor Jarc
 See http://iot-playground.com for details
 Please use community fourum on website do not contact author directly

 Code based on MySensors datatypes and API. http://www.mysensors.org
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include "Esp8266EasyIoTMsg.h"

Esp8266EasyIoTMsg::Esp8266EasyIoTMsg() {
	start = START_MSG;
	version = PROTOCOL_VERSION;
}


Esp8266EasyIoTMsg::Esp8266EasyIoTMsg(uint8_t _sensor, uint8_t _type) {
	start = START_MSG;
	version = PROTOCOL_VERSION;

	sensor = _sensor;
	type = _type;
}

Esp8266EasyIoTMsg& Esp8266EasyIoTMsg::set(const char* value) {
	uint8_t len = strlen(value);
	length  = len;
	miSetPayloadType(P_STRING);
	strncpy(data, value, min(length, MAX_PAYLOAD));
	return *this;
}

Esp8266EasyIoTMsg& Esp8266EasyIoTMsg::set(uint8_t value) {
	length = 1;
	miSetPayloadType(P_BYTE);
	data[0] = value;
	return *this;
}

Esp8266EasyIoTMsg& Esp8266EasyIoTMsg::set(float value, uint8_t decimals) {
	length =  5; // 32 bit float + persi
	miSetPayloadType(P_FLOAT32);
	fValue=value;
	fPrecision = decimals;
	return *this;
}


uint16_t Esp8266EasyIoTMsg::getUInt() const {
	if (miGetPayloadType() == P_UINT16) { 
		return uiValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}

int Esp8266EasyIoTMsg::getInt() const {
	if (miGetPayloadType() == P_INT16) { 
		return iValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}

bool Esp8266EasyIoTMsg::getBool() const {
	return getInt();
}

float Esp8266EasyIoTMsg::getFloat() const {
	if (miGetPayloadType() == P_FLOAT32) {
		return fValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atof(data);
	} else {
		return 0;
	}
}

unsigned long Esp8266EasyIoTMsg::getULong() const {
	if (miGetPayloadType() == P_ULONG32) {
		return ulValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atol(data);
	} else {
		return 0;
	}
}


/*
 * calculate CRC8 
 */
void Esp8266EasyIoTMsg::crc8() {
	crc = calculateCrc8();
}


uint8_t Esp8266EasyIoTMsg::calculateCrc8() {
	uint8_t loop_count;
	uint8_t bit_counter;
	uint8_t dt;
	uint8_t feedback_bit;
	uint8_t tmpcrc;

	uint8_t number_of_bytes_to_read = HEADER_SIZE + length;

	// Must set crc to a constant value.
	tmpcrc = crc;
	crc = 0;
	uint8_t  extcrc = 0;

	uint8_t* buff = reinterpret_cast<uint8_t*>(&start);

	for (loop_count = 0; loop_count != number_of_bytes_to_read; loop_count++)
	{
		dt = (uint8_t)buff[loop_count];

		bit_counter = 8;
		do {
		  feedback_bit = (extcrc ^ dt) & 0x01;

		  if ( feedback_bit == 0x01 ) {
			extcrc = extcrc ^ 0x18;              //0X18 = X^8+X^5+X^4+X^0
		  }
		  extcrc = (extcrc >> 1) & 0x7F;
		  if ( feedback_bit == 0x01 ) {
			extcrc = extcrc | 0x80;
		  }

		  dt = dt >> 1;
		  bit_counter--;

		} while (bit_counter > 0);
	}

	crc = tmpcrc;
	return extcrc;
}