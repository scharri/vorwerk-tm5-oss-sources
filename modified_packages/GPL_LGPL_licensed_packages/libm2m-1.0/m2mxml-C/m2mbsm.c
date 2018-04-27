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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m2mmem.h"
#include "m2mbsm.h"
#include "m2mitoa.h"

#define MAX_SEQUENCENUMBERS				16
#define SEQUENCENUMBER_SIZE				16

/* ERROR MESSAGES */

#define ERROR_INCLUDEBEHAVIORFAILURE	"Cannot add a behavior; increase buffer and recompile"
#define ERROR_ADDRESSNOTFOUND			"Address required, but not found"
#define ERROR_CREATEMESSAGEFAILURE		"Cannot create message buffer; increase buffer and recompile"
#define ERROR_CREATEPROPERTYFAILURE		"Cannot create properties; increase buffer and recompile"
#define ERROR_EXCLUDEBEHAVIORFAILURE	"Cannot find behavior to exclude"
#define ERROR_DUPLICATECOMMAND			"Duplicate command received"
#define ERROR_INVALIDTRANSDUCER			"Invalid transducer"
#define ERROR_PROPERTYNOTFOUND			"Property not found"
#define ERROR_UNHANDLEDCOMMAND			"Unhandled command"
#define ERROR_UUIDMISMATCH				"Device and M2MXML message UUIDs are not the same"

/* STATE DEFINITION */

typedef struct {
	M2MXML_HANDLERS baseHandlers;
	M2MBSM_HANDLERS appHandlers;
	M2MBSM_DEVICE *pDevice;
	M2MBSM_BEHAVIOR *pNextBehavior;
	M2MBSM_BEHAVIOR *pFreeBehavior;
	int maxProperties;
	char *pPropertyBuffer;
	int propertySize;
	char *pParserBuffer;
	int parserSize;
	char *pFormatBuffer;
	int formatSize;
	char commandsSeen[MAX_SEQUENCENUMBERS][SEQUENCENUMBER_SIZE];
	int lastCommandIndex;
	int commandCount;
	time_t processTime;
} M2MBSM_STATE;

static M2MBSM_STATE state;

/* GENERAL HELPER FUNCTIONS */

static const char *getPerceptType(int typeID,int nullDefault) {
	switch(typeID) {
	case M2MXML_PERCEPTTYPEID_ANALOG:
		return nullDefault ? NULL : M2MXML_PERCEPTTYPE_ANALOG;
	case M2MXML_PERCEPTTYPEID_DIGITAL:
		return M2MXML_PERCEPTTYPE_DIGITAL;
	case M2MXML_PERCEPTTYPEID_STRING:
		return M2MXML_PERCEPTTYPE_STRING;
	case M2MXML_PERCEPTTYPEID_LOCATION:
		return M2MXML_PERCEPTTYPE_LOCATION;
	}
	return "unknown";
}

static const char *getPerceptProperty(int typeID,int inout,int nullDefault) {
	int out;
	
	out = inout & M2MBSM_DIRECTION_OUT;
	switch(typeID) {
	case M2MXML_PERCEPTTYPEID_ANALOG:
		return out ? M2MXML_PERCEPTPROPERTY_ANALOGOUT : nullDefault ? NULL : M2MXML_PERCEPTPROPERTY_ANALOGIN;
	case M2MXML_PERCEPTTYPEID_DIGITAL:
		return out ? M2MXML_PERCEPTPROPERTY_DIGITALOUT : M2MXML_PERCEPTPROPERTY_DIGITALIN;
	case M2MXML_PERCEPTTYPEID_STRING:
		return out ? M2MXML_PERCEPTPROPERTY_STRINGOUT : M2MXML_PERCEPTPROPERTY_STRINGIN;
	case M2MXML_PERCEPTTYPEID_LOCATION:
		return out ? M2MXML_PERCEPTPROPERTY_LOCATIONOUT : M2MXML_PERCEPTPROPERTY_LOCATIONIN;
	}
	return out ? "unknownOut" : "unknownIn";
}

static int getTargetTransducerIndex(const char *pAddress) {
	int index;
	int count;
	M2MBSM_TRANSDUCER *pTransducer;
	
	if (pAddress == NULL)
		return -1;
	index = 0;
	count = state.pDevice->transducerCount;
	pTransducer = state.pDevice->pTransducers;
	for (; index < count; pTransducer++, index++)
		if (strcmp(pAddress,pTransducer->pAddress) == 0)
			return index;
	return -1;
}

static void logError(int code,const char *pDescription) {
	if (state.baseHandlers.pLogErrorHandler)
		(*state.baseHandlers.pLogErrorHandler)(state.baseHandlers.pAppData,code,pDescription);
}

/* MESSAGE SENDING HELPER FUNCTIONS */

static void sendMessage() {
	if (state.appHandlers.pSendMessageHandler)
		(*state.appHandlers.pSendMessageHandler)(state.pDevice,state.pFormatBuffer);
}

static void sendException(int exceptionCode,const char *pExceptionMessage) {
	const char *pError;
	
	logError(-1,pExceptionMessage);
	if (!M2MXML_createMessage(state.pDevice->pUUID,state.pFormatBuffer,state.formatSize))
		logError(-1,ERROR_CREATEMESSAGEFAILURE);
	else if ((pError = M2MXML_createException(state.pFormatBuffer,exceptionCode,pExceptionMessage)) != NULL)
		logError(-1,pError);
	else
		sendMessage();
}

static void sendResponse(const char *pError,const char *pSeq,int resultCode,const char *pOptionalMessage,time_t timestamp) {
	if (pError != NULL || (pSeq != NULL && (pError = M2MXML_createResponse(state.pFormatBuffer,pSeq,resultCode,pOptionalMessage,timestamp)) != NULL))
		logError(-1,pError);
	else
		sendMessage();
}

