/* ============================================================================
 *                   GNU Lesser General Public License
 * ============================================================================
 *
 * Copyright (C) 2005 SensorLogic, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _M2MXML_H_
#define _M2MXML_H_

#include "expat/expat.h"
#include "time.h"

#define M2MXML_UUIDSIZE						32

#define M2MXML_NOTIMESTAMP					((time_t)-1)

/* M2MXML KEYWORDS */

#define M2MXML_VERSION						"1.0"

#define M2MXML_ELEMENT_MESSAGE				"M2MXML"
#define M2MXML_ELEMENT_COMMAND				"Command"
#define M2MXML_ELEMENT_EXCEPTION			"Exception"
#define M2MXML_ELEMENT_PERCEPT				"Percept"
#define M2MXML_ELEMENT_PROPERTY				"Property"
#define M2MXML_ELEMENT_RESPONSE				"Response"

#define M2MXML_ATTRIBUTE_ADDRESS			"address"
#define M2MXML_ATTRIBUTE_EXCEPTIONCODE		"exceptionCode"
#define M2MXML_ATTRIBUTE_ENTRYTYPE			"entryType"
#define M2MXML_ATTRIBUTE_LABEL				"label"
#define M2MXML_ATTRIBUTE_NAME				"name"
#define M2MXML_ATTRIBUTE_MESSAGE			"message"
#define M2MXML_ATTRIBUTE_PERCEPTTYPE		"perceptType"
#define M2MXML_ATTRIBUTE_RESULTCODE			"resultCode"
#define M2MXML_ATTRIBUTE_SEQUENCENUMBER		"seq"
#define M2MXML_ATTRIBUTE_TELEMTRYDEVICE		"td"
#define M2MXML_ATTRIBUTE_TIMESTAMP			"timestamp"
#define M2MXML_ATTRIBUTE_TYPE				"type"
#define M2MXML_ATTRIBUTE_UNITS				"units"
#define M2MXML_ATTRIBUTE_VALUE				"value"
#define M2MXML_ATTRIBUTE_VERSION			"ver"

#define M2MXML_COMMANDID_UNKNOWN			0
#define M2MXML_COMMANDID_REQUESTPERCEPT		1
#define M2MXML_COMMANDID_TURNON				2
#define M2MXML_COMMANDID_TURNOFF			3
#define M2MXML_COMMANDID_SETSTRINGOUTPUT	4
#define M2MXML_COMMANDID_SETANALOGOUTPUT	5
#define M2MXML_COMMANDID_REBOOT				6	
#define M2MXML_COMMANDID_QUERYCONFIGURATION	7
#define M2MXML_COMMANDID_SETCONFIGURATION	8

#define M2MXML_COMMAND_REQUESTPERCEPT		"requestPercept"
#define M2MXML_COMMAND_TURNON				"turnOn"
#define M2MXML_COMMAND_TURNOFF				"turnOff"
#define M2MXML_COMMAND_SETSTRINGOUTPUT		"setStringOutput"
#define M2MXML_COMMAND_SETANALOGOUTPUT		"setAnalogOutput"
#define M2MXML_COMMAND_REBOOT				"reboot"	
#define M2MXML_COMMAND_QUERYCONFIGURATION	"queryConfiguration"
#define M2MXML_COMMAND_SETCONFIGURATION		"setConfiguration"
#define M2MXML_COMMAND_SETUPTRANSDUCER		"setupTransducer"

#define M2MXML_PROPERTY_DATA				"data"
#define M2MXML_PROPERTY_DURATION			"duration"
#define M2MXML_PROPERTY_SETPOINT			"setPoint"

#define M2MXML_PERCEPTTYPEID_UNKNOWN		0
#define M2MXML_PERCEPTTYPEID_ANALOG			1
#define M2MXML_PERCEPTTYPEID_DIGITAL		2
#define M2MXML_PERCEPTTYPEID_STRING			3
#define M2MXML_PERCEPTTYPEID_LOCATION		4

#define M2MXML_PERCEPTTYPE_ANALOG			"analog"
#define M2MXML_PERCEPTTYPE_DIGITAL			"digital"
#define M2MXML_PERCEPTTYPE_STRING			"string"
#define M2MXML_PERCEPTTYPE_LOCATION			"location"

#define M2MXML_PERCEPTPROPERTY_ANALOGIN		"analogIn"
#define M2MXML_PERCEPTPROPERTY_ANALOGOUT	"analogOut"
#define M2MXML_PERCEPTPROPERTY_DIGITALIN	"digitialIn"
#define M2MXML_PERCEPTPROPERTY_DIGITALOUT	"digitialOut"
#define M2MXML_PERCEPTPROPERTY_STRINGIN		"stringIn"
#define M2MXML_PERCEPTPROPERTY_STRINGOUT	"stringOut"
#define M2MXML_PERCEPTPROPERTY_LOCATIONIN	"locationIn"
#define M2MXML_PERCEPTPROPERTY_LOCATIONOUT	"locationOut"

