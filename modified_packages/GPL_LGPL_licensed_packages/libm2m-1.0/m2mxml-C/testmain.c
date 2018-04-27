#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m2mbsm.h"
#include "m2mitoa.h"

#define BUFFER_SIZE		4096
#define MAX_BEHAVIORS	32
#define MAX_PROPERTIES	32
#define VALUE_SIZE		32

static char uuid[M2MXML_UUIDSIZE + 1];
static char sendBuffer[BUFFER_SIZE];
static char receiveBuffer[BUFFER_SIZE];
static char parserBuffer[BUFFER_SIZE];
static char propertyBuffer[BUFFER_SIZE];
static char formatBuffer[BUFFER_SIZE];

static M2MBSM_BEHAVIOR behaviorBuffer[MAX_BEHAVIORS];

static char deviceKeyValue[VALUE_SIZE] = "value";
static char reportAllAbsoluteRun[VALUE_SIZE] = "0";
static char reportAllAbsoluteTime[VALUE_SIZE] = "0";
static char reportAllIntervalRun[VALUE_SIZE] = "0";
static char reportAllIntervalStart[VALUE_SIZE] = "0";
static char reportAllIntervalTime[VALUE_SIZE] = "0";
static M2MBSM_PROPERTY deviceProperties[] = {
/*	{"key",M2MBSM_DEFINEVALUE(deviceKeyValue),XML_TRUE}, // HACK: the portal is not currently allowing custom properties */
	{M2MBSM_PROP_RPTALLABSRUN,M2MBSM_DEFINEVALUE(reportAllAbsoluteRun),XML_FALSE},
	{M2MBSM_PROP_RPTALLABSTIME,M2MBSM_DEFINEVALUE(reportAllAbsoluteTime),XML_FALSE},
	{M2MBSM_PROP_RPTALLINTRUN,M2MBSM_DEFINEVALUE(reportAllIntervalRun),XML_FALSE},
	{M2MBSM_PROP_RPTALLINTSTART,M2MBSM_DEFINEVALUE(reportAllIntervalStart),XML_FALSE},
	{M2MBSM_PROP_RPTALLINTTIME,M2MBSM_DEFINEVALUE(reportAllIntervalTime),XML_FALSE}
};

static char ain01KeyValue[VALUE_SIZE] = "ain01";
static char analogCalibration[VALUE_SIZE] = "0";
static char reportPointAbsoluteRun[VALUE_SIZE] = "0";
static char reportPointAbsoluteTime[VALUE_SIZE] = "0";
static char reportPointIntervalRun[VALUE_SIZE] = "0";
static char reportPointIntervalStart[VALUE_SIZE] = "0";
static char reportPointIntervalTime[VALUE_SIZE] = "0";
static char reportLow[VALUE_SIZE] = "0";
static char reportHigh[VALUE_SIZE] = "0";
static char reportLowLow[VALUE_SIZE] = "0";
static char reportHighHigh[VALUE_SIZE] = "0";
static M2MBSM_PROPERTY ain01Properties[] = {
/*	{"key",M2MBSM_DEFINEVALUE(ain01KeyValue),XML_TRUE}, // HACK: the portal is not currently allowing custom properties */
	{M2MBSM_PROP_ANALOGCALIBRATION,M2MBSM_DEFINEVALUE(analogCalibration),XML_FALSE},
	{M2MBSM_PROP_RPTPTABSRUN,M2MBSM_DEFINEVALUE(reportPointAbsoluteRun),XML_FALSE},
	{M2MBSM_PROP_RPTPTABSTIME,M2MBSM_DEFINEVALUE(reportPointAbsoluteTime),XML_FALSE},
	{M2MBSM_PROP_RPTPTINTRUN,M2MBSM_DEFINEVALUE(reportPointIntervalRun),XML_FALSE},
	{M2MBSM_PROP_RPTPTINTSTART,M2MBSM_DEFINEVALUE(reportPointIntervalStart),XML_FALSE},
	{M2MBSM_PROP_RPTPTINTTIME,M2MBSM_DEFINEVALUE(reportPointIntervalTime),XML_FALSE},
	{M2MBSM_PROP_RPTLOW,M2MBSM_DEFINEVALUE(reportLow),XML_FALSE},
	{M2MBSM_PROP_RPTHIGH,M2MBSM_DEFINEVALUE(reportHigh),XML_FALSE},
	{M2MBSM_PROP_RPTLOWLOW,M2MBSM_DEFINEVALUE(reportLowLow),XML_FALSE},
	{M2MBSM_PROP_RPTHIGHHIGH,M2MBSM_DEFINEVALUE(reportHighHigh),XML_FALSE}
};