/* BEHAVIOR MANAGEMENT FUNCTIONS */

void M2MBSM_excludeBehavior(M2MBSM_BEHAVIOR *pBehavior) {
	M2MBSM_BEHAVIOR *pPrevious;
	M2MBSM_BEHAVIOR *pCurrent;
	
	pPrevious = NULL;
	if ((pCurrent = state.pNextBehavior) == NULL) {
		logError(-1,ERROR_EXCLUDEBEHAVIORFAILURE);
		return;
	}
	do {
		if (pPrevious != NULL && pCurrent == pBehavior)
			break;
		pPrevious = pCurrent;
		pCurrent = pCurrent->pNext;
	} while (pCurrent != state.pNextBehavior);
	if (pPrevious == NULL || pCurrent == NULL) {
		logError(-1,ERROR_EXCLUDEBEHAVIORFAILURE);
		return;
	}
	if (pCurrent == pPrevious)
		state.pNextBehavior = NULL;
	else {
		if (state.pNextBehavior == pCurrent)
			state.pNextBehavior = pCurrent->pNext;
		pPrevious->pNext = pCurrent->pNext;
	}
	pCurrent->pNext = state.pFreeBehavior;
	state.pFreeBehavior = pCurrent;
}

static M2MBSM_BEHAVIOR *includeBehavior(M2MBSM_TRANSDUCER *pTransducer,M2MBSM_HANDLER_BEHAVIOR pBehaviorHandler) {
	M2MBSM_BEHAVIOR *pResult;
	M2MBSM_BEHAVIOR *pCurrent;
	
	if (state.pNextBehavior != NULL) {
		pCurrent = state.pNextBehavior;
		do {
			if (pTransducer == pCurrent->pTransducer && (void*)pBehaviorHandler == pCurrent->pHandler)
				return pCurrent;
			pCurrent = pCurrent->pNext;
		} while (pCurrent != state.pNextBehavior);
	}
	if ((pResult = state.pFreeBehavior) == NULL) {
		logError(-1,ERROR_INCLUDEBEHAVIORFAILURE);
		return NULL;
	}
	state.pFreeBehavior = pResult->pNext;
	memset(pResult,0,sizeof(*pResult));
	pResult->pTransducer = pTransducer;
	pResult->pHandler = pBehaviorHandler;
	if (state.pNextBehavior == NULL)
		state.pNextBehavior = pResult->pNext = pResult;
	else {
		pResult->pNext = state.pNextBehavior->pNext;
		state.pNextBehavior->pNext = pResult;
		state.pNextBehavior = pResult;
	}
	return pResult;
}

static int configureStandardBehavior(M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty) {
	static M2MBSM_HANDLER_BEHAVIOR behaviorHandlers[] = {
		&M2MBSM_reportAllAbsoluteBehaviorHandler,
		&M2MBSM_reportPointAbsoluteBehaviorHandler,
		&M2MBSM_reportAllIntervalBehaviorHandler,
		&M2MBSM_reportPointIntervalBehaviorHandler,
		&M2MBSM_reportDigitalTransitionBehaviorHandler,
		&M2MBSM_reportOutOfBoundsBehaviorHandler,
		&M2MBSM_timerOffAbsoluteBehaviorHandler,
		&M2MBSM_timerOnAbsoluteBehaviorHandler,
		&M2MBSM_timerOffIntervalBehaviorHandler,
		&M2MBSM_timerOnIntervalBehaviorHandler
	};
	int index;
	
	for (index = 0; index < M2MBSM_ARRAYCOUNT(behaviorHandlers); index++)
		if (M2MBSM_configureBehavior(pTransducer,pProperty,behaviorHandlers[index]))
			return XML_TRUE;
	return XML_FALSE;
}

/* MESSAGE PREPARATION HELPER FUNCTION */

static void checkUUID(const char *pUUID) {
	if (pUUID != NULL && state.pDevice->pUUID != NULL && strcmp(pUUID,state.pDevice->pUUID) != 0)
		logError(-1,ERROR_UUIDMISMATCH);
}

static int checkSeq(const char *pSeq) {
	int index;
	
	if (pSeq == NULL)
		return XML_TRUE;
	for (index = 0; index < state.commandCount; index++)
		if (strcmp(pSeq,state.commandsSeen[index]) == 0) {
			logError(-1,ERROR_DUPLICATECOMMAND);
			return XML_FALSE;
		}
	if (state.commandCount < MAX_SEQUENCENUMBERS)
		strcpy(state.commandsSeen[state.lastCommandIndex = state.commandCount++],pSeq);
	else {
		state.lastCommandIndex = (state.lastCommandIndex + 1) % MAX_SEQUENCENUMBERS;
		strcpy(state.commandsSeen[state.lastCommandIndex],pSeq);
	}
	return XML_TRUE;
}

static int prepareMessage(const char *pSeq) {
	if (!checkSeq(pSeq))
		return XML_FALSE;
	if (M2MXML_createMessage(state.pDevice->pUUID,state.pFormatBuffer,state.formatSize))
		return XML_TRUE;
	logError(-1,ERROR_CREATEMESSAGEFAILURE);
	return  XML_FALSE;
}

