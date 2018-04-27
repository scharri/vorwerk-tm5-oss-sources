#ifndef __ICOLL_H
#define __ICOLL_H

#include "rom_types.h"

//#include "hw_irq.h"

#define ICOLL_VECTOR_COUNT 128

struct ICOLL_HandlerEntry
{
    void      (*pHandler)(void*);
    void       *pContextData;
};

typedef struct ICOLL_HandlerEntry  ICOLL_HandlerTable_t;

//called to initialize the ICOLL code
void ICOLL_Initialize(ICOLL_HandlerTable_t HandlerTable[ICOLL_VECTOR_COUNT]);

//These two functions are used to get the current handlers, if necessary
void *ICOLL_GetContextData(unsigned int uIRQVectorNumber);
void (*ICOLL_GetHandler(unsigned int uIRQVectorNumber))(void*);

//Register the associate a vector with an IRQ handler, plus the context data that will be passed to the vector
//when the IRQ happens
void ICOLL_RegisterHandler(unsigned int uIRQVectorNumber, void (*Handler)(void*), void *pContextData);

//self explanatory
void ICOLL_SetVectorPriority(unsigned int uIRQVectorNumber, unsigned int uPriority);

//self explanatory
void ICOLL_SoftTriggerInterrupt(unsigned int uIRQVectorNumber, bool bEnable);

//self explanatory
void ICOLL_EnableVector(unsigned int uIRQVectorNumber, bool bEnable);


#endif //__ICOLL_H

