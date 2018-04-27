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

#include <string.h>
#include <stdlib.h>
#include "m2mmem.h"
#include "m2mxml.h"

/* ERROR MESSAGES */

#define ERROR_INVALIDCOMMAND	"Invalid command"
#define ERROR_INVALIDMESSAGE	"Invalid message"
#define ERROR_INVALIDPROPERTY	"Invalid property"
#define ERROR_NOANALOGDATA		"'setAnalogOutput' setPoint not found"
#define ERROR_NOSTRINGDATA		"'setStringOutput' data not found"
#define ERROR_NOTENOUGHSPACE	"Not enough M2MXML buffer space; increase and recompile"
#define ERROR_TOOMANYPROPERTIES	"Too many properties"
#define ERROR_TOOMANYCOMMANDS	"A command has already been found"
#define ERROR_UNEXPECTEDCOMMAND "Unexpected command ID"
#define ERROR_UNEXPECTEDDEPTH	"Unexpected depth"
#define ERROR_UNHANDLEDCOMMAND	"Unhandled command ID"

/* STATE DEFINITION */

typedef struct {
	XML_Parser parser;
	M2MXML_HANDLERS *pHandlers;
	const char * pUUID;
	const char * pCommandName;
	const char * pAddressName;
	const char * pSeq;
	int isValid;
	int requirePropertyValue;
	int commandID;
	int depth;
	int propertyCount;
	const char **ppProperties;
	int maxProperties;
	time_t parserTime;
	M2MMEM_BUFFER buffer;
} M2MXML_STATE;

/* GENERAL HELPER FUNCTIONS */

static void logError(M2MXML_STATE *pState,int code,const char *pDescription) {
	if (pState->pHandlers->pLogErrorHandler)
		(*pState->pHandlers->pLogErrorHandler)(pState->pHandlers->pAppData,code,pDescription);
}

static void logParserError(M2MXML_STATE *pState) {
	int code = XML_GetErrorCode(pState->parser);
	logError(pState,code,(const char *)XML_ErrorString(code));
}

/* ELEMENT VALIDATION FUNCTIONS */

static int validateMessage(M2MXML_STATE *pState,const char *pElementName,const char **ppAttributes) {
	if (strcmp(pElementName,M2MXML_ELEMENT_MESSAGE) != 0 ||
		strcmp(M2MXML_getPropertyString(ppAttributes,M2MXML_ATTRIBUTE_VERSION,""),M2MXML_VERSION) != 0) {
		logError(pState,-1,ERROR_INVALIDMESSAGE);
		return XML_FALSE;
	}
	return XML_TRUE;
}

static int validateCommand(M2MXML_STATE *pState,const char *pElementName,const char **ppAttributes) {
	if (strcmp(pElementName,M2MXML_ELEMENT_COMMAND) != 0 ||
		(pState->pCommandName = M2MXML_getPropertyString(ppAttributes,M2MXML_ATTRIBUTE_NAME,NULL)) == NULL ||
		(pState->pSeq = M2MMEM_allocateString(&pState->buffer,M2MXML_getPropertyString(ppAttributes,M2MXML_ATTRIBUTE_SEQUENCENUMBER,NULL))) == NULL) {
		logError(pState,-1,ERROR_INVALIDCOMMAND);
		return XML_FALSE;
	}
	pState->pAddressName = M2MMEM_allocateString(&pState->buffer,M2MXML_getPropertyString(ppAttributes,M2MXML_ATTRIBUTE_ADDRESS,NULL));
	pState->requirePropertyValue = XML_TRUE;
	if (strcmp(pState->pCommandName,M2MXML_COMMAND_REQUESTPERCEPT) == 0) {
		pState->commandID = M2MXML_COMMANDID_REQUESTPERCEPT;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_TURNON) == 0) {
		pState->commandID = M2MXML_COMMANDID_TURNON;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_TURNOFF) == 0) {
		pState->commandID = M2MXML_COMMANDID_TURNOFF;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_SETSTRINGOUTPUT) == 0) {
		pState->commandID = M2MXML_COMMANDID_SETSTRINGOUTPUT;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_SETANALOGOUTPUT) == 0) {
		pState->commandID = M2MXML_COMMANDID_SETANALOGOUTPUT;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_REBOOT) == 0) {
		pState->commandID = M2MXML_COMMANDID_REBOOT;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_QUERYCONFIGURATION) == 0) {
		pState->commandID = M2MXML_COMMANDID_QUERYCONFIGURATION;
		pState->requirePropertyValue = XML_FALSE;
	} else if (strcmp(pState->pCommandName,M2MXML_COMMAND_SETCONFIGURATION) == 0) {
		pState->commandID = M2MXML_COMMANDID_SETCONFIGURATION;
	} else if (pState->commandID != M2MXML_COMMANDID_UNKNOWN) {
		logError(pState,pState->commandID,ERROR_TOOMANYCOMMANDS);
		return XML_FALSE;
	}
	return XML_TRUE;
}

