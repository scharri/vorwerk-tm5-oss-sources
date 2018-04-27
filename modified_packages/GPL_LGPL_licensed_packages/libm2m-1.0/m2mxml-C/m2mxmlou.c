/* ============================================================================
 *				   GNU Lesser General Public License
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "m2mmem.h"
#include "m2mxml.h"
#include "m2mitoa.h"

/* ERROR MESSAGES */

#define ERROR_ADDRESSNAMEREQUIRED	"Address name required for percept"
#define ERROR_COMMANDNAMEREQUIRED	"Command name required"
#define ERROR_NOTENOUGHBUFFER		"Not enough message buffer space; increase buffer size and recompile"
#define ERROR_SEQUENCENUMBERNEEDED	"Sequence number required"
#define ERROR_TOOMANYPROPERTIES		"Too many properties"
#define ERROR_VALUEREQUIRED			"Value required for percept"

/* M2MXML OBJECT MODEL */

typedef struct _M2MXML_COMMAND {
	const char *pSeq;
	const char *pAddressName;
	const char *pCommandName;
	const char **ppProperties;
	int requirePropertyValues;
	time_t timestamp;
	struct _M2MXML_COMMAND *pNext;
} M2MXML_COMMAND;

typedef struct _M2MXML_PERCEPT {
	const char *pSeq;
	const char *pAddressName;
	const char *pValue;
	const char *pType;
	int entry;
	time_t timestamp;
	struct _M2MXML_PERCEPT *pNext;
} M2MXML_PERCEPT;

typedef struct _M2MXML_EXCEPTION {
	int exceptionCode;
	const char *pOptionalMessage;
	struct _M2MXML_EXCEPTION *pNext;
} M2MXML_EXCEPTION;

typedef struct _M2MXML_RESPONSE {
	const char *pSeq;
	int resultCode;
	const char *pOptionalMessage;
	time_t timestamp;
	struct _M2MXML_RESPONSE *pNext;
} M2MXML_RESPONSE;

typedef struct {
	const char *pUUID;
	const char **ppProperties;
	int usedPropertyCount;
	int maxPropertyCount;
	M2MXML_COMMAND *pCommands;
	M2MXML_PERCEPT *pPercepts;
	M2MXML_EXCEPTION *pExceptions;
	M2MXML_RESPONSE *pResponses;
	M2MMEM_BUFFER buffer;
} M2MXML_MESSAGE;

/* GLOBAL VARIABLES */

static int _timezoneOffset = 0;

/* GENERIC XML FORMATTING HELPER FUNCTIONS */

static int openElement(M2MMEM_BUFFER *pBuffer,const char *pElementName) {
	return
		M2MMEM_appendString(pBuffer,"<") &&
		M2MMEM_appendString(pBuffer,pElementName);
}

static int continueElement(M2MMEM_BUFFER *pBuffer) {
	return M2MMEM_appendString(pBuffer,">") != NULL;
}

static int closeElement(M2MMEM_BUFFER *pBuffer,const char *pElementName) {
	return pElementName ?
		M2MMEM_appendString(pBuffer,"</") && M2MMEM_appendString(pBuffer,pElementName) && M2MMEM_appendString(pBuffer,">") :
		M2MMEM_appendString(pBuffer,"/>") != NULL;
		
}

/* M2MXML FORMATTING HELPER FUNCTIONS */

static int addAttribute(M2MMEM_BUFFER *pBuffer,const char *pKey,const char *pValue) {
	return
		M2MMEM_appendString(pBuffer," ") &&
		M2MMEM_appendString(pBuffer,pKey) &&
		M2MMEM_appendString(pBuffer,"=\"") &&
		M2MMEM_appendString(pBuffer,pValue) &&
		M2MMEM_appendString(pBuffer,"\"");
}

static int addTimestamp(M2MMEM_BUFFER *pBuffer,time_t timestamp) {
	char timeBuffer[64];
	struct tm *pTimestamp;
	
	if (timestamp == M2MXML_NOTIMESTAMP)
		return XML_TRUE;
	timestamp += _timezoneOffset;
	pTimestamp = localtime(&timestamp);
	if (pTimestamp == NULL)
		return XML_FALSE;
	sprintf(timeBuffer,"%04d%02d%02d%02d%02d%02d",
		pTimestamp->tm_year + 1900,
		pTimestamp->tm_mon + 1,
		pTimestamp->tm_mday,
		pTimestamp->tm_hour,
		pTimestamp->tm_min,
		pTimestamp->tm_sec);
	return addAttribute(pBuffer,M2MXML_ATTRIBUTE_TIMESTAMP,timeBuffer);
}