static char reportLevelChange[VALUE_SIZE] = "0";
static M2MBSM_PROPERTY din01Properties[] = {
	{M2MBSM_PROP_RPTLEVELCHANGE,M2MBSM_DEFINEVALUE(reportLevelChange),XML_FALSE}
};

static char pulseLowRun[VALUE_SIZE] = "0";
static char pulseLowTime[VALUE_SIZE] = "0";
static char pulseHighRun[VALUE_SIZE] = "0";
static char pulseHighTime[VALUE_SIZE] = "0";
static char timerLowAbsoluteRun[VALUE_SIZE] = "0";
static char timerLowAbsoluteTime[VALUE_SIZE] = "0";
static char timerHighAbsoluteRun[VALUE_SIZE] = "0";
static char timerHighAbsoluteTime[VALUE_SIZE] = "0";
static char timerLowIntervalRun[VALUE_SIZE] = "0";
static char timerLowIntervalTime[VALUE_SIZE] = "0";
static char timerHighIntervalRun[VALUE_SIZE] = "0";
static char timerHighIntervalTime[VALUE_SIZE] = "0";
static M2MBSM_PROPERTY dout01Properties[] = {
	{M2MBSM_PROP_PULSELOWRUN,M2MBSM_DEFINEVALUE(pulseLowRun),XML_FALSE},
	{M2MBSM_PROP_PULSELOWTIME,M2MBSM_DEFINEVALUE(pulseLowTime),XML_FALSE},
	{M2MBSM_PROP_PULSEHIGHRUN,M2MBSM_DEFINEVALUE(pulseHighRun),XML_FALSE},
	{M2MBSM_PROP_PULSEHIGHTIME,M2MBSM_DEFINEVALUE(pulseHighTime),XML_FALSE},
	{M2MBSM_PROP_TIMERLOWABSRUN,M2MBSM_DEFINEVALUE(timerLowAbsoluteRun),XML_FALSE},
	{M2MBSM_PROP_TIMERLOWABSTIME,M2MBSM_DEFINEVALUE(timerLowAbsoluteTime),XML_FALSE},
	{M2MBSM_PROP_TIMERHIGHABSRUN,M2MBSM_DEFINEVALUE(timerHighAbsoluteRun),XML_FALSE},
	{M2MBSM_PROP_TIMERHIGHABSTIME,M2MBSM_DEFINEVALUE(timerHighAbsoluteTime),XML_FALSE},
	{M2MBSM_PROP_TIMERLOWINTRUN,M2MBSM_DEFINEVALUE(timerLowIntervalRun),XML_FALSE},
	{M2MBSM_PROP_TIMERLOWINTTIME,M2MBSM_DEFINEVALUE(timerLowIntervalTime),XML_FALSE},
	{M2MBSM_PROP_TIMERHIGHINTRUN,M2MBSM_DEFINEVALUE(timerHighIntervalRun),XML_FALSE},
	{M2MBSM_PROP_TIMERHIGHINTTIME,M2MBSM_DEFINEVALUE(timerHighIntervalTime),XML_FALSE}
};