static int validateProperty(M2MXML_STATE *pState,const char *pElementName,const char **ppAttributes) {
	const char *pKey;
	const char *pValue;
	if (strcmp(pElementName,M2MXML_ELEMENT_PROPERTY) != 0 ||
		(pKey = M2MMEM_allocateString(&pState->buffer,M2MXML_getPropertyString(ppAttributes,M2MXML_ATTRIBUTE_NAME,NULL))) == NULL ||
		((pValue = M2MMEM_allocateString(&pState->buffer,M2MXML_getPropertyString(ppAttributes,M2MXML_ATTRIBUTE_VALUE,NULL))) == NULL && pState->requirePropertyValue)) {
		logError(pState,-1,ERROR_INVALIDPROPERTY);
		return XML_FALSE;
	}
	if (pState->propertyCount >= pState->maxProperties) {
		logError(pState,-1,ERROR_TOOMANYPROPERTIES);
		return XML_FALSE;
	}
	pState->ppProperties[pState->propertyCount++] = pKey;
	pState->ppProperties[pState->propertyCount++] = pValue ? pValue : "";
	pState->ppProperties[pState->propertyCount] = NULL;
	return XML_TRUE;
}

/* COMMAND PROCESSING FUNCTIONS */

static int executeCommand(M2MXML_STATE *pState) {
	switch (pState->commandID) {
	case M2MXML_COMMANDID_UNKNOWN:
		if (pState->pHandlers->pUnknownCommandHandler)
			 return (*pState->pHandlers->pUnknownCommandHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,pState->pCommandName,pState->ppProperties,pState->parserTime);
		break;
	case M2MXML_COMMANDID_REQUESTPERCEPT:
		if (pState->pHandlers->pRequestPerceptHandler)
			return (*pState->pHandlers->pRequestPerceptHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,pState->parserTime);
		break;
	case M2MXML_COMMANDID_TURNON:
		if (pState->pHandlers->pTurnOnOffHandler)
			return (*pState->pHandlers->pTurnOnOffHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,1,pState->ppProperties);
		break;
	case M2MXML_COMMANDID_TURNOFF:
		if (pState->pHandlers->pTurnOnOffHandler)
			return (*pState->pHandlers->pTurnOnOffHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,0,pState->ppProperties);
		break;
	case M2MXML_COMMANDID_SETSTRINGOUTPUT:
		if (pState->pHandlers->pSetStringOutputHandler) {
			const char *pString = M2MXML_getPropertyString(pState->ppProperties,M2MXML_PROPERTY_DATA,NULL);
			if (pString == NULL) {
				logError(pState,-1,ERROR_NOSTRINGDATA);
				return XML_FALSE;
			}
			return (*pState->pHandlers->pSetStringOutputHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,pString);
		}
		break;
	case M2MXML_COMMANDID_SETANALOGOUTPUT:
		if (pState->pHandlers->pSetAnalogOutputHandler) {
			const char *pString = M2MXML_getPropertyString(pState->ppProperties,M2MXML_PROPERTY_SETPOINT,NULL);
			if (pString == NULL) {
				logError(pState,-1,ERROR_NOANALOGDATA);
				return XML_FALSE;
			}
			return (*pState->pHandlers->pSetAnalogOutputHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,atof(pString));
		}
		break;
	case M2MXML_COMMANDID_REBOOT:
		if (pState->pHandlers->pRebootHandler)
			return (*pState->pHandlers->pRebootHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq);
		break;
	case M2MXML_COMMANDID_QUERYCONFIGURATION:
		if (pState->pHandlers->pSetConfigurationHandler)
			return (*pState->pHandlers->pQueryConfigurationHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,pState->ppProperties);
		break;
	case M2MXML_COMMANDID_SETCONFIGURATION:
		if (pState->pHandlers->pQueryConfigurationHandler)
			return (*pState->pHandlers->pSetConfigurationHandler)(pState->pHandlers->pAppData,pState->pUUID,pState->pSeq,pState->pAddressName,pState->ppProperties);
		break;
	default:
		logError(pState,pState->commandID,ERROR_UNEXPECTEDCOMMAND);
		return XML_FALSE;
	}
	logError(pState,pState->commandID,ERROR_UNHANDLEDCOMMAND);
	return XML_FALSE;
}