static int addProperty(M2MMEM_BUFFER *pBuffer,const char *pKey,const char *pValue) {
	return
		openElement(pBuffer,M2MXML_ELEMENT_PROPERTY) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_NAME,pKey) &&
		(pValue == NULL || addAttribute(pBuffer,M2MXML_ATTRIBUTE_VALUE,pValue)) &&
		closeElement(pBuffer,NULL);
}

static int addProperties(M2MMEM_BUFFER *pBuffer,const char **ppProperties,int requirePropertyValues) {
	const char **ppNextProperty = ppProperties;
	
	for (ppNextProperty = ppProperties; *ppNextProperty; ppNextProperty += 2)
		if (!addProperty(pBuffer,ppNextProperty[0],requirePropertyValues ? ppNextProperty[1] : NULL))
			return XML_FALSE;
	return XML_TRUE;
}

static M2MXML_COMMAND *addCommand(M2MMEM_BUFFER *pBuffer,M2MXML_COMMAND *pCommand) {
	if (!openElement(pBuffer,M2MXML_ELEMENT_COMMAND) ||
		!addAttribute(pBuffer,M2MXML_ATTRIBUTE_NAME,pCommand->pCommandName) ||
		(pCommand->pAddressName && !addAttribute(pBuffer,M2MXML_ATTRIBUTE_ADDRESS,pCommand->pAddressName)) ||
		(pCommand->pSeq && !addAttribute(pBuffer,M2MXML_ATTRIBUTE_SEQUENCENUMBER,pCommand->pSeq)) ||
		!addTimestamp(pBuffer,pCommand->timestamp))
		return NULL;
	if (pCommand->ppProperties == NULL || *pCommand->ppProperties == NULL) {
		if (!closeElement(pBuffer,NULL))
			return NULL;
	} else {
		if (!continueElement(pBuffer) ||
			!addProperties(pBuffer,pCommand->ppProperties,pCommand->requirePropertyValues) ||
			!closeElement(pBuffer,M2MXML_ELEMENT_COMMAND))
			return NULL;
	}
	return pCommand->pNext;
}

static M2MXML_PERCEPT *addPercept(M2MMEM_BUFFER *pBuffer,M2MXML_PERCEPT *pPercept) {
	char entryBuffer[32];
	
	m2mitoa(pPercept->entry,entryBuffer,sizeof(entryBuffer));
	return
		openElement(pBuffer,M2MXML_ELEMENT_PERCEPT) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_ADDRESS,pPercept->pAddressName) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_VALUE,pPercept->pValue) &&
		(pPercept->pType == NULL || addAttribute(pBuffer,M2MXML_ATTRIBUTE_PERCEPTTYPE,pPercept->pType)) &&
		(pPercept->pSeq == NULL || addAttribute(pBuffer,M2MXML_ATTRIBUTE_SEQUENCENUMBER,pPercept->pSeq)) &&
		(pPercept->entry == M2MXML_ENTRYTYPEID_SCHEDULED || addAttribute(pBuffer,M2MXML_ATTRIBUTE_ENTRYTYPE,entryBuffer)) &&
		addTimestamp(pBuffer,pPercept->timestamp) &&
		closeElement(pBuffer,NULL) ?
		pPercept->pNext : NULL;
}

static M2MXML_EXCEPTION *addException(M2MMEM_BUFFER *pBuffer,M2MXML_EXCEPTION *pException) {
	char valueBuffer[32];
	
	m2mitoa(pException->exceptionCode,valueBuffer,sizeof(valueBuffer));
	return
		openElement(pBuffer,M2MXML_ELEMENT_EXCEPTION) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_EXCEPTIONCODE,valueBuffer) &&
		(pException->pOptionalMessage == NULL || addAttribute(pBuffer,M2MXML_ATTRIBUTE_MESSAGE,pException->pOptionalMessage)) &&
		closeElement(pBuffer,NULL) ?
		pException->pNext : NULL;
}

static M2MXML_RESPONSE *addResponseWithProperties(M2MMEM_BUFFER *pBuffer,M2MXML_RESPONSE *pResponse, const char **ppProperties) {
	char resultBuffer[32];
	
	m2mitoa(pResponse->resultCode,resultBuffer,sizeof(resultBuffer));
	return		
		openElement(pBuffer,M2MXML_ELEMENT_RESPONSE) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_RESULTCODE,resultBuffer) &&
		(pResponse->pOptionalMessage == NULL || addAttribute(pBuffer,M2MXML_ATTRIBUTE_MESSAGE,pResponse->pOptionalMessage)) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_SEQUENCENUMBER,pResponse->pSeq) &&
		addTimestamp(pBuffer,pResponse->timestamp) &&
		continueElement(pBuffer) &&
		addProperties(pBuffer, ppProperties, XML_TRUE) &&
		closeElement(pBuffer,M2MXML_ELEMENT_RESPONSE) ?
		pResponse->pNext : NULL;
}