static M2MBSM_TRANSDUCER *prepareSetTransducerMessage(const char *pSeq,const char *pAddress,int type,void *pHandler) {
	int index;
	M2MBSM_TRANSDUCER *pTransducer;
	
	if (!prepareMessage(pSeq))
		return NULL;
	if (pAddress == NULL) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_FAILEDEXECUTION,ERROR_ADDRESSNOTFOUND,M2MXML_NOTIMESTAMP);
		return NULL;
	}
	if (pHandler == NULL) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_FAILEDEXECUTION,ERROR_UNHANDLEDCOMMAND,M2MXML_NOTIMESTAMP);
		return NULL;
	}
	if ((index = getTargetTransducerIndex(pAddress)) < 0) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_BADARGUMENT,pAddress,M2MXML_NOTIMESTAMP);
		return NULL;
	}
	pTransducer = state.pDevice->pTransducers + index;
	if (type != M2MXML_PERCEPTTYPEID_UNKNOWN && (type != pTransducer->type || !(pTransducer->inout & M2MBSM_DIRECTION_OUT))) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_FAILEDEXECUTION,ERROR_INVALIDTRANSDUCER,M2MXML_NOTIMESTAMP);
		return NULL;
	}
	return pTransducer;
}

static void sendSinglePercept(const char *pSeq,const char *pAddress,int updatePercept,int entryType,time_t timestamp) {
	int resultCode;
	const char *pError;
	const char *pOptionalMessage;
	M2MBSM_TRANSDUCER *pTransducer;
	
	if ((pTransducer = prepareSetTransducerMessage(pSeq,pAddress,M2MXML_PERCEPTTYPEID_UNKNOWN,(void*)(state.appHandlers.pRequestPerceptHandler))) == NULL)
		return;
	pError = NULL;
	pOptionalMessage = NULL;
	if ((resultCode = updatePercept ? (*state.appHandlers.pRequestPerceptHandler)(state.pDevice,pTransducer,&timestamp) : M2MXML_RESULTCODE_SUCCESS) == M2MXML_RESULTCODE_SUCCESS)
		pError = M2MXML_createAnyPercept(state.pFormatBuffer,pSeq,pAddress,pTransducer->currentValue.pData,getPerceptType(pTransducer->type,XML_TRUE),entryType,timestamp);
	else {
		pOptionalMessage = pAddress;
		timestamp = M2MXML_NOTIMESTAMP;
	}
	sendResponse(pError,pSeq,resultCode,pOptionalMessage,timestamp);
}

/* M2MXML EVENT HANDLERS */

static int logErrorHandler(void *pAppData,int code,const char *pDescription) {
	return
		state.appHandlers.pLogErrorHandler &&
		(*state.appHandlers.pLogErrorHandler)(state.appHandlers.pAppData,code,pDescription);
}