/* EXPAT CALLBACK FUNCTIONS */

static void XMLCALL startElement(void *pData,const char *pElementName,const char **ppAttributes) {
	M2MXML_STATE *pState = (M2MXML_STATE*)pData;
	if (pState->isValid)
		switch (pState->depth) {
		case 0:
			pState->isValid = validateMessage(pState,pElementName,ppAttributes);
			break;
		case 1:
			pState->isValid = validateCommand(pState,pElementName,ppAttributes);
			break;
		case 2:
			pState->isValid = validateProperty(pState,pElementName,ppAttributes);
			break;
		default:
			logError(pState,pState->depth,ERROR_UNEXPECTEDDEPTH);
			pState->isValid = XML_FALSE;
			break;
		}
	pState->depth++;
}

static void XMLCALL stopElement(void *pData,const char *pElementName) {
	M2MXML_STATE *pState = (M2MXML_STATE*)pData;
	pState->depth--;
	if (pState->isValid)
		switch (pState->depth) {
		case 0:
			break;
		case 1:
			pState->isValid = executeCommand(pState);
			break;
		case 2:
			break;
		default:
			logError(pState,pState->depth,ERROR_UNEXPECTEDDEPTH);
			pState->isValid = XML_FALSE;
			break;
		}
}

/* STATE HANDLING FUNCTIONS */

static const char *initializeState(M2MXML_STATE *pState,M2MXML_HANDLERS *pHandlers,int maxProperties,char *pFormatBuffer,int formatSize) {
	memset(pState,0,sizeof(*pState));
	pState->pHandlers = pHandlers;
	pState->isValid = XML_TRUE;
	pState->requirePropertyValue = XML_TRUE;
	pState->commandID = M2MXML_COMMANDID_UNKNOWN;
	pState->parser = XML_ParserCreate(NULL);
	XML_SetUserData(pState->parser,pState);
	XML_SetElementHandler(pState->parser,startElement,stopElement);
	
	M2MMEM_initializeBuffer(&pState->buffer,pFormatBuffer,formatSize);
	if ((pState->ppProperties = (const char **)M2MMEM_allocateSpace(&pState->buffer,sizeof(*pState->ppProperties) * (maxProperties * 2 + 1))) == NULL)
		return ERROR_NOTENOUGHSPACE;
	*pState->ppProperties = NULL;
	pState->maxProperties = maxProperties;
	pState->parserTime = time(NULL);
	return NULL;
}

static void terminateState(M2MXML_STATE *pState) {
	XML_ParserFree(pState->parser);
}

/* EXTERNAL API */

const char *M2MXML_getPropertyString(const char **ppProperties,const char *pName,const char *pDefault) {
	if (ppProperties) {
		while (*ppProperties != NULL)
			if (strcmp(*ppProperties++,pName) == 0)
				return *ppProperties;
			else if (*ppProperties)
				ppProperties++;
	}
	return pDefault;
}

void M2MXML_parseMessage(M2MXML_HANDLERS *pHandlers,const char *pMessageBuffer,int messageSize,int maxProperties,char *pFormatBuffer,int formatSize) {
	M2MXML_STATE state;
	const char *pError = initializeState(&state,pHandlers,maxProperties,pFormatBuffer,formatSize);
	if (pError != NULL)
		logError(&state,-1,pError);
	else if (XML_Parse(state.parser,pMessageBuffer,messageSize,XML_TRUE) == XML_STATUS_ERROR)
		logParserError(&state);
	terminateState(&state);
}