static M2MXML_RESPONSE *addResponse(M2MMEM_BUFFER *pBuffer,M2MXML_RESPONSE *pResponse) {
	char resultBuffer[32];
	
	m2mitoa(pResponse->resultCode,resultBuffer,sizeof(resultBuffer));
	return		
		openElement(pBuffer,M2MXML_ELEMENT_RESPONSE) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_RESULTCODE,resultBuffer) &&
		(pResponse->pOptionalMessage == NULL || addAttribute(pBuffer,M2MXML_ATTRIBUTE_MESSAGE,pResponse->pOptionalMessage)) &&
		addAttribute(pBuffer,M2MXML_ATTRIBUTE_SEQUENCENUMBER,pResponse->pSeq) &&
		addTimestamp(pBuffer,pResponse->timestamp) &&
		closeElement(pBuffer,NULL) ?
		pResponse->pNext : NULL;
}

/* EXTERNAL API */

void *M2MXML_createMessage(const char *pUUID,char *pBuffer,int size) {
	M2MXML_MESSAGE *pMessage;
	
	if (size <= sizeof(M2MXML_MESSAGE))
		return NULL;
	pMessage = (M2MXML_MESSAGE*)pBuffer;
	memset(pMessage,0,sizeof(*pMessage));
	M2MMEM_initializeBuffer(&pMessage->buffer,pBuffer + sizeof(*pMessage),size - sizeof(*pMessage));
	if (pUUID != NULL)
		pMessage->pUUID = M2MMEM_allocateString(&pMessage->buffer,pUUID);
	return pMessage;
}

const char *M2MXML_createProperty(void *pMessageData,const char *pKey,const char *pValue) {
	M2MXML_MESSAGE *pMessage;
	const char **ppProperty;
	
	pMessage = (M2MXML_MESSAGE*)pMessageData;
	if (pMessage->usedPropertyCount >= pMessage->maxPropertyCount)
		return ERROR_TOOMANYPROPERTIES;
	ppProperty = pMessage->ppProperties + pMessage->usedPropertyCount * 2;
	if ((ppProperty[0] = M2MMEM_allocateString(&pMessage->buffer,pKey)) == NULL ||
		(ppProperty[1] = pValue != NULL ? M2MMEM_allocateString(&pMessage->buffer,pValue) : "") == NULL)
		return ERROR_NOTENOUGHBUFFER;
	ppProperty[2] = NULL;
	pMessage->usedPropertyCount++;
	return NULL;
}

const char *M2MXML_createPropertySet(void *pMessageData,int propertyCount) {
	M2MXML_MESSAGE *pMessage;

	pMessage = (M2MXML_MESSAGE*)pMessageData;
	pMessage->ppProperties = M2MMEM_allocatePropertySet(&pMessage->buffer,propertyCount);
	if (pMessage->ppProperties == NULL)
		return ERROR_NOTENOUGHBUFFER;
	pMessage->ppProperties[0] = NULL;
	pMessage->usedPropertyCount = 0;
	pMessage->maxPropertyCount = propertyCount;
	return NULL;
}

const char *M2MXML_createCommand(void *pMessageData,const char *pSeq,const char *pAddressName,const char *pCommandName,const char **ppProperties,int requirePropertyValues,time_t timestamp) {
	M2MXML_MESSAGE *pMessage;
	M2MXML_COMMAND *pCommand;
	M2MXML_COMMAND *pLast;

	pMessage = (M2MXML_MESSAGE*)pMessageData;
	if (pCommandName == NULL)
		return ERROR_COMMANDNAMEREQUIRED;
	if ((pSeq != NULL && (pSeq = M2MMEM_allocateString(&pMessage->buffer,pSeq)) == NULL) ||
		(pCommandName = M2MMEM_allocateString(&pMessage->buffer,pCommandName)) == NULL ||
		(pAddressName != NULL && (pAddressName = M2MMEM_allocateString(&pMessage->buffer,pAddressName)) == NULL) ||
		(ppProperties != NULL && (ppProperties = M2MMEM_allocateProperties(&pMessage->buffer,ppProperties)) == NULL) ||
		(pCommand = (M2MXML_COMMAND*)M2MMEM_allocateSpace(&pMessage->buffer,sizeof(M2MXML_COMMAND))) == NULL)
		return ERROR_NOTENOUGHBUFFER;
	pCommand->pSeq = pSeq;
	pCommand->pAddressName = pAddressName;
	pCommand->pCommandName = pCommandName;
	pCommand->ppProperties = ppProperties;
	pCommand->requirePropertyValues = requirePropertyValues;
	pCommand->timestamp = timestamp;
	pCommand->pNext = NULL;
	if ((pLast = pMessage->pCommands) == NULL)
		pMessage->pCommands = pCommand;
	else {
		while (pLast->pNext != NULL)
			pLast = pLast->pNext;
		pLast->pNext = pCommand;
	}
	return NULL;
}