static int unknownCommandHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char *pCommand,const char **ppProperties,time_t timestamp) {
	checkUUID(pUUID);
	if (!prepareMessage(pSeq))
		return XML_TRUE;
	if (state.appHandlers.pUnknownCommandHandler &&
		(*state.appHandlers.pUnknownCommandHandler)(state.appHandlers.pAppData,pUUID,pSeq,pAddress,pCommand,ppProperties,timestamp))
		return XML_TRUE;
	sendResponse(NULL,pSeq,M2MXML_RESULTCODE_UNKNOWNCOMMAND,pCommand,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

static int requestPerceptHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,time_t timestamp) {
	int count;
	int resultCode;
	const char *pError;
	const char *pOptionalMessage;
	M2MBSM_TRANSDUCER *pTransducer;
	
	checkUUID(pUUID);
	if (pAddress != NULL) {
		sendSinglePercept(pSeq,pAddress,XML_TRUE,M2MXML_ENTRYTYPEID_REQUESTED,timestamp);
		return XML_TRUE;
	}
	if (!prepareMessage(pSeq))
		return XML_TRUE;
	pError = NULL;
	pOptionalMessage = NULL;
	resultCode = M2MXML_RESULTCODE_SUCCESS;
	pTransducer = state.pDevice->pTransducers;
	if (pTransducer == NULL || state.appHandlers.pRequestPerceptHandler == NULL) {
		resultCode = M2MXML_RESULTCODE_FAILEDEXECUTION;
		pOptionalMessage = ERROR_UNHANDLEDCOMMAND;
		timestamp = M2MXML_NOTIMESTAMP;
	} else {
		for (count = state.pDevice->transducerCount; pError == NULL && count-- > 0; pTransducer++)
			if (pTransducer->inout & M2MBSM_DIRECTION_IN) {
				if ((resultCode = (*state.appHandlers.pRequestPerceptHandler)(state.pDevice,pTransducer,&timestamp)) != M2MXML_RESULTCODE_SUCCESS) {
					pOptionalMessage = pTransducer->pAddress;
					timestamp = M2MXML_NOTIMESTAMP;
					break;
				}
				pError = M2MXML_createAnyPercept(state.pFormatBuffer,pSeq,pTransducer->pAddress,pTransducer->currentValue.pData,getPerceptType(pTransducer->type,XML_TRUE),M2MXML_ENTRYTYPEID_REQUESTED,timestamp);
			}
	}
	sendResponse(pError,pSeq,resultCode,pOptionalMessage,timestamp);
	return XML_TRUE;
}

static int turnOnOffHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,int sense,const char **ppProperties) {
	int resultCode;
	M2MBSM_TRANSDUCER *pTransducer;
	
	checkUUID(pUUID);
	if ((pTransducer = prepareSetTransducerMessage(pSeq,pAddress,M2MXML_PERCEPTTYPEID_DIGITAL,(void*)(state.appHandlers.pTurnOnOffHandler))) == NULL)
		return XML_TRUE;
	resultCode = (*state.appHandlers.pTurnOnOffHandler)(state.pDevice,pTransducer,sense,ppProperties);
	sendResponse(NULL,pSeq,resultCode,resultCode == M2MXML_RESULTCODE_SUCCESS ? NULL : pAddress,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

static int setStringOutputHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char *pValue) {
	int resultCode;
	M2MBSM_TRANSDUCER *pTransducer;
	
	checkUUID(pUUID);
	if ((pTransducer = prepareSetTransducerMessage(pSeq,pAddress,M2MXML_PERCEPTTYPEID_STRING,(void*)(state.appHandlers.pSetStringOutputHandler))) == NULL)
		return XML_TRUE;
	resultCode = (*state.appHandlers.pSetStringOutputHandler)(state.pDevice,pTransducer,pValue);
	sendResponse(NULL,pSeq,resultCode,resultCode == M2MXML_RESULTCODE_SUCCESS ? NULL : pAddress,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

static int setAnalogOutputHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,double setPoint) {
	int resultCode;
	M2MBSM_TRANSDUCER *pTransducer;
	
	checkUUID(pUUID);
	if ((pTransducer = prepareSetTransducerMessage(pSeq,pAddress,M2MXML_PERCEPTTYPEID_ANALOG,(void*)(state.appHandlers.pSetAnalogOutputHandler))) == NULL)
		return XML_TRUE;
	resultCode = (*state.appHandlers.pSetAnalogOutputHandler)(state.pDevice,pTransducer,setPoint);
	sendResponse(NULL,pSeq,resultCode,resultCode == M2MXML_RESULTCODE_SUCCESS ? NULL : pAddress,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

static int rebootHandler(void *pAppData,const char *pUUID,const char *pSeq) {
	int resultCode;
	
	checkUUID(pUUID);
	if (!prepareMessage(pSeq))
		return XML_TRUE;
	if (state.appHandlers.pRebootHandler == NULL) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_FAILEDEXECUTION,ERROR_UNHANDLEDCOMMAND,M2MXML_NOTIMESTAMP);
		return XML_TRUE;
	}
	resultCode = (*state.appHandlers.pRebootHandler)(state.pDevice);
	sendResponse(NULL,pSeq,resultCode,NULL,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

static int queryConfigurationHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char **ppProperties) {
	int index;
	int resultCode;
	int propertyCount;
	int allProperties;
	const char *pError;
	const char *pOptionalMessage;
	M2MBSM_TRANSDUCER *pTransducer;
	M2MBSM_PROPERTY *pProperty;
	
	checkUUID(pUUID);
	if (!prepareMessage(pSeq))
		return XML_TRUE;
	if (state.appHandlers.pQueryConfigurationHandler == NULL) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_FAILEDEXECUTION,ERROR_UNHANDLEDCOMMAND,M2MXML_NOTIMESTAMP);
		return XML_TRUE;
	}
	pTransducer = NULL;
	if (pAddress == NULL) {
		if (M2MXML_getPropertyString(ppProperties,M2MBSM_PROP_SETUPDEVICE,NULL) != NULL)
			M2MBSM_configureBehavior(NULL,NULL,&M2MBSM_setupDeviceBehaviorHandler);
	} else if ((index = getTargetTransducerIndex(pAddress)) >= 0) {
		pTransducer = state.pDevice->pTransducers + index;
	} else {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_BADARGUMENT,pAddress,M2MXML_NOTIMESTAMP);
		return XML_TRUE;
	}
	propertyCount = pTransducer ? pTransducer->propertyCount : state.pDevice->propertyCount;
	pProperty = pTransducer ? pTransducer->pProperties : state.pDevice->pProperties;
	if ((pError = M2MXML_createPropertySet(state.pFormatBuffer,propertyCount)) != NULL) {
		logError(-1,pError);
		return XML_TRUE;
	}
	pOptionalMessage = NULL;
	resultCode = M2MXML_RESULTCODE_SUCCESS;
	allProperties = ppProperties == NULL || *ppProperties == NULL;
	for (; pError == NULL && propertyCount-- > 0; pProperty++)
		if ((allProperties && pProperty->show) || M2MXML_getPropertyString(ppProperties,pProperty->pKey,NULL) != NULL) {
			if ((resultCode = (*state.appHandlers.pQueryConfigurationHandler)(state.pDevice,pTransducer,pProperty)) != M2MXML_RESULTCODE_SUCCESS) {
				pOptionalMessage = pProperty->pKey;
				break;
			}
			pError = M2MXML_createProperty(state.pFormatBuffer,pProperty->pKey,pProperty->value.pData);
		}
	sendResponse(pError,pSeq,resultCode,pOptionalMessage,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

static int setConfigurationHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char **ppProperties) {
	int index;
	int resultCode;
	int propertyCount;
	const char *pOptionalMessage;
	const char *pValue;
	M2MBSM_TRANSDUCER *pTransducer;
	M2MBSM_PROPERTY *pProperty;
	
	checkUUID(pUUID);
	if (!prepareMessage(pSeq))
		return XML_TRUE;
	if (state.appHandlers.pSetConfigurationHandler == NULL) {
		sendResponse(NULL,pSeq,M2MXML_RESULTCODE_FAILEDEXECUTION,ERROR_UNHANDLEDCOMMAND,M2MXML_NOTIMESTAMP);
		return XML_TRUE;
	}
	pTransducer = NULL;
	if (pAddress != NULL) {
		if ((index = getTargetTransducerIndex(pAddress)) >= 0)
			pTransducer = state.pDevice->pTransducers + index;
		else {
			sendResponse(NULL,pSeq,M2MXML_RESULTCODE_BADARGUMENT,pAddress,M2MXML_NOTIMESTAMP);
			return XML_TRUE;
		}
	}
	pOptionalMessage = NULL;
	resultCode = M2MXML_RESULTCODE_SUCCESS;
	propertyCount = pTransducer ? pTransducer->propertyCount : state.pDevice->propertyCount;
	pProperty = pTransducer ? pTransducer->pProperties : state.pDevice->pProperties;
	for (; propertyCount-- > 0; pProperty++)
		if ((pValue = M2MXML_getPropertyString(ppProperties,pProperty->pKey,NULL)) != NULL) {
			pProperty->show = XML_TRUE;
			strncpy(pProperty->value.pData,pValue,pProperty->value.size - 1);
			pProperty->value.pData[pProperty->value.size - 1] = '\0';
			if (!configureStandardBehavior(pTransducer,pProperty) &&
				(resultCode = (*state.appHandlers.pSetConfigurationHandler)(state.pDevice,pTransducer,pProperty)) != M2MXML_RESULTCODE_SUCCESS) {
				pOptionalMessage = pProperty->pKey;
				break;
			}
		}
	sendResponse(NULL,pSeq,resultCode,pOptionalMessage,M2MXML_NOTIMESTAMP);
	return XML_TRUE;
}

/* STANDARD BEHAVIOR HELPER FUNCTIONS */

static int reportAbsoluteBehavior(M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp,M2MBSM_PROPERTY *pRunProperty,M2MBSM_PROPERTY *pTimeProperty) {
	int time;
	
	if (pRunProperty == NULL || pTimeProperty == NULL ||
		!pRunProperty->show || !pTimeProperty->show ||
		!atoi(pRunProperty->value.pData))
		return XML_FALSE;
	time = atoi(pTimeProperty->value.pData);
	if (event == M2MBSM_BEHAVIOREVENTID_CONFIGURE)
		pBehavior->nextFiring = M2MBSM_nextStartTime(time,XML_FALSE);
	else if (event == M2MBSM_BEHAVIOREVENTID_EXECUTE) {
		requestPerceptHandler(NULL,state.pDevice->pUUID,NULL,pBehavior->pTransducer != NULL ? pBehavior->pTransducer->pAddress : NULL,timestamp);
		pBehavior->nextFiring = M2MBSM_nextStartTime(time,XML_TRUE);
	}
	return XML_TRUE;
}

static int reportIntervalBehavior(M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp,M2MBSM_PROPERTY *pRunProperty,M2MBSM_PROPERTY *pStartProperty,M2MBSM_PROPERTY *pIntervalProperty) {
	int start;
	int interval;
	time_t now;
	
	if (pRunProperty == NULL || pStartProperty == NULL || pIntervalProperty == NULL ||
		!pRunProperty->show || !pStartProperty->show || !pIntervalProperty->show ||
		!atoi(pRunProperty->value.pData))
		return XML_FALSE;
	start = atoi(pStartProperty->value.pData);
	if ((interval = atoi(pIntervalProperty->value.pData)) < 60)
		m2mitoa(interval = 60,pIntervalProperty->value.pData,pIntervalProperty->value.size);
	if (event == M2MBSM_BEHAVIOREVENTID_CONFIGURE)
		pBehavior->nextFiring = M2MBSM_nextStartTime(start,XML_FALSE);
	else if (event == M2MBSM_BEHAVIOREVENTID_EXECUTE) {
		requestPerceptHandler(NULL,state.pDevice->pUUID,NULL,pBehavior->pTransducer != NULL ? pBehavior->pTransducer->pAddress : NULL,timestamp);
		now = time(NULL);
		while ((pBehavior->nextFiring += interval) < now)
			;
	}
	return XML_TRUE;
}

static int timerOnOffBehavior(M2MBSM_BEHAVIOR *pBehavior,int event,int sense,const char *pRunKey,const char *pTimeKey,int *pTimeValue) {
	int propertyCount;
	M2MBSM_PROPERTY *pProperties;
	M2MBSM_PROPERTY *pRunProperty;
	M2MBSM_PROPERTY *pTimeProperty;
	M2MBSM_TRANSDUCER *pTransducer;
	
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return
			strcmp((const char *)pBehavior,pRunKey) == 0 ||
			strcmp((const char *)pBehavior,pTimeKey) == 0;
	if ((pTransducer = pBehavior->pTransducer) == NULL || pTransducer->type != M2MXML_PERCEPTTYPEID_DIGITAL || state.appHandlers.pTurnOnOffHandler == NULL)
		return XML_FALSE;
	propertyCount = pTransducer->propertyCount;
	pProperties = pTransducer->pProperties;
	if ((pRunProperty = M2MBSM_findProperty(pProperties,propertyCount,pRunKey)) == NULL ||
		(pTimeProperty = M2MBSM_findProperty(pProperties,propertyCount,pTimeKey)) == NULL ||
		!pRunProperty->show ||
		!pTimeProperty->show ||
		!atoi(pRunProperty->value.pData))
		return XML_FALSE;
	*pTimeValue = atoi(pTimeProperty->value.pData);
	if (event == M2MBSM_BEHAVIOREVENTID_EXECUTE)
		turnOnOffHandler(NULL,state.pDevice->pUUID,NULL,pBehavior->pTransducer->pAddress,sense,NULL);
	return XML_TRUE;
}

static int timerOnOffAbsoluteBehavior(M2MBSM_BEHAVIOR *pBehavior,int event,int sense,const char *pRunKey,const char *pTimeKey) {
	int time;
	
	time = 0;
	if (!timerOnOffBehavior(pBehavior,event,sense,pRunKey,pTimeKey,&time))
		return XML_FALSE;
	switch (event) {
	case M2MBSM_BEHAVIOREVENTID_CONFIGURE:
		pBehavior->nextFiring = M2MBSM_nextStartTime(time,XML_FALSE);
		break;
	case M2MBSM_BEHAVIOREVENTID_EXECUTE:
		pBehavior->nextFiring = M2MBSM_nextStartTime(time,XML_TRUE);
		break;		
	}
	return XML_TRUE;
}

static int timerOnOffIntervalBehavior(M2MBSM_BEHAVIOR *pBehavior,int event,int sense,const char *pRunKey,const char *pIntervalKey) {
	int interval;
	time_t now;
	
	interval = 0;
	if (!timerOnOffBehavior(pBehavior,event,sense,pRunKey,pIntervalKey,&interval))
		return XML_FALSE;
	now = time(NULL);
	switch (event) {
	case M2MBSM_BEHAVIOREVENTID_CONFIGURE:
		pBehavior->nextFiring = now + interval;
		break;
	case M2MBSM_BEHAVIOREVENTID_EXECUTE:
		if (interval <= 0)
			return XML_FALSE;
		while ((pBehavior->nextFiring += interval) < now)
			;
		break;		
	}
	return XML_TRUE;
}

static int valueCrossesBoundary(double lastValue,double currentValue,M2MBSM_PROPERTY *pLowProperty,M2MBSM_PROPERTY *pHighProperty) {
	double boundary;
	
	if (pLowProperty != NULL && pLowProperty->show) {
		boundary = atof(pLowProperty->value.pData);
		if (lastValue >= boundary && boundary > currentValue)
			return XML_TRUE;
	}
	if (pHighProperty != NULL && pHighProperty->show) {
		boundary = atof(pHighProperty->value.pData);
		if (lastValue <= boundary && boundary < currentValue)
			return XML_TRUE;
	}
	return XML_FALSE;
}

/* STANDARD BEHAVIOR EVENT HANDLERS */

int M2MBSM_reportAllAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTALLABSRUN) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTALLABSTIME) == 0;
	return reportAbsoluteBehavior(pBehavior,event,timestamp,
		M2MBSM_findProperty(state.pDevice->pProperties,state.pDevice->propertyCount,M2MBSM_PROP_RPTALLABSRUN),
		M2MBSM_findProperty(state.pDevice->pProperties,state.pDevice->propertyCount,M2MBSM_PROP_RPTALLABSTIME));
}

