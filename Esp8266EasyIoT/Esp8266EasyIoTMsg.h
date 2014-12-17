 /*
 V1.0 - first version
 
 Created by Igor Jarc <igor.jarc1@gmail.com>
 See http://iot-playground.com for details
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#ifndef EasyIoTEspMessage_h
#define EasyIoTEspMessage_h

#ifdef __cplusplus
#include <Arduino.h>
#endif


#define LIBRARY_VERSION "0.1"
#define PROTOCOL_VERSION 1
#define MAX_MESSAGE_LENGTH 127
#define HEADER_SIZE 9
#define MAX_PAYLOAD (MAX_MESSAGE_LENGTH - HEADER_SIZE)

#define START_MSG 0x68
#define NODE_SENSOR_ID 0xFF // Node child id 


#define BIT(n)                  ( 1<<(n) )
// Create a bitmask of length len.
#define BIT_MASK(len)           ( BIT(len)-1 )
// Create a bitfield mask of length starting at bit 'start'.
#define BF_MASK(start, len)     ( BIT_MASK(len)<<(start) )

// Prepare a bitmask for insertion or combining.
#define BF_PREP(x, start, len)  ( ((x)&BIT_MASK(len)) << (start) )
// Extract a bitfield of length len starting at bit 'start' from y.
#define BF_GET(y, start, len)   ( ((y)>>(start)) & BIT_MASK(len) )
// Insert a new bitfield value x into y.
#define BF_SET(y, x, start, len)    ( y= ((y) &~ BF_MASK(start, len)) | BF_PREP(x, start, len) )


// Getters/setters for special bit fields in header
#define mSetSender(_msg,_sender) _msg.senderL = _sender & 0xFF; _msg.senderH = ((_sender >> 8) & 0xFF);
#define mGetSender(_msg) (uint16_t)((_msg.senderH << 8) + _msg.senderL)

#define mSetCommand(_msg,_command) BF_SET(_msg.command_ack_payloadype, _command, 0, 3)
#define mGetCommand(_msg) BF_GET(_msg.command_ack_payloadype, 0, 3)

#define mSetRequestAck(_msg,_rack) BF_SET(_msg.command_ack_payloadype, _rack, 3, 1)
#define mGetRequestAck(_msg) BF_GET(_msg.command_ack_payloadype, 3, 1)

#define mSetAck(_msg,_ackMsg) BF_SET(_msg.command_ack_payloadype, _ackMsg, 4, 1)
#define mGetAck(_msg) BF_GET(_msg.command_ack_payloadype, 4, 1)

#define mSetDataType(_msg, _pt) BF_SET(_msg.command_ack_payloadype, _pt, 5, 3)
#define mGetDataType(_msg) BF_GET(_msg.command_ack_payloadype, 5, 3)

#define miSetPayloadType(_pt) BF_SET(command_ack_payloadype, _pt, 5, 3)
#define miGetPayloadType() BF_GET(command_ack_payloadype, 5, 3)



// Message types
typedef enum {
	C_PRESENTATION = 0,
	C_SET = 1,
	C_REQ = 2,
	C_INTERNAL = 3,
} e_command_type;

typedef enum {
	  I_ID_REQUEST = 0,		// sprejem
      I_ID_RESPONSE = 1,	// oddaja
      I_PING = 2,			// sprejem/oddaja
      I_PING_RESPONSE = 3	// oddaja/oddaja
} e_internal_type;


typedef enum {
      S_GENERIC         = 0,  // AI, DI
      S_DIGITAL_INPUT   = 1,  // DI
      S_DIGITAL_OUTPUT  = 2,  // DO
      S_DOOR            = 3,  // DI
      S_ANALOG_INPUT    = 4,  // AI
      S_TEMP            = 5,  // AI
      S_HUM             = 6,  // AI
      S_LEAK            = 7   // DI
} e_sensor_type;


typedef enum {
	P_STRING , P_BYTE, P_INT16, P_UINT16, P_LONG32, P_ULONG32, P_CUSTOM, P_FLOAT32
} e_data_type;


    // Type of sensor data (for set/req/ack messages)
typedef enum {
	  V_UNDEFINED = 0,
      V_DIGITAL_INPUT = 1,
      V_DIGITAL_OUTPUT = 2,
      V_DOOR = 3,
      V_TEMP = 4, 
      V_HUM = 5, 
} ;


#ifdef __cplusplus
class Esp8266EasyIoTMsg
{
public:
	// Constructors
	Esp8266EasyIoTMsg();
	Esp8266EasyIoTMsg(uint8_t sensor, uint8_t type);
	void crc8();
	uint8_t calculateCrc8();

	Esp8266EasyIoTMsg& set(const char* value);
	Esp8266EasyIoTMsg& set(float value, uint8_t decimals);

	int getInt() const;
	uint16_t getUInt() const;
	bool getBool() const;

#else

typedef union {
struct
{

#endif
	uint8_t start;					// start 
    uint8_t version;				// protocol version
    uint8_t senderH;				// sedner address H
    uint8_t senderL;				// sedner address L
    uint8_t command_ack_payloadype;	// 3 bit - Command type
									// 1 bit - Request an ack - Indicator that receiver should send an ack back.
									// 1 bit - Is ack messsage - Indicator that this is the actual ack message.
									// 3 bit - Data type

    uint8_t type;            	    // 8 bit - Type varies depending on command      
    uint8_t sensor;          	    // 8 bit - Id of sensor that this message concerns.
    uint8_t length;					// message length
    uint8_t crc;					// crc8


	union {
		uint8_t bValue;
		unsigned long ulValue;
		long lValue;
		uint16_t uiValue;
		int iValue;
		struct { // Float messages
			float fValue;
			uint8_t fPrecision;   // Number of decimals when serializing
		};
		char data[MAX_PAYLOAD + 1];
	} __attribute__((packed));
#ifdef __cplusplus
} __attribute__((packed));
#else
};
uint8_t array[HEADER_SIZE + MAX_PAYLOAD + 1];	
} __attribute__((packed)) EasyIoTEspMessage;
#endif

#endif