const char *M2MXML_createAnyPercept(void *pMessageData,const char *pSeq,const char *pAddressName,const char *pValue,const char *pType,int entry,time_t timestamp) {
	M2MXML_PERCEPT *pPercept;
	M2MXML_MESSAGE *pMessage;
	M2MXML_PERCEPT *pLast;
	
	if (pAddressName == NULL)
		return ERROR_ADDRESSNAMEREQUIRED;
	if (pValue == NULL)
		return ERROR_VALUEREQUIRED;
	pMessage = (M2MXML_MESSAGE*)pMessageData;
	if ((pSeq != NULL && (pSeq = M2MMEM_allocateString(&pMessage->buffer,pSeq)) == NULL) ||
		(pAddressName = M2MMEM_allocateString(&pMessage->buffer,pAddressName)) == NULL ||
		(pValue = M2MMEM_allocateString(&pMessage->buffer,pValue)) == NULL ||
		(pType != NULL && (pType = M2MMEM_allocateString(&pMessage->buffer,pType)) == NULL) ||
		(pPercept = (M2MXML_PERCEPT*)M2MMEM_allocateSpace(&pMessage->buffer,sizeof(M2MXML_PERCEPT))) == NULL)
		return ERROR_NOTENOUGHBUFFER;
	pPercept->pSeq = pSeq;
	pPercept->pAddressName = pAddressName;
	pPercept->pValue = pValue;
	pPercept->pType = pType;
	pPercept->entry = entry;
	pPercept->timestamp = timestamp;
	pPercept->pNext = NULL;
	if ((pLast = pMessage->pPercepts) == NULL)
		pMessage->pPercepts = pPercept;
	else {
		while (pLast->pNext != NULL)
			pLast = pLast->pNext;
		pLast->pNext = pPercept;
	}
	return NULL;
}

const char *M2MXML_createAnalogPercept(void *pMessageData,const char *pSeq,const char *pAddressName,double value,int entry,time_t timestamp) {
	char valueBuffer[32];
	
	sprintf(valueBuffer,"%lf",value);
	return M2MXML_createAnyPercept((M2MXML_MESSAGE*)pMessageData,pSeq,pAddressName,valueBuffer,NULL,entry,timestamp);
}

const char *M2MXML_createDigitalPercept(void *pMessageData,const char *pSeq,const char *pAddressName,int value,int entry,time_t timestamp) {
	return M2MXML_createAnyPercept((M2MXML_MESSAGE*)pMessageData,pSeq,pAddressName,value ? "1" : "0",M2MXML_PERCEPTTYPE_DIGITAL,entry,timestamp);
}

const char *M2MXML_createLocationPercept(void *pMessageData,const char *pSeq,const char *pAddressName,double latValue,double longValue,int entry,time_t timestamp) {
	char valueBuffer[64];
	
	sprintf(valueBuffer,"%lf,%lf",latValue,longValue);
	return M2MXML_createAnyPercept((M2MXML_MESSAGE*)pMessageData,pSeq,pAddressName,valueBuffer,M2MXML_PERCEPTTYPE_LOCATION,entry,timestamp);
}

const char *M2MXML_createStringPercept(void *pMessageData,const char *pSeq,const char *pAddressName,const char *pValue,int entry,time_t timestamp) {
	return M2MXML_createAnyPercept((M2MXML_MESSAGE*)pMessageData,pSeq,pAddressName,pValue,M2MXML_PERCEPTTYPE_STRING,entry,timestamp);
}