int M2MBSM_reportPointAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	int propertyCount;
	M2MBSM_PROPERTY *pProperties;
	
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTPTABSRUN) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTPTABSTIME) == 0;
	if (pBehavior->pTransducer == NULL)
		return XML_FALSE;
	propertyCount = pBehavior->pTransducer->propertyCount;
	pProperties = pBehavior->pTransducer->pProperties;
	return reportAbsoluteBehavior(pBehavior,event,timestamp,
		M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTPTABSRUN),
		M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTPTABSTIME));
}

int M2MBSM_reportAllIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTALLINTRUN) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTALLINTSTART) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTALLINTTIME) == 0;
	return reportIntervalBehavior(pBehavior,event,timestamp,
		M2MBSM_findProperty(state.pDevice->pProperties,state.pDevice->propertyCount,M2MBSM_PROP_RPTALLINTRUN),
		M2MBSM_findProperty(state.pDevice->pProperties,state.pDevice->propertyCount,M2MBSM_PROP_RPTALLINTSTART),
		M2MBSM_findProperty(state.pDevice->pProperties,state.pDevice->propertyCount,M2MBSM_PROP_RPTALLINTTIME));
}

int M2MBSM_reportPointIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	int propertyCount;
	M2MBSM_PROPERTY *pProperties;
	
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTPTINTRUN) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTPTINTSTART) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTPTINTTIME) == 0;
	if (pBehavior->pTransducer == NULL)
		return XML_FALSE;
	propertyCount = pBehavior->pTransducer->propertyCount;
	pProperties = pBehavior->pTransducer->pProperties;
	return reportIntervalBehavior(pBehavior,event,timestamp,
		M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTPTINTRUN),
		M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTPTINTSTART),
		M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTPTINTTIME));
}

