#include "expat.h"

/* GLOBAL VARIABLES */

char *_ppAttributes[64 + 1];
const char *_pLastError;
void *_pAppData;
XML_STARTELEMENT _startElement;
XML_STOPELEMENT _stopElement;

/* INTERNAL HELPER FUNCTIONS */

char *skipKeyword(char *pMessage) {
	for (; *pMessage; pMessage++)
		if (!isalnum(*pMessage))
			break;
	return pMessage;		
}

/* NOTE: this doesn't handle embedded double quotes */
char *skipValue(char *pMessage) {
	for (; *pMessage; pMessage++)
		if (*pMessage == '"')
			break;
	return pMessage;
}

char *skipWhitespace(char *pMessage) {
	for (; *pMessage; pMessage++)
		if (*pMessage != ' ')
			break;
	return pMessage;
}

int returnLastError(const char *pError) {
	_pLastError = pError;
	return XML_STATUS_ERROR;
}

char peekNextToken(char **ppMessage) {
	char result;
	
	result = **ppMessage;
	**ppMessage = '\0';
	(*ppMessage)++;
	if (result != ' ')
		return result;
	*ppMessage = skipWhitespace(*ppMessage);
	return **ppMessage;
}


int parseElement(char **ppBuffer) {
	char *pElementName;
	char *pAttributeName;
	char *pAttributeValue;
	char nextChar;
	int attributeCount;
	int elementLength;
	
	if ((*ppBuffer)[0] != '<')
		return returnLastError("Begin element expected");
	*ppBuffer = skipKeyword(pElementName = *ppBuffer + 1);
	elementLength = *ppBuffer - pElementName;
	if (elementLength <= 0)
		return returnLastError("Element name expected");
	nextChar = peekNextToken(ppBuffer);
	attributeCount = 0;
	_ppAttributes[attributeCount] = NULL;
	while (attributeCount <= sizeof(_ppAttributes)/sizeof(*_ppAttributes) - 3) {
		if (!isalnum(nextChar))
			break;
		*ppBuffer = skipKeyword(pAttributeName = *ppBuffer);
		if (*ppBuffer == pAttributeName)
			return returnLastError("Attribute name expected");
		if (peekNextToken(ppBuffer) != '=')
			return returnLastError("Attribute = separator expected");
		if (peekNextToken(ppBuffer) != '"')
			return returnLastError("Attribute value expected");
		*ppBuffer = skipValue(pAttributeValue = *ppBuffer);
		if (peekNextToken(ppBuffer) != '"')
			return returnLastError("Attribute value doesn't terminate");
		_ppAttributes[attributeCount++] = pAttributeName;
		_ppAttributes[attributeCount++] = pAttributeValue;
		_ppAttributes[attributeCount] = NULL;
		nextChar = peekNextToken(ppBuffer);
	}
	if (nextChar == '/' && (*ppBuffer)[0] == '>') {
		(*ppBuffer)++;
		(*_startElement)(_pAppData,pElementName,(const char **)_ppAttributes);
		(*_stopElement)(_pAppData,pElementName);
		return XML_STATUS_OK;
	}
	if (nextChar != '>')
		return returnLastError("End of element expected");
	(*_startElement)(_pAppData,pElementName,(const char **)_ppAttributes);
	while ((*ppBuffer)[0] == '<' && (*ppBuffer)[1] != '/')
		if (parseElement(ppBuffer) == XML_STATUS_ERROR)
			return XML_STATUS_ERROR;
	if ((*ppBuffer)[0] != '<' || (*ppBuffer)[1] != '/')
		return returnLastError("End element tag expected");
	if (strncmp(pElementName,*ppBuffer + 2,elementLength) != 0)
		return returnLastError("Mismatching ending element name");
	if ((*ppBuffer)[elementLength + 2] != '>')
		return returnLastError("Incomplete element end tag");
	*ppBuffer += elementLength + 3;
	(*_stopElement)(_pAppData,pElementName);
	return XML_STATUS_OK;
}

/* EXTERNAL API */

/* NOTE: this simple implementation assumes you'll ask for the last error */
const char *XML_ErrorString(int code) {
	return _pLastError;
}

int XML_GetErrorCode(XML_Parser parser) {
	return 0;
}

/* NOTE: this simple implementation requires null-terminated buffers */
int XML_Parse(XML_Parser parser,const char *pBuffer,int bufferSize,int flags) {
	char *pHackedBuffer = (char *)pBuffer;
	_pLastError = NULL;
	return parseElement(&pHackedBuffer);
}

/* NOTE: this simple implementation allows only a single thread */
XML_Parser XML_ParserCreate(void *pUnused) {
	return NULL;
}

void XML_ParserFree(XML_Parser parser) {
}

void XML_SetUserData(XML_Parser parser,void *pAppData) {
	_pAppData = pAppData;
}

void XML_SetElementHandler(XML_Parser parser,XML_STARTELEMENT pStartElement,XML_STOPELEMENT pStopElement) {
	_startElement = pStartElement;
	_stopElement = pStopElement;
}