static char ainout01LastValue[VALUE_SIZE];
static char ainout01CurrentValue[VALUE_SIZE] = "1";
static char dinout01LastValue[VALUE_SIZE];
static char dinout01CurrentValue[VALUE_SIZE] = "1";
static char sinout01LastValue[VALUE_SIZE];
static char sinout01CurrentValue[VALUE_SIZE] = "sin01";
static char linout01LastValue[VALUE_SIZE];
static char linout01CurrentValue[VALUE_SIZE] = "12.34,56.78";
static M2MBSM_TRANSDUCER transducers[] = {
	{NULL,M2MXML_PERCEPTTYPEID_ANALOG,M2MBSM_DIRECTION_IN,"ain01","Analog In #1","units",ain01Properties,M2MBSM_ARRAYCOUNT(ain01Properties),M2MBSM_DEFINEVALUE(ainout01LastValue),M2MBSM_DEFINEVALUE(ainout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_ANALOG,M2MBSM_DIRECTION_INOUT,"aout01","Analog Out #1","units",NULL,0,M2MBSM_DEFINEVALUE(ainout01LastValue),M2MBSM_DEFINEVALUE(ainout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_DIGITAL,M2MBSM_DIRECTION_IN,"din01","Digital In #1","units",din01Properties,M2MBSM_ARRAYCOUNT(din01Properties),M2MBSM_DEFINEVALUE(dinout01LastValue),M2MBSM_DEFINEVALUE(dinout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_DIGITAL,M2MBSM_DIRECTION_OUT,"dout01","Digital Out #1","units",dout01Properties,M2MBSM_ARRAYCOUNT(dout01Properties),M2MBSM_DEFINEVALUE(dinout01LastValue),M2MBSM_DEFINEVALUE(dinout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_STRING,M2MBSM_DIRECTION_IN,"sin01","String In #1","units",NULL,0,M2MBSM_DEFINEVALUE(sinout01LastValue),M2MBSM_DEFINEVALUE(sinout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_STRING,M2MBSM_DIRECTION_OUT,"sout01","String Out #1","units",NULL,0,M2MBSM_DEFINEVALUE(sinout01LastValue),M2MBSM_DEFINEVALUE(sinout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_LOCATION,M2MBSM_DIRECTION_IN,"lin01","Location In #1","units",NULL,0,M2MBSM_DEFINEVALUE(linout01LastValue),M2MBSM_DEFINEVALUE(linout01CurrentValue)},
	{NULL,M2MXML_PERCEPTTYPEID_LOCATION,M2MBSM_DIRECTION_OUT,"lout01","Location Out #1","units",NULL,0,M2MBSM_DEFINEVALUE(linout01LastValue),M2MBSM_DEFINEVALUE(linout01CurrentValue)}
};

static M2MBSM_DEVICE device = {NULL,NULL,transducers,M2MBSM_ARRAYCOUNT(transducers),deviceProperties,M2MBSM_ARRAYCOUNT(deviceProperties)};
static int acknowledgement = XML_FALSE;

/* HELPER FUNCTIONS */

static void checkAcknowledgement() {
	if (!acknowledgement) {
		fputs("Acknowledgement expected\n",stderr);
		exit(EXIT_FAILURE);
	}
}

static void processAllBehaviors () {
	int count = M2MBSM_countBehaviors();
	while (count-- > 0)
		M2MBSM_processState();
}

/* M2MXML HANDLER FUNCTIONS */

static int logErrorHandler(void * pAppData,int code,const char *pDescription) {
	fprintf(stderr,"Error %d: %s\n",code,pDescription);
	return XML_TRUE;
}

static int unknownCommandHandler(void *pAppData,const char *pUUID,const char *pSeq,const char *pAddress,const char *pCommand,const char **ppProperties,time_t timestamp) {
	return XML_FALSE;
}

/* M2MBSM HANDLER FUNCTIONS */

static int requestPerceptHandler(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,time_t *pTimestamp) {
	*pTimestamp = M2MXML_NOTIMESTAMP;
	return M2MXML_RESULTCODE_SUCCESS;
}

static int turnOnOffHandler(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,int sense,const char **ppProperties) {
	strcpy(pTransducer->lastValue.pData,pTransducer->currentValue.pData);
	m2mitoa(sense,pTransducer->currentValue.pData,pTransducer->currentValue.size);
	return M2MXML_RESULTCODE_SUCCESS;
}

static int setStringOutputHandler(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,const char *pValue) {
	strcpy(pTransducer->lastValue.pData,pTransducer->currentValue.pData);
	strncpy(pTransducer->currentValue.pData,pValue,pTransducer->currentValue.size - 1);
	pTransducer->currentValue.pData[pTransducer->currentValue.size - 1] = '\0';
	return M2MXML_RESULTCODE_SUCCESS;
}

static int setAnalogOutputHandler(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,double setPoint) {
	strcpy(pTransducer->lastValue.pData,pTransducer->currentValue.pData);
	sprintf(pTransducer->currentValue.pData,"%lf",setPoint);
	return M2MXML_RESULTCODE_SUCCESS;
}

static int rebootHandler(M2MBSM_DEVICE *pDevice) {
	return M2MXML_RESULTCODE_SUCCESS;
}

static int queryConfigurationHandler(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty) {
	return M2MXML_RESULTCODE_SUCCESS;
}

static int setConfigurationHandler(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty) {
	return M2MXML_RESULTCODE_SUCCESS;
}

static void sendMessageHandler(M2MBSM_DEVICE *pDevice,void *pMessageData) {
	int size = M2MXML_formatMessage(pMessageData,sendBuffer,sizeof(sendBuffer) - 2);
	if (size <= 0)
		fputs("No M2MXML message created\n",stderr);
	else {
		sendBuffer[size] = '\n';
		sendBuffer[size + 1] = '\0';
		fputs(sendBuffer,stdout);
		acknowledgement = XML_TRUE;
	}
}

/* MAIN FUNCTION */

int main(int argc, char *argv[]) {
	FILE *input = stdin;
	time_t deadline;
	int firstTime = XML_TRUE;
	int requireAcknowledgement = XML_FALSE;
	M2MBSM_HANDLERS *pHandlers = M2MBSM_initializeState(&device,behaviorBuffer,M2MBSM_ARRAYCOUNT(behaviorBuffer),MAX_PROPERTIES,propertyBuffer,sizeof(propertyBuffer),parserBuffer,sizeof(parserBuffer),formatBuffer,sizeof(formatBuffer));
	pHandlers->pLogErrorHandler = &logErrorHandler;
	pHandlers->pUnknownCommandHandler = &unknownCommandHandler;
	pHandlers->pRequestPerceptHandler = &requestPerceptHandler;
	pHandlers->pTurnOnOffHandler = &turnOnOffHandler;
	pHandlers->pSetStringOutputHandler = &setStringOutputHandler;
	pHandlers->pSetAnalogOutputHandler = &setAnalogOutputHandler;
	pHandlers->pRebootHandler = &rebootHandler;
	pHandlers->pQueryConfigurationHandler = &queryConfigurationHandler;
	pHandlers->pSetConfigurationHandler = &setConfigurationHandler;
	pHandlers->pSendMessageHandler = &sendMessageHandler;
	
	if (argc >= 2 && (input = fopen(argv[1],"r")) == NULL) {
		fprintf(stderr,"File not found: %s\n",argv[1]);
		return -1;
	}
	
	setbuf(stdout,NULL);
	setbuf(input,NULL);
	while (fgets(receiveBuffer,sizeof(receiveBuffer),input)) {
		int length = strlen(receiveBuffer);
		if (firstTime) {
			firstTime = XML_FALSE;
			if (length-- > 1) {
				if (length >= sizeof(uuid) - 1)
					length = sizeof(uuid) - 1;
				device.pUUID = strncpy(uuid,receiveBuffer,length);
				uuid[length] = '\0';
			}
		} else if (length > 1) {
	    	acknowledgement = XML_FALSE;
			switch (receiveBuffer[0]) {
			case '<':
		    	M2MBSM_parseMessage(receiveBuffer,length);
	    		if (requireAcknowledgement)
	    			checkAcknowledgement();
		    	break;
		    case 'w':					/* 'w'ait # seconds */
		    	deadline = time(NULL) + atoi(receiveBuffer + 1);
		    	while (time(NULL) <= deadline)
		    		processAllBehaviors();
	    		if (requireAcknowledgement)
	    			checkAcknowledgement();
		    	break;
		    case 'a':					/* 'a'cknowledge an outgoing message */
	    		processAllBehaviors();
	    		checkAcknowledgement();
		    	break;
		    case 'n':					/* 'n'othing should happen */
	    		processAllBehaviors();
	    		if (acknowledgement) {
	    			fputs("Nothing should have happened\n",stderr);
	    			return(EXIT_FAILURE);
	    		}
	    		break;
		    case 'p':					/* 'p'ause until no more pending behaviors */
		    	while (M2MBSM_processState() > 0)
		    		;
	    		if (requireAcknowledgement)
	    			checkAcknowledgement();
		    	break;
		    case 'r':					/* 'r'equire acknowledgement true/false */
		    	requireAcknowledgement = atoi(receiveBuffer + 1);
		    	break;
		    case '#':					/* '#' comment */
		    	break;
		    default:
		    	fputs("USAGE: [# ...]|[w<sec>]|[a]|[n]|[p]|[r#]|<M2MXML/>\n",stderr);
			}
		}
	}
  return 0;
}
