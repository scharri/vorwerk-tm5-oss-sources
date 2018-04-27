#include "rom_types.h"
#include "cpu_support.h"
#include "icoll.h"
#include "icoll_internal.h"
#include "regsicoll.h"


//unsigned int ICOLL_SimVector=5;

/*
if we have only a few IRQ vectors being handled at once, we could sacrifice some performance for more memory, 
and have a sparsely populated table, instead.
*/
void ICOLL_Initialize(ICOLL_HandlerTable_t HandlerTable[ICOLL_VECTOR_COUNT])
{
    int i;
    for(i=0;i<ICOLL_VECTOR_COUNT;i++)
    {
        HandlerTable[i].pHandler = NULL;
        HandlerTable[i].pContextData = NULL;
    }
    //zero out the icoll table
    HW_ICOLL_CTRL_CLR(BM_ICOLL_CTRL_SFTRST|BM_ICOLL_CTRL_CLKGATE);

    //Store the pointer in the ICOLL register
    HW_ICOLL_VBASE_WR((reg32_t)HandlerTable);
}

void (*ICOLL_GetHandler(unsigned int uIRQVectorNumber))(void*)
{
    ICOLL_HandlerTable_t *pHandlerTable = (ICOLL_HandlerTable_t *)HW_ICOLL_VBASE_RD();
    return pHandlerTable[uIRQVectorNumber].pHandler;
}
void *ICOLL_GetContextData(unsigned int uIRQVectorNumber)
{
    ICOLL_HandlerTable_t *pHandlerTable = (ICOLL_HandlerTable_t *)HW_ICOLL_VBASE_RD();
    return pHandlerTable[uIRQVectorNumber].pHandler;
}

void ICOLL_RegisterHandler(unsigned int uIRQVectorNumber, void (*Handler)(void*), void *pContextData)
{
    bool bOldEnable;
#ifdef BUILD_RANGE_CHECK
    if(uIRQVectorNumber <ICOLL_VECTOR_COUNT)
    {
#endif
        ICOLL_HandlerTable_t *pHandlerTable = (ICOLL_HandlerTable_t *)HW_ICOLL_VBASE_RD();
        bOldEnable = cpu_EnableIrqInterrupt(false);
        pHandlerTable[uIRQVectorNumber].pHandler = Handler;
        pHandlerTable[uIRQVectorNumber].pContextData = pContextData;
        cpu_EnableIrqInterrupt(bOldEnable );
#ifdef BUILD_RANGE_CHECK
    }
    else
    {
        SystemHalt();
    }
#endif
}

void ICOLL_Handler(void)
{
    unsigned char *pBaseAddress=0;
    unsigned int uIRQVectorNumber;
    unsigned int uVectorLevel;
    ICOLL_HandlerTable_t *pHandlerTable = (ICOLL_HandlerTable_t *)HW_ICOLL_VBASE_RD();
    //get irq vector
    uIRQVectorNumber  = HW_ICOLL_STAT.B.VECTOR_NUMBER;//needs to be changed the the vector number, no the address.
    HW_ICOLL_VECTOR_WR(uIRQVectorNumber);//tell the state machine that we've begun work on the vector in question.

#ifdef NESTED_INTERRUPTS
    //enable interrupts
    cpu_EnableIrqInterrupt(true);
#endif

    pHandlerTable[uIRQVectorNumber].pHandler(pHandlerTable[uIRQVectorNumber].pContextData);

#ifdef NESTED_INTERRUPTS
    //enable interrupts
    cpu_EnableIrqInterrupt(false);
#endif
    //acknowledge the vector level
    pBaseAddress = ICOLL_GetVectorControlAddress(uIRQVectorNumber);
    uVectorLevel = ((*pBaseAddress)& BM_ICOLL_INTERRUPTn_PRIORITY)>>BP_ICOLL_INTERRUPTn_PRIORITY;
    HW_ICOLL_LEVELACK_SET(1<<uVectorLevel);
}



unsigned char *ICOLL_GetVectorControlAddress(unsigned int uIRQVectorNumber)
{
    unsigned char *pBaseAddress=0;
#ifdef BUILD_RANGE_CHECK
    if(uIRQVectorNumber <ICOLL_VECTOR_COUNT)
    {
#endif
        pBaseAddress = (unsigned char*)HW_ICOLL_INTERRUPTn_ADDR((uIRQVectorNumber));
        //pBaseAddress += uIRQVectorNumber%4;
#ifdef BUILD_RANGE_CHECK
    }
    else
    {
        SystemHalt();
    }
#endif
    return pBaseAddress;
}


void ICOLL_SetVectorPriority(unsigned int uIRQVectorNumber, unsigned int uPriority)
{   
    unsigned char *pBaseAddress = ICOLL_GetVectorControlAddress(uIRQVectorNumber);
#ifdef BUILD_RANGE_CHECK
    if(pBaseAddress)
    {
        //get the old value
        if(uPriority <= HW_ICOLL_PRIORITY0_PRIORITY0__LEVEL3)
        {
#endif
            unsigned char ucRegisterValue = *pBaseAddress;
            ucRegisterValue &= (~BM_ICOLL_INTERRUPTn_PRIORITY);
            ucRegisterValue |= uPriority;
            *pBaseAddress = ucRegisterValue;
#ifdef BUILD_RANGE_CHECK
        }
        else
        {
            SystemHalt();
        }
    }
    else
    {
        SystemHalt();
    }
#endif
}
void ICOLL_SoftTriggerInterrupt(unsigned int uIRQVectorNumber, bool bEnable)
{
    unsigned char *pBaseAddress = ICOLL_GetVectorControlAddress(uIRQVectorNumber);
#ifdef BUILD_RANGE_CHECK
    if(pBaseAddress)
#endif
    {
        if(bEnable)
            *(pBaseAddress + 4) = BM_ICOLL_INTERRUPTn_SOFTIRQ;
        else
            *(pBaseAddress + 8) = BM_ICOLL_INTERRUPTn_SOFTIRQ;
    }
#ifdef BUILD_RANGE_CHECK
    else
    {
        SystemHalt();
    }
#endif
}

void ICOLL_EnableVector(unsigned int uIRQVectorNumber, bool bEnabled)
{
    unsigned char *pBaseAddress = ICOLL_GetVectorControlAddress(uIRQVectorNumber);
#if BUILD_RANGE_CHECK
    if(pBaseAddress)
#endif
    {
        if(bEnabled)
            *(pBaseAddress + 4) = BM_ICOLL_INTERRUPTn_ENABLE;
        else
            *(pBaseAddress + 8) = BM_ICOLL_INTERRUPTn_ENABLE;

    }
#if BUILD_RANGE_CHECK
    else
    {
        SystemHalt();
    }
#endif
}



// eof