#define M2MXML_ENTRYTYPEID_SCHEDULED		0
#define M2MXML_ENTRYTYPEID_REQUESTED		1
#define M2MXML_ENTRYTYPEID_BYEXCEPTION		2
#define M2MXML_ENTRYTYPEID_MANUAL			3
#define M2MXML_ENTRYTYPEID_BYACTUATOR		4
#define M2MXML_ENTRYTYPEID_OTHHER			5

#define M2MXML_EXCEPTIONCODE_HARDWARE		0
#define M2MXML_EXCEPTIONCODE_SOFTWARE		1
#define M2MXML_EXCEPTIONCODE_NOSEQNUM		2
#define M2MXML_EXCEPTIONCODE_BADMESSAGE		3

#define M2MXML_RESULTCODE_SUCCESS			0
#define M2MXML_RESULTCODE_RECEIVED			1
#define M2MXML_RESULTCODE_SUBMITTED			2
#define M2MXML_RESULTCODE_FAILEDDELIVERY	3
#define M2MXML_RESULTCODE_FAILEDEXECUTION	4
#define M2MXML_RESULTCODE_UNKNOWNCOMMAND	5
#define M2MXML_RESULTCODE_BADARGUMENT		6

//Added in protocol revision 3.0
#define M2MXML_RESULTCODE_REPLYBYURL		10 // indicates that the reply would not fit into the fixed buffer size and has to be sent via POST
#define M2MXML_RESULTCODE_DETACHFAILED		11 // indicated that the requested database detach has failed


/* M2MXML HANDLERS DEFINITION */

typedef int (*M2MXML_HANDLER_LOGERROR)(void *pAppData,int code,const char *pDescription);
typedef int (*M2MXML_HANDLER_UNKNOWN)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char *pCommand,const char **ppProperties,time_t timestamp);
typedef int (*M2MXML_HANDLER_REQUESTPERCEPT)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,time_t timestamp);
typedef int (*M2MXML_HANDLER_TURNONOFF)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,int sense,const char **ppProperties);
typedef int (*M2MXML_HANDLER_SETSTRINGOUTPUT)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char *pValue);
typedef int (*M2MXML_HANDLER_SETANALOGOUTPUT)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,double setPoint);
typedef int (*M2MXML_HANDLER_REBOOT)(void *pAppData,const char *pUUID,const char *pSeq);
typedef int (*M2MXML_HANDLER_QUERYCONFIGURATION)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char **ppProperties);
typedef int (*M2MXML_HANDLER_SETCONFIGURATION)(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char **ppProperties);

typedef struct {
	void *pAppData;
	
	M2MXML_HANDLER_LOGERROR				pLogErrorHandler;
	M2MXML_HANDLER_UNKNOWN				pUnknownCommandHandler;
	M2MXML_HANDLER_REQUESTPERCEPT		pRequestPerceptHandler;
	M2MXML_HANDLER_TURNONOFF			pTurnOnOffHandler;
	M2MXML_HANDLER_SETSTRINGOUTPUT		pSetStringOutputHandler;
	M2MXML_HANDLER_SETANALOGOUTPUT		pSetAnalogOutputHandler;
	M2MXML_HANDLER_REBOOT				pRebootHandler;
	M2MXML_HANDLER_QUERYCONFIGURATION	pQueryConfigurationHandler;
	M2MXML_HANDLER_SETCONFIGURATION		pSetConfigurationHandler;
} M2MXML_HANDLERS;

/* EXTERNAL API */

void *M2MXML_createMessage(const char *pUUID,char *pBuffer,int size);
const char *M2MXML_createProperty(void *pMessageData,const char *pKey,const char *pValue);
const char *M2MXML_createPropertySet(void *pMessageData,int propertyCount);
const char *M2MXML_createCommand(void *pMessageData,const char *pSeq,const char *pAddressName,const char *pCommandName,const char **ppProperties,int requirePropertyValues,time_t timestamp);
const char *M2MXML_createException(void *pMessageData,int exceptionCode,const char *pOptionalMessage);
const char *M2MXML_createAnyPercept(void *pMessageData,const char *pSeq,const char *pAddressName,const char *pValue,const char *pType,int entry,time_t timestamp);
const char *M2MXML_createAnalogPercept(void *pMessageData,const char *pSeq,const char *pAddressName,double value,int entry,time_t timestamp);
const char *M2MXML_createDigitalPercept(void *pMessageData,const char *pSeq,const char *pAddressName,int value,int entry,time_t timestamp);
const char *M2MXML_createLocationPercept(void *pMessageData,const char *pSeq,const char *pAddressName,double latValue,double longValue,int entry,time_t timestamp);
const char *M2MXML_createStringPercept(void *pMessageData,const char *pSeq,const char *pAddressName,const char *pValue,int entry,time_t timestamp);
const char *M2MXML_createResponse(void *pMessageData,const char *pSeq,int resultCode,const char *pOptionalMessage,time_t timestamp);
int M2MXML_formatMessage(void *pMessageData,char *pBuffer,int maxSize);
const char *M2MXML_getPropertyString(const char **ppProperties,const char *pKey,const char *pDefault);
void M2MXML_parseMessage(M2MXML_HANDLERS *pHandlers,const char *pMessageBuffer,int messageSize,int maxProperties,char *pParserBuffer,int parserSize);
void M2MXML_setTimezoneOffset(int timezoneOffset);

#endif