int M2MBSM_reportDigitalTransitionBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	int levelChange;
	int lastValue;
	int currentValue;
	M2MBSM_TRANSDUCER *pTransducer;
	M2MBSM_PROPERTY *pLevelChangeProperty;
	
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return strcmp((const char *)pBehavior,M2MBSM_PROP_RPTLEVELCHANGE) == 0;
	if ((pTransducer = pBehavior->pTransducer) == NULL || pTransducer->type != M2MXML_PERCEPTTYPEID_DIGITAL || state.appHandlers.pRequestPerceptHandler == NULL)
		return XML_FALSE;
	if ((pLevelChangeProperty = M2MBSM_findProperty(pBehavior->pTransducer->pProperties,pBehavior->pTransducer->propertyCount,M2MBSM_PROP_RPTLEVELCHANGE)) != NULL && !pLevelChangeProperty->show)
		return XML_FALSE;
	levelChange = atoi(pLevelChangeProperty->value.pData);
	if (event == M2MBSM_BEHAVIOREVENTID_CONFIGURE)
		return levelChange != M2MBSM_LEVELCHANGE_OFF;
	if (event == M2MBSM_BEHAVIOREVENTID_EXECUTE &&
		(*state.appHandlers.pRequestPerceptHandler)(pDevice,pTransducer,&timestamp) == M2MXML_RESULTCODE_SUCCESS) {
		lastValue = atoi(pTransducer->lastValue.pData);
		currentValue = atoi(pTransducer->currentValue.pData);
		if (lastValue != currentValue && (levelChange == M2MBSM_LEVELCHANGE_LOW2HIGH ? currentValue : lastValue)) {
			strcpy(pTransducer->lastValue.pData,pTransducer->currentValue.pData);
			sendSinglePercept(NULL,pTransducer->pAddress,XML_FALSE,M2MXML_ENTRYTYPEID_BYEXCEPTION,timestamp);
		}
	}
	return XML_TRUE;
}