const char *M2MXML_createException(void *pMessageData,int exceptionCode,const char *pOptionalMessage) {
	M2MXML_EXCEPTION *pException;
	M2MXML_MESSAGE *pMessage;
	M2MXML_EXCEPTION *pLast;
	
	pMessage = (M2MXML_MESSAGE*)pMessageData;
	if ((pOptionalMessage != NULL && (pOptionalMessage = M2MMEM_allocateString(&pMessage->buffer,pOptionalMessage)) == NULL) ||
		(pException = (M2MXML_EXCEPTION*)M2MMEM_allocateSpace(&pMessage->buffer,sizeof(M2MXML_EXCEPTION))) == NULL)
		return ERROR_NOTENOUGHBUFFER;
	pException->exceptionCode = exceptionCode;
	pException->pOptionalMessage = pOptionalMessage;
	pException->pNext = NULL;
	if ((pLast = pMessage->pExceptions) == NULL)
		pMessage->pExceptions = pException;
	else {
		while (pLast->pNext != NULL)
			pLast = pLast->pNext;
		pLast->pNext = pException;
	}
	return NULL;
}

const char *M2MXML_createResponse(void *pMessageData,const char *pSeq,int resultCode,const char *pOptionalMessage,time_t timestamp) {
	M2MXML_RESPONSE *pResponse;
	M2MXML_MESSAGE *pMessage;
	M2MXML_RESPONSE *pLast;
	
	if (pSeq == NULL)
		return ERROR_SEQUENCENUMBERNEEDED;
	pMessage = (M2MXML_MESSAGE*)pMessageData;
	if ((pSeq = M2MMEM_allocateString(&pMessage->buffer,pSeq)) == NULL ||
		(pOptionalMessage != NULL && (pOptionalMessage = M2MMEM_allocateString(&pMessage->buffer,pOptionalMessage)) == NULL) ||
		(pResponse = (M2MXML_RESPONSE*)M2MMEM_allocateSpace(&pMessage->buffer,sizeof(M2MXML_RESPONSE))) == NULL)
		return ERROR_NOTENOUGHBUFFER;
	pResponse->pSeq = pSeq;
	pResponse->resultCode = resultCode;
	pResponse->pOptionalMessage = pOptionalMessage;
	pResponse->timestamp = timestamp;
	pResponse->pNext = NULL;
	if ((pLast = pMessage->pResponses) == NULL)
		pMessage->pResponses = pResponse;
	else {
		while (pLast->pNext != NULL)
			pLast = pLast->pNext;
		pLast->pNext = pResponse;
	}
	return NULL;
}

int M2MXML_formatMessage(void *pMessageData,char *pFormatBuffer,int formatSize) {
	M2MMEM_BUFFER buffer;
	M2MXML_MESSAGE *pMessage;
	M2MXML_COMMAND *pNextCommand;
	M2MXML_PERCEPT *pNextPercept;
	M2MXML_EXCEPTION *pNextException;
	M2MXML_RESPONSE *pNextResponse;
	int iNofResponses = 0;
	
	M2MMEM_initializeBuffer(&buffer,pFormatBuffer,formatSize);
	openElement(&buffer,M2MXML_ELEMENT_MESSAGE);
	addAttribute(&buffer,M2MXML_ATTRIBUTE_VERSION,M2MXML_VERSION);
	
	pMessage = (M2MXML_MESSAGE*)pMessageData;
	if (pMessage->pUUID != NULL)
		addAttribute(&buffer,M2MXML_ATTRIBUTE_TELEMTRYDEVICE,pMessage->pUUID);
	continueElement(&buffer);
	
	pNextCommand = pMessage->pCommands;
	while (pNextCommand)
		pNextCommand = addCommand(&buffer,pNextCommand);
	
	pNextPercept = pMessage->pPercepts;
	while (pNextPercept)
		pNextPercept = addPercept(&buffer,pNextPercept);
	
	pNextException = pMessage->pExceptions;
	while (pNextException)
		pNextException = addException(&buffer,pNextException);
	
	pNextResponse = pMessage->pResponses;
	while (pNextResponse)
	{
		if((iNofResponses == 0) && (pMessage->ppProperties != NULL))
		{
			pNextResponse = addResponseWithProperties(&buffer,pNextResponse,pMessage->ppProperties);
		}
		else
		{
			pNextResponse = addResponse(&buffer,pNextResponse);
		}
		iNofResponses++;
	}

	if(iNofResponses == 0)
	{
		if (pMessage->ppProperties)
			addProperties(&buffer,pMessage->ppProperties,XML_TRUE);
	}

	closeElement(&buffer,M2MXML_ELEMENT_MESSAGE);
	return buffer.maxSize >= 0 ? buffer.usedCount : -1;
}

void M2MXML_setTimezoneOffset(int timezoneOffset) {
	_timezoneOffset = timezoneOffset * 60; /* should be in minutes, turn into seconds */
}

