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

#include "Esp8266EasyIoT.h"


// Inline function and macros
inline Esp8266EasyIoTMsg& build (Esp8266EasyIoTMsg &msg, uint16_t sender, uint8_t command, uint8_t type, uint8_t sensor, bool enableAck) {	
	mSetSender(msg,sender);
	mSetCommand(msg, command);
	msg.type = type;
	msg.sensor = sensor;
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

Esp8266EasyIoT::Esp8266EasyIoT(){
	_state = E_START;
	rxFlush();
}

void Esp8266EasyIoT::begin(void (*_msgCallback)(const Esp8266EasyIoTMsg &), int resetPin, Stream *serial)
{
	begin(_msgCallback, resetPin, serial, NULL);
}

void Esp8266EasyIoT::begin(void (*_msgCallback)(const Esp8266EasyIoTMsg &), int resetPin, Stream *serial, Stream *serialDebug = NULL)
{
	debug(PSTR("begin\n"));
	_resetPin = resetPin;
#ifdef DEBUG
	this->_serialDebug = serialDebug;

	if (serialDebug != NULL)
		this->isDebug = true;	
	else
		this->isDebug = false;		
#endif
	this->_serial = serial;	
	this->msgCallback = _msgCallback;

	// Read settings from EEPROM
	eeprom_read_block((void*)&_nodeId, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(uint16_t));

	debug(PSTR("nodeid:%d\n"), _nodeId);
	pinMode(_resetPin, OUTPUT);

	hwReset();

	delay(2000);

	//while(processesp() != E_IDLE) delay(1);

	// wait some time
	for(int i=0;i<600;i++)
	{
		if (processesp() != E_IDLE)
			delay(10);
		else
			break;
	}
	
	// Try to fetch node-id from gateway
	if (_nodeId == AUTO) {
		requestNodeId();
	}
}

void Esp8266EasyIoT::hwReset()
{
	debug(PSTR("hwReset\n"));
	digitalWrite(_resetPin, LOW); 
	delay(500);              // wait 
	digitalWrite(_resetPin, HIGH);   
}

void Esp8266EasyIoT::requestNodeId()
{
	debug(PSTR("Request Node id\n"));

	if (waitIdle())
	{
		writeesp(build(msg, _nodeId, C_INTERNAL, I_ID_REQUEST, NODE_SENSOR_ID, false).set(""));

		while(true)
		{
			if ((processesp() == E_IDLE) && (_nodeId != AUTO))
				break;
		}
	}
	else
	{
		debug(PSTR("Request Node id bussy...\n"));
	}
}

void Esp8266EasyIoT::requestTime(void (* _timeCallback)(unsigned long)) {
	timeCallback = _timeCallback;
	
	_waitingCommandResponse = true;
	_commandRespondTimer = millis();

	sendinternal(build(msg, _nodeId, C_INTERNAL, I_TIME, NODE_SENSOR_ID, false).set(""));
	_waitingCommandResponse = false;
}


void Esp8266EasyIoT::present(uint8_t childSensorId, uint8_t sensorType, bool enableAck) {
	sendinternal(build(msg, _nodeId, C_PRESENTATION, sensorType, childSensorId, enableAck).set(LIBRARY_VERSION));
}


void Esp8266EasyIoT::request(uint8_t childSensorId, uint8_t variableType)
{
	sendinternal(build(msg, _nodeId, C_REQ, variableType, childSensorId, false).set(""));
}


void Esp8266EasyIoT::sendBatteryLevel(uint8_t value, bool enableAck) {
	sendinternal(build(msg, _nodeId, C_INTERNAL, I_BATTERY_LEVEL, NODE_SENSOR_ID, enableAck).set(value));
}


// external send
void Esp8266EasyIoT::send(Esp8266EasyIoTMsg &message)
{	
	mSetCommand(message,C_SET);
	mSetSender(message,_nodeId);
	
	sendinternal(message);
}

// internal send
void Esp8266EasyIoT::sendinternal(Esp8266EasyIoTMsg &message)
{	
	resetPingTimmer();
	
	if (processesp() != E_IDLE)
	{
		for(int i=0;i<100;i++)
		{
			if (processesp() == E_IDLE)
				break;
			delay(10);
		}
	}

	writeesp(message);

	if (processesp() != E_IDLE)
	{
		for(int i=0;i<100;i++)
		{
			if (processesp() == E_IDLE)
				break;
			delay(10);
		}
	}
}

bool Esp8266EasyIoT::waitIdle() {
	unsigned long enter = millis();

	// Wait a couple of seconds for idle
	while (!isTimeout(enter, 17000)) {
		if (processesp() == E_IDLE)
		{
			return true;
		}
	}

	return false;
}


bool Esp8266EasyIoT::process()
{	
	bool ret = false;

	if (_newMessage)
	{
		if(!(msg.version == PROTOCOL_VERSION)) {
			debug(PSTR("Version mismatch\n"));
		}
		else
		{
			uint8_t command = mGetCommand(msg);
			uint8_t len = msg.length;
			uint8_t type = msg.type;
			uint16_t sender = mGetSender(msg);

			uint8_t calcCrc = msg.calculateCrc8();

			debug(PSTR("Receive: command: %d, type: %d, len: %d, sender: %d, crc: %d(%d)\n"), mGetCommand(msg), msg.type, msg.length, mGetSender(msg), msg.crc, (calcCrc == msg.crc)?"OK":"Error");

			if (calcCrc == msg.crc)
			{
				msg.data[msg.length] = '\0';

				if (command == C_INTERNAL)
				{
					debug(PSTR("Command\n"));
				
					if (type == I_ID_RESPONSE)
					{
						debug(PSTR("Id response\n"));

						if (_nodeId == AUTO) {
							_nodeId = msg.getUInt();
							// Write id to EEPROM
							if (_nodeId == AUTO) {
								debug(PSTR("Sensor network full\n"));

								while (1);
							} else {
								eeprom_write_block((void*)&_nodeId, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(uint16_t));							
							}

							debug(PSTR("New nodeId=%d\n"), _nodeId);	
						}
						resetPingTimmer();
					}
					else if (type == I_PING_RESPONSE)
					{
						debug(PSTR("PING response\n"));					
						_waitingCommandResponse = false;
						resetPingTimmer();			
					}
					else if (type == I_TIME)
					{
						debug(PSTR("TIME received\n"));		
						if (timeCallback != NULL) {
							timeCallback(msg.getULong());
						}
						_waitingCommandResponse = false;
						resetPingTimmer();			
					}

					else
					{
						debug(PSTR("Unkonwn command\n"));	
					}

				}
				else if (command == C_SET) 
				{
					debug(PSTR("Receive C_SET\n"));					
					resetPingTimmer();	

					_newMessage = false;
					// Call incoming message callback if available
					if (msgCallback != NULL) {
						msgCallback(msg);
					}		

					// check if we need to send ACK
					if (mGetRequestAck(msg)) {
						// answer with message copy
						ack = msg;
						mSetRequestAck(ack,false); // Reply without ack flag
						mSetAck(ack,true);
						mSetSender(ack, _nodeId);
						// calclulate new CRC
						ack.crc8();

						debug(PSTR("ACK message\n"));
						//sendinternal(ack);
						if (writeesp(ack))
						{
							debug(PSTR("ACK message\n"));		
							waitIdle();				
						}
					}
				}
			}
			else
			{
				debug(PSTR("Invalid CRC\n"));	
			}

		}
		_newMessage = false;
	}

	if (!_waitingCommandResponse && (_state == E_IDLE) && (isTimeout(_pingTimmer, PING_TIME) && (_nodeId != AUTO)))
	{		
		//if (_nodeId != AUTO) 
		//{
			if (writeesp(build(msg, _nodeId, C_INTERNAL, I_PING, NODE_SENSOR_ID, false).set("")))
			{
				debug(PSTR("PING\n"));		
				_waitingCommandResponse = true;
				_commandRespondTimer = millis();

				waitIdle();				
			}
		//}
	}

	if (_waitingCommandResponse)
	{
		if (isTimeout(_commandRespondTimer, COMMAND_RESPONSE_TIME))
		{
			debug(PSTR("Command response timeout\n"));	
			_waitingCommandResponse = false;
			_state = E_CIPCLOSE;
		}
	}


	processesp();
	

	if ((_state == E_IDLE) && (!_waitingCommandResponse))
		ret = true;

	return ret;
}


void Esp8266EasyIoT::setNewMsg(Esp8266EasyIoTMsg &msgnew)
{
	msg = msgnew;
}


void Esp8266EasyIoT::setPingTimmer()
{
	_pingTimmer = millis() - PING_TIME - 1;
}

void Esp8266EasyIoT::resetPingTimmer()
{
	_pingTimmer = millis();
}


// return false when idle
e_internal_state Esp8266EasyIoT::processesp()
{
	byte from1, from2;
	
	switch(_state)
	{
	case E_WAIT_OK:		
		if (isOk(_rxFlushProcessed))
		{
			debug(PSTR("\nResponse->OK\n"));	
			_state = _okState;
			processesp();
		}
		else if (isError(_rxFlushProcessed))
		{
			debug(PSTR("\nResponse->error\n"));	
			_state = _errorState;			
		}		
		else if (isTimeout(_startTime, _respondTimeout))
		{
			debug(PSTR("\nResponse->timeout\n"));	
			_state = _errorState;
		}
		break;
	case E_START:
		debug(PSTR("E_START\n"));	
		_initErrCnt = 0;
		rxFlush();
		_state = E_CWMODE;
		//processesp();
		break;
	case E_HWRESET:
		hwReset();
		_initErrCnt = 0;
		rxFlush();
		_state = E_CWMODE;
		break;
	case E_CWMODE:	
		if (++_initErrCnt > 3)
		{
			_state = E_HWRESET;
			processesp();
			break;
		}     
		debug(PSTR("E_CWMODE\n"));	
		executeCommand("AT+CWMODE=1", 2000);
		_state = E_WAIT_OK;
		_okState = E_CWJAP;		
		_errorState = E_CWMODE;
		_rxFlushProcessed = true;		
		break;
// connect to AP
	case E_CWJAP:
		_cmd="AT+CWJAP=\"";
        _cmd+=AP_SSID;
        _cmd+="\",\"";
        _cmd+=AP_PASSWORD;
        _cmd+="\"";
	    executeCommand(_cmd, 17000);
	    _state = E_WAIT_OK;
		_okState = E_CWJAP1;
		_errorState = E_START;
		_rxFlushProcessed = true;
		_connectErrCnt = 0;
		break;
// if ok then connected to AP
	case E_CWJAP1:
		if (++_connectErrCnt > 3)
		{
			_state = E_START;
			processesp();
			break;
		}
        executeCommand("AT+CWJAP?", 17000);
	    _state = E_WAIT_OK;
		_okState = E_CIPSTART;
		_errorState = E_START;
		_rxFlushProcessed = true;
		break;
// connect
	case E_CIPSTART:
		_cmd = "AT+CIPSTART=\"TCP\",\"";
		_cmd += SERVER_IP;
		_cmd += "\",";
		_cmd += SERVER_PORT;
		executeCommand(_cmd, 17000);
		_state = E_WAIT_OK;
		_okState = E_IDLE;
		_errorState = E_CIPCLOSE;
		_rxFlushProcessed = true;
		_waitingCommandResponse = false;
		setPingTimmer();		
		//resetPingTimmer();		
		break;
// close connection
	case E_CIPCLOSE:
		executeCommand("AT+CIPCLOSE", 17000);
		_state = E_WAIT_OK;
		_okState = E_CIPSTART;
		_errorState = E_CWJAP1;
		break;
	case E_IDLE:
		_connectErrCnt = 0;
		receiveAll();

		if (_rxHead != _rxTail)
		{
			//receive
			if (rxPos("+IPD", _rxHead, _rxTail, 0, 0))
				processReceive();
			else if (rxchopUntil("Unlink", true, true) || rxchopUntil("FAIL", true, true) || rxchopUntil("ERROR", true, true))
			{
				_state = E_CWJAP1;
				_connectErrCnt = 0;
				processesp();
			}
		}
		break;
	case E_CIPSEND:
		_cmd = "AT+CIPSEND="+String(_txLen);
		executeCommand(_cmd, 17000);
		_state = E_CIPSEND_1;
		
		break;
	case E_CIPSEND_1:
		receiveAll();		
		_connectErrCnt = 0;
		_rxFlushProcessed = true;				

		if (rxPos("+IPD", _rxHead, _rxTail, 0, 0))
			processReceive();
		else if (rxchopUntil(">", true, true))
		{
			debug(PSTR("Sending len:%d\n"), _txLen);	

			_serial->write(_txBuff, _txLen);

			_state = E_WAIT_OK;
			_okState = E_IDLE;
			_errorState = E_CIPSTART;

		}
		else if (isError(_rxFlushProcessed))
		{
			debug(PSTR("Response error\n"));	
			_state = E_CIPSTART;	
		}
		else if (isTimeout(_startTime, _respondTimeout))
		{
			_errorState = E_CIPSTART;
			_rxFlushProcessed = true;
		}
		break;
	default:
		break;
	}

	return _state;
}


void Esp8266EasyIoT::processReceive()
{
	byte from1, from2;

	if (rxPos("+IPD", _rxHead, _rxTail, &from1, 0))
	{			
		unsigned long recTimeout = millis();

		while(true)
		{
			if (!isTimeout(recTimeout, 2000))
			{
				if (rxPos(":", _rxHead, _rxTail, &from2, 0))
				{
					String lenstr = rxCopy(from1 + 5, from2);				
					byte packet_len = lenstr.toInt();

					byte from = (from2 + 1) & BUFFERMASK;
					byte to = (from2 + packet_len + 1) & BUFFERMASK;

					if (isInBuffer(to))
					{
						int i = 0;
						bool startFound = false;

						uint8_t* buff = reinterpret_cast<uint8_t*>(&msg);

						// copy message
						for(byte b1=from; b1!=to;b1=(b1+1)& BUFFERMASK)
						{		
							if ((_rxBuffer[b1] == START_MSG) || (startFound))
							{
								*(buff + i++) = _rxBuffer[b1]; 
								startFound = true;
							}
						}
						
						// copy rest if message at begginning
						byte cnt = 0;
						for(byte b1=to; b1!=_rxTail;b1=(b1+1)& BUFFERMASK)
						{
							_rxBuffer[(from1 + cnt) & BUFFERMASK ] = _rxBuffer[b1]; 
							cnt++;
						}
						_rxTail = (from1 + cnt) & BUFFERMASK;	
						if (startFound)
						{
							//debugPrintBuffer();
							debug(PSTR("\nProcess new message\n"));
							_newMessage = true;
							process();
						};
						return;
					}
				}
			}
			else
			{
				debug(PSTR("Receive timeout\n"));	
				_state = E_CIPCLOSE;
#ifdef DEBUG
				debugPrintBuffer();
#endif
				return;
			}
			delay(10);
			receiveAll();
		}
	}
	else
	{
		debug(PSTR("Receive no +IPD\n"));	
		_state = E_IDLE;
	}
}

bool Esp8266EasyIoT::writeesp(Esp8266EasyIoTMsg &message)
{
	if (_state == E_IDLE)
	{
		message.crc8();
		debug(PSTR("\n\nwrite: command: %d, type: %d, len: %d, sender: %d, crc:%d \n"), mGetCommand(message), message.type, message.length, mGetSender(message), message.crc);
		_txBuff = reinterpret_cast<const uint8_t*>(&message);
		_txLen = (uint8_t)(message.length + HEADER_SIZE);
		_state = E_CIPSEND;
		processesp();
		return true;
	}
	else
		return false;
}

void Esp8266EasyIoT::executeCommand(String cmd, unsigned long respondTimeout)
{
	startTimmer(respondTimeout);
	cmd += "\r\n";
	_serial->print(cmd);
	debug(PSTR("Send cmd:%s\n"), cmd.c_str());	
}

void Esp8266EasyIoT::receiveAll()
{
	while (_serial->available())
    {
		char c = _serial->read();

		#ifdef DEBUG
		_serialDebug->print(c);
		#endif

		byte aux=(_rxTail+1)& BUFFERMASK;
		if(aux!=_rxHead)
		{
			_rxBuffer[_rxTail]=c;
			_rxBuffer[aux]=0;
			_rxTail=aux;
		}
	}
}

bool Esp8266EasyIoT::rxPos(const char* reference, byte*  from, byte* to)
{
	return rxPos(reference, _rxHead, _rxTail, from, to);
}

bool Esp8266EasyIoT::rxPos(const char* reference, byte thishead, byte thistail, byte* from, byte* to)
{
	int refcursor=0;
	bool into=false;
	byte b2, binit;
	bool possible=1;
	
	if(reference[0]==0)
		return true;
		
	for(byte b1=thishead; b1!=thistail;b1=(b1+1)& BUFFERMASK)
	{
		possible = 1;
		b2 = b1;
		while (possible&&(b2!=thistail))
		{	
			if(_rxBuffer[b2]==reference[refcursor])
			{
				if(!into)	
					binit=b2;
				into=true;
				refcursor++;
				if(reference[refcursor]==0)
				{
					if(from) 
						*from=binit;
					if(to)	
						*to=b2;
					return true;
				}
			}
			else if (into==true)
			{
				possible = 0;
				into=false;
				refcursor=0;
			}
			b2=(b2+1)& BUFFERMASK;
		}
	}
	return false;
}


bool Esp8266EasyIoT::rxchopUntil(const char* reference, bool movetotheend, bool usehead)
{
	byte from, to;

	if(rxPos(reference, _rxHead, _rxTail, &from, &to))
	{
		if(usehead)
		{
			if(movetotheend)
				_rxHead=(to+1) & BUFFERMASK;
			else
				_rxHead=from;
		}
		else
		{
			if(movetotheend)
				_rxTail=(to+1) & BUFFERMASK;
			else
				_rxTail=from;
		}
		return true;
	}
	else
	{
		return false;
	}
}

String Esp8266EasyIoT::rxCopy(byte from, byte to)
{
	String ret = "";

	from = from & BUFFERMASK;
	to = to & BUFFERMASK;

	for(byte b1=from; b1!=to;b1=(b1+1)& BUFFERMASK)
		ret += _rxBuffer[b1];

	return ret;
}

bool Esp8266EasyIoT::isInBuffer(byte pos1)
{
	if (_rxTail == pos1)
		return true;

	for(byte b1=_rxHead; b1!=_rxTail;b1=(b1+1)& BUFFERMASK)
		if (pos1 == b1)
			return true;
	return false;
}

bool Esp8266EasyIoT::isOk(bool chop)
{
	receiveAll();
	if (chop)
		return rxchopUntil("ALREADY CONNECTED", true, true) || rxchopUntil("Linked", true, true) || rxchopUntil("SEND OK", true, true) || rxchopUntil("OK", true, true) || rxchopUntil("no change", true, true);
	else
		return rxPos("ALREADY CONNECTED", _rxHead, _rxTail) || rxPos("Linked", _rxHead, _rxTail) || rxPos("SEND OK", _rxHead, _rxTail) || rxPos("OK", _rxHead, _rxTail) || rxPos("no change", _rxHead, _rxTail);
}

bool Esp8266EasyIoT::isError(bool chop)
{
	receiveAll();
	if (chop)
		return rxchopUntil("link is not", true, true) || rxchopUntil("Unlink", true, true) || rxchopUntil("Error", true, true) || rxchopUntil("ERROR", true, true) || rxchopUntil("FAIL", true, true) || rxchopUntil("busy s...", true, true);
	else
		return rxPos("link is not", _rxHead, _rxTail) || rxPos("Unlink", _rxHead, _rxTail) || rxPos("Error", _rxHead, _rxTail) || rxPos("ERROR", _rxHead, _rxTail) || rxPos("FAIL", _rxHead, _rxTail) || rxPos("busy s...", _rxHead, _rxTail);
}


void Esp8266EasyIoT::rxFlush()
{
	_rxHead=_rxTail;
}

void Esp8266EasyIoT::startTimmer(unsigned long respondTimeout)
{
	_startTime = millis();
	_respondTimeout = respondTimeout;
}

bool Esp8266EasyIoT::isTimeout(unsigned long startTime, unsigned long timeout)
{
	unsigned long now = millis();

	if (startTime <= now)
	{
	 if ( (unsigned long)(now - startTime )  < timeout ) 
		return false;
	 else
        return true;
	}
	else
	{
	 if ( (unsigned long)(startTime - now) < timeout ) 
		return false;
	 else
        return true;
	}
}

#ifdef DEBUG
void Esp8266EasyIoT::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	va_list args;
	va_start (args, fmt );
	va_end (args);
	vsnprintf_P(fmtBuffer, 299, fmt, args);
	va_end (args);
	
	
	if (isDebug)
	{
		_serialDebug->print(fmtBuffer);
		_serialDebug->flush();
	}
}


void Esp8266EasyIoT::debugPrintBuffer()
{
	debug(PSTR("\n\nDEBUG buffer start\n"));
	debug(PSTR("rxBuffer:  _rxHead=%d, _rxTail=%d buffer=\n"), _rxHead, _rxTail);
	for(byte b1=_rxHead; b1!=_rxTail;b1=(b1+1)& BUFFERMASK)
		_serialDebug->print(_rxBuffer[b1]);
	debug(PSTR("\n\nDEBUG buffer end\n"));
}
#endif