int M2MBSM_reportOutOfBoundsBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	double lastValue;
	double currentValue;
	int propertyCount;
	M2MBSM_PROPERTY *pProperties;
	M2MBSM_PROPERTY *pLowProperty;
	M2MBSM_PROPERTY *pLowLowProperty;
	M2MBSM_PROPERTY *pHighProperty;
	M2MBSM_PROPERTY *pHighHighProperty;
	M2MBSM_TRANSDUCER *pTransducer;
	
	if (event == M2MBSM_BEHAVIOREVENTID_MATCH)
		return
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTLOW) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTHIGH) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTLOWLOW) == 0 ||
			strcmp((const char *)pBehavior,M2MBSM_PROP_RPTHIGHHIGH) == 0;
	if ((pTransducer = pBehavior->pTransducer) == NULL || pTransducer->type != M2MXML_PERCEPTTYPEID_ANALOG || state.appHandlers.pRequestPerceptHandler == NULL)
		return XML_FALSE;
	propertyCount = pBehavior->pTransducer->propertyCount;
	pProperties = pBehavior->pTransducer->pProperties;
	if ((pLowProperty = M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTLOW)) == NULL ||
		(pLowLowProperty = M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTLOWLOW)) == NULL ||
		(pHighProperty = M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTHIGH)) == NULL ||
		(pHighHighProperty = M2MBSM_findProperty(pProperties,propertyCount,M2MBSM_PROP_RPTHIGHHIGH)) == NULL ||
		!pLowProperty->show ||
		!pLowLowProperty->show ||
		!pHighProperty->show ||
		!pHighHighProperty->show)
		return XML_FALSE;
	if (event == M2MBSM_BEHAVIOREVENTID_EXECUTE &&
		(*state.appHandlers.pRequestPerceptHandler)(pDevice,pTransducer,&timestamp) == M2MXML_RESULTCODE_SUCCESS) {
		lastValue = atof(pTransducer->lastValue.pData);
		currentValue = atof(pTransducer->currentValue.pData);
		if (valueCrossesBoundary(lastValue,currentValue,pLowProperty,pHighProperty) ||
			valueCrossesBoundary(lastValue,currentValue,pLowLowProperty,pHighHighProperty)) {
			strcpy(pTransducer->lastValue.pData,pTransducer->currentValue.pData);
			sendSinglePercept(NULL,pTransducer->pAddress,XML_FALSE,M2MXML_ENTRYTYPEID_BYEXCEPTION,timestamp);
		}
	}
	return XML_TRUE;
}

int M2MBSM_setupDeviceBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	M2MMEM_BUFFER buffer;
	int nextIndex;
	int transducerCount;
	int propertyCount;
	const char **ppProperties;
	const char *pError;
	M2MBSM_TRANSDUCER *pTransducer;
	M2MBSM_PROPERTY *pProperty;
	
	if (event == M2MBSM_BEHAVIOREVENTID_CONFIGURE)
		return XML_TRUE;
	if (event != M2MBSM_BEHAVIOREVENTID_EXECUTE || !prepareMessage(NULL))
		return XML_FALSE;
	transducerCount = state.pDevice->transducerCount;
	pTransducer = state.pDevice->pTransducers;
	while (transducerCount-- > 0) {
		M2MMEM_initializeBuffer(&buffer,state.pPropertyBuffer,state.propertySize);
		propertyCount = pTransducer->propertyCount;
		if ((ppProperties = M2MMEM_allocatePropertySet(&buffer,propertyCount + 3)) == NULL) {
			logError(-1,ERROR_CREATEPROPERTYFAILURE);
			return XML_FALSE;
		}
/* HACK: 'nullDefault' is set to XML_FALSE because the portal doesn't yet properly handle a default type property */
		nextIndex = M2MMEM_allocateProperty(&buffer,ppProperties,M2MXML_ATTRIBUTE_TYPE,getPerceptProperty(pTransducer->type,pTransducer->inout,XML_FALSE),0);
		nextIndex = M2MMEM_allocateProperty(&buffer,ppProperties,M2MXML_ATTRIBUTE_LABEL,pTransducer->pLabel,nextIndex);
		nextIndex = M2MMEM_allocateProperty(&buffer,ppProperties,M2MXML_ATTRIBUTE_UNITS,pTransducer->pUnits,nextIndex);
		for (pProperty = pTransducer->pProperties; propertyCount-- > 0; pProperty++)
			if (pProperty->show)
				nextIndex = M2MMEM_allocateProperty(&buffer,ppProperties,pProperty->pKey,pProperty->value.pData,nextIndex);
		if ((pError = nextIndex < 0 ? ERROR_CREATEPROPERTYFAILURE : M2MXML_createCommand(state.pFormatBuffer,NULL,pTransducer->pAddress,M2MXML_COMMAND_SETUPTRANSDUCER,ppProperties,XML_TRUE,M2MXML_NOTIMESTAMP)) != NULL) {
			logError(-1,pError);
			return XML_FALSE;
		}
		pTransducer++;
	}
	sendMessage();
	return XML_FALSE;
}

int M2MBSM_timerOffAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	return timerOnOffAbsoluteBehavior(pBehavior,event,0,M2MBSM_PROP_TIMERLOWABSRUN,M2MBSM_PROP_TIMERLOWABSTIME);
}

int M2MBSM_timerOnAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	return timerOnOffAbsoluteBehavior(pBehavior,event,1,M2MBSM_PROP_TIMERHIGHABSRUN,M2MBSM_PROP_TIMERHIGHABSTIME);
}

int M2MBSM_timerOffIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	return timerOnOffIntervalBehavior(pBehavior,event,0,M2MBSM_PROP_TIMERLOWINTRUN,M2MBSM_PROP_TIMERLOWINTTIME);
}

int M2MBSM_timerOnIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp) {
	return timerOnOffIntervalBehavior(pBehavior,event,1,M2MBSM_PROP_TIMERHIGHINTRUN,M2MBSM_PROP_TIMERHIGHINTTIME);
}

/* EXTERNAL API */

int M2MBSM_configureBehavior(M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty,M2MBSM_HANDLER_BEHAVIOR pBehaviorHandler) {
	M2MBSM_BEHAVIOR *pBehavior;
	
	if (pProperty != NULL && !(*pBehaviorHandler)(state.pDevice,(M2MBSM_BEHAVIOR *)pProperty->pKey,M2MBSM_BEHAVIOREVENTID_MATCH,M2MXML_NOTIMESTAMP))
		return XML_FALSE;
	if ((pBehavior = includeBehavior(pTransducer,pBehaviorHandler)) == NULL)
		return XML_FALSE;
	if ((*pBehaviorHandler)(state.pDevice,pBehavior,M2MBSM_BEHAVIOREVENTID_CONFIGURE,M2MXML_NOTIMESTAMP))
		return XML_TRUE;
	M2MBSM_excludeBehavior(pBehavior);
	return XML_FALSE;
}

int M2MBSM_countBehaviors() {
	int result;
	M2MBSM_BEHAVIOR *pCurrent;
	
	if ((pCurrent = state.pNextBehavior) == NULL)
		return 0;
	result = 0;
	do {
		pCurrent = pCurrent->pNext;
		result++;
	} while (pCurrent != state.pNextBehavior);
	return result;
}

M2MBSM_PROPERTY *M2MBSM_findProperty(M2MBSM_PROPERTY *pProperties,int propertyCount,const char *pKey) {
	for (; propertyCount-- > 0; pProperties++)
		if (strcmp(pKey,pProperties->pKey) == 0)
			return pProperties;
	return NULL;
}

M2MBSM_HANDLERS *M2MBSM_initializeState(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehaviorBuffer,int maxBehaviors,int maxProperties,char *pPropertyBuffer,int propertySize,char *pParserBuffer,int parserSize,char *pFormatBuffer,int formatSize) {
	memset(&state,0,sizeof(state));
	state.pDevice = pDevice;
	state.baseHandlers.pAppData = &state;
	state.baseHandlers.pLogErrorHandler = &logErrorHandler;
	state.baseHandlers.pUnknownCommandHandler = &unknownCommandHandler;
	state.baseHandlers.pRequestPerceptHandler = &requestPerceptHandler;
	state.baseHandlers.pTurnOnOffHandler = &turnOnOffHandler;
	state.baseHandlers.pSetStringOutputHandler = &setStringOutputHandler;
	state.baseHandlers.pSetAnalogOutputHandler = &setAnalogOutputHandler;
	state.baseHandlers.pRebootHandler = &rebootHandler;
	state.baseHandlers.pQueryConfigurationHandler = &queryConfigurationHandler;
	state.baseHandlers.pSetConfigurationHandler = &setConfigurationHandler;
	state.maxProperties = maxProperties;
	state.pPropertyBuffer = pPropertyBuffer;
	state.propertySize = propertySize;
	state.pParserBuffer = pParserBuffer;
	state.parserSize = parserSize;
	state.pFormatBuffer = pFormatBuffer;
	state.formatSize = formatSize;
	
	while (maxBehaviors-- > 0) {
		pBehaviorBuffer->pNext = state.pFreeBehavior;
		state.pFreeBehavior = pBehaviorBuffer++;
	}
	return &state.appHandlers;
}

time_t M2MBSM_nextStartTime(int hhmmss,int mustBeFuture) {
	struct tm tmNow;
	time_t now;
	time_t result;
	
	now = time(NULL);
	tmNow = *localtime(&now);
	result = now + hhmmss - (tmNow.tm_hour * 60 * 60) - (tmNow.tm_min * 60) - (tmNow.tm_sec);
	return result < now && mustBeFuture ? result + 24 * 60 * 60 : result;
}

void M2MBSM_parseMessage(const char *pMessageBuffer,int messageSize) {
   	M2MXML_parseMessage(&state.baseHandlers,pMessageBuffer,messageSize,state.maxProperties,state.pParserBuffer,state.parserSize);
}

time_t M2MBSM_processState() {
	M2MBSM_BEHAVIOR *pCurrent;
	M2MBSM_BEHAVIOR *pNext;
	time_t result;
	
	if ((pCurrent = state.pNextBehavior) == NULL)
		return -1;
	state.processTime = time(NULL);
	do {
		if (pCurrent->nextFiring < state.processTime)
			break;
		pCurrent = pCurrent->pNext;
	} while (pCurrent != state.pNextBehavior);
	if (pCurrent->nextFiring < state.processTime) {
		state.pNextBehavior = pCurrent->pNext;
		if (!(*pCurrent->pHandler)(state.pDevice,pCurrent,M2MBSM_BEHAVIOREVENTID_EXECUTE,state.processTime))
			M2MBSM_excludeBehavior(pCurrent);
	}
	pNext = state.pNextBehavior;
	if ((pCurrent = state.pNextBehavior) == NULL)
		return -1;
	do {
		if (pCurrent->nextFiring < pNext->nextFiring)
			pNext = pCurrent;
		pCurrent = pCurrent->pNext;
	} while (pCurrent != state.pNextBehavior);
	state.pNextBehavior = pCurrent;
	result = pCurrent->nextFiring - state.processTime;
	return result >= 0 ? result : 0;
}
