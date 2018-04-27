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

#ifndef _M2MBSM_H_
#define _M2MBSM_H_

#include "m2mxml.h"

#define M2MBSM_ARRAYCOUNT(x)					(sizeof(x)/sizeof(*x))

/* DEVICE BEHAVIORS */

#define M2MBSM_PROP_RPTALLABSRUN	"reportallabsolute.run"
#define M2MBSM_PROP_RPTALLABSTIME	"reportallabsolute.time"

#define M2MBSM_PROP_RPTALLINTRUN	"reportallinterval.run"
#define M2MBSM_PROP_RPTALLINTSTART	"reportallinterval.start"
#define M2MBSM_PROP_RPTALLINTTIME	"reportallinterval.time"

#define M2MBSM_PROP_SETUPDEVICE		"template" /* NOTE: experimental */

/* GENERAL TRANSDUCER BEHAVIORS */

#define M2MBSM_PROP_RPTPTABSRUN		"ReportPtAbsolute.Run"
#define M2MBSM_PROP_RPTPTABSTIME	"ReportPtAbsolute.Time"

#define M2MBSM_PROP_RPTPTINTRUN		"ReportPtInterval.Run"
#define M2MBSM_PROP_RPTPTINTSTART	"ReportPtInterval.Start"
#define M2MBSM_PROP_RPTPTINTTIME	"ReportPtInterval.Time"

/* ANALOG TRANSDUCER BEHAVIORS */

#define M2MBSM_PROP_ANALOGCALIBRATION		"type"

#define M2MBSM_ANALOGCALIBRATIONID_10VDC	0
#define M2MBSM_ANALOGCALIBRATIONID_20MA		1
#define M2MBSM_ANALOGCALIBRATIONID_ICTD		2

#define M2MBSM_PROP_RPTLOW					"low"
#define M2MBSM_PROP_RPTLOWLOW				"lowlow"
#define M2MBSM_PROP_RPTHIGH					"high"
#define M2MBSM_PROP_RPTHIGHHIGH				"highhigh"

/* DIGITAL INPUT TRANSDUCER BEHAVIORS */

#define M2MBSM_PROP_RPTLEVELCHANGE			"alarmLevel"

#define M2MBSM_LEVELCHANGE_OFF				0
#define M2MBSM_LEVELCHANGE_LOW2HIGH			1
#define M2MBSM_LEVELCHANGE_HIGH2LOW			2

/* DIGITAL OUTPUT TRANSDUCER BEHAVIORS */

#define M2MBSM_PROP_PULSELOWRUN				"PulseLow.Run"
#define M2MBSM_PROP_PULSELOWTIME			"PulseLow.Time"

#define M2MBSM_PROP_PULSEHIGHRUN			"PulseHigh.Run"
#define M2MBSM_PROP_PULSEHIGHTIME			"PulseHigh.Time"

#define M2MBSM_PROP_TIMERLOWABSRUN			"TimerLowAbsolute.Run"
#define M2MBSM_PROP_TIMERLOWABSTIME			"TimerLowAbsolute.Time"

#define M2MBSM_PROP_TIMERHIGHABSRUN			"TimerHighAbsolute.Run"
#define M2MBSM_PROP_TIMERHIGHABSTIME		"TimerHighAbsolute.Time"

#define M2MBSM_PROP_TIMERLOWINTRUN			"TimerLowInterval.Run"
#define M2MBSM_PROP_TIMERLOWINTTIME			"TimerLowInterval.Time"

#define M2MBSM_PROP_TIMERHIGHINTRUN			"TimerHighInterval.Run"
#define M2MBSM_PROP_TIMERHIGHINTTIME		"TimerHighInterval.Time"

/* VALUE DEFINITION */

typedef struct {
	int size;
	char *pData;
} M2MBSM_VALUE;

#define M2MBSM_DEFINEVALUE(x)					{sizeof(x),(x)}

/* PROPERTY DEFINITION */

typedef struct {
	const char *pKey;
	M2MBSM_VALUE value;
	int show;
} M2MBSM_PROPERTY;

/* TRANSDUCER DEFINITION */

#define M2MBSM_DIRECTION_HIDDEN					0
#define M2MBSM_DIRECTION_IN						1
#define M2MBSM_DIRECTION_OUT					2
#define M2MBSM_DIRECTION_INOUT					(M2MBSM_DIRECTION_IN | M2MBSM_DIRECTION_OUT)

typedef struct {
	void *pAppData;
	int type;
	int inout;
	const char *pAddress;
	const char *pLabel;
	const char *pUnits;
	M2MBSM_PROPERTY *pProperties;
	int propertyCount;
	M2MBSM_VALUE lastValue;
	M2MBSM_VALUE currentValue;
} M2MBSM_TRANSDUCER;

