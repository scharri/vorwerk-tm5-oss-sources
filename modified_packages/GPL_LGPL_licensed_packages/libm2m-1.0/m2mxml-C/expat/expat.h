/*** BeginHeader
		XML_TRUE,
		XML_FALSE,
		XML_STATUS_OK,
		XML_STATUS_ERROR,
		XML_Parser,
		XML_ErrorString,
		XML_GetErrorCode,
		XML_ParserCreate,
		XML_ParserFree,
		XML_SetUserData,
		XML_SetElementHandler
 */
 
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define XMLCALL

#define XML_TRUE			1
#define XML_FALSE			0
#define XML_STATUS_OK		0
#define XML_STATUS_ERROR	(-1)

typedef void *XML_Parser;

typedef void (*XML_STARTELEMENT)(void *pData,const char *pElementName,const char **ppAttributes);
typedef void (*XML_STOPELEMENT)(void *pData,const char *pElementName);

const char *XML_ErrorString(int code);
int XML_GetErrorCode(XML_Parser parser);
int XML_Parse(XML_Parser parser,const char *pMessageBuffer,int messageSize,int flags);
XML_Parser XML_ParserCreate(void *pUnused);
void XML_ParserFree(XML_Parser parser);
void XML_SetUserData(XML_Parser parser,void *pAppData);
void XML_SetElementHandler(XML_Parser parser,XML_STARTELEMENT pStartElement,XML_STOPELEMENT pStopElement);

/*** EndHeader */