/* DEVICE DEFINITION */

typedef struct {
	void *pAppData;
	const char *pUUID;
	M2MBSM_TRANSDUCER *pTransducers;
	int transducerCount;
	M2MBSM_PROPERTY *pProperties;
	int propertyCount;
} M2MBSM_DEVICE;

/* BEHAVIOR DEFINITION */

#define M2MBSM_BEHAVIOREVENTID_MATCH			0
#define M2MBSM_BEHAVIOREVENTID_CONFIGURE		1
#define M2MBSM_BEHAVIOREVENTID_EXECUTE			2

typedef struct _M2MBSM_BEHAVIOR {
	void *pAppData;
	M2MBSM_TRANSDUCER *pTransducer;
	int (*pHandler)(M2MBSM_DEVICE *,struct _M2MBSM_BEHAVIOR *,int,time_t);
	time_t nextFiring;
	time_t firingPeriod;
	struct _M2MBSM_BEHAVIOR *pNext;
} M2MBSM_BEHAVIOR;

/* HANDLERS DEFINITION */

typedef int (*M2MBSM_HANDLER_REQUESTPERCEPT)(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,time_t *pTimestamp);
typedef int (*M2MBSM_HANDLER_TURNONOFF)(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,int sense,const char **ppProperties);
typedef int (*M2MBSM_HANDLER_SETSTRINGOUTPUT)(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,const char *pValue);
typedef int (*M2MBSM_HANDLER_SETANALOGOUTPUT)(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,double setPoint);
typedef int (*M2MBSM_HANDLER_REBOOT)(M2MBSM_DEVICE *pDevice);
typedef int (*M2MBSM_HANDLER_QUERYCONFIGURATION)(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty);
typedef int (*M2MBSM_HANDLER_SETCONFIGURATION)(M2MBSM_DEVICE *pDevice,M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty);
typedef int (*M2MBSM_HANDLER_BEHAVIOR)(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
typedef void (*M2MBSM_HANDLER_SENDMESSAGE)(M2MBSM_DEVICE * pDevice,void *pData);

typedef struct {
	void *pAppData;
	
	M2MXML_HANDLER_LOGERROR				pLogErrorHandler;
	M2MXML_HANDLER_UNKNOWN				pUnknownCommandHandler;

	M2MBSM_HANDLER_REQUESTPERCEPT		pRequestPerceptHandler;
	M2MBSM_HANDLER_TURNONOFF			pTurnOnOffHandler;
	M2MBSM_HANDLER_SETSTRINGOUTPUT		pSetStringOutputHandler;
	M2MBSM_HANDLER_SETANALOGOUTPUT		pSetAnalogOutputHandler;
	M2MBSM_HANDLER_REBOOT				pRebootHandler;
	M2MBSM_HANDLER_QUERYCONFIGURATION	pQueryConfigurationHandler;
	M2MBSM_HANDLER_SETCONFIGURATION		pSetConfigurationHandler;
	
	M2MBSM_HANDLER_SENDMESSAGE			pSendMessageHandler;
} M2MBSM_HANDLERS;

/* STANDARD BEHAVIOR EVENT HANDLERS */

int M2MBSM_reportAllAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_reportPointAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_reportAllIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_reportPointIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_reportDigitalTransitionBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_reportOutOfBoundsBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_setupDeviceBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_timerOffAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_timerOnAbsoluteBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_timerOffIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);
int M2MBSM_timerOnIntervalBehaviorHandler(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehavior,int event,time_t timestamp);

/* EXTERNAL API */

int M2MBSM_configureBehavior(M2MBSM_TRANSDUCER *pTransducer,M2MBSM_PROPERTY *pProperty,M2MBSM_HANDLER_BEHAVIOR pBehaviorHandler);
int M2MBSM_countBehaviors(void);
M2MBSM_PROPERTY *M2MBSM_findProperty(M2MBSM_PROPERTY *pProperties,int propertyCount,const char *pKey);
M2MBSM_HANDLERS *M2MBSM_initializeState(M2MBSM_DEVICE *pDevice,M2MBSM_BEHAVIOR *pBehaviorBuffer,int maxBehaviors,int maxProperties,char *pPropertyBuffer,int propertySize,char *pParserBuffer,int parserSize,char *pFormatBuffer,int formatSize);
time_t M2MBSM_nextStartTime(int hhmmss,int mustBeFuture);
void M2MBSM_parseMessage(const char *pMessageBuffer,int messageSize);
time_t M2MBSM_processState(void);

#endif
