#include "rom_types.h"
#include "return_codes.h"
#include "rom_nand_hal_api.h"
//#include "rom_nand_status_codes.h"
#include "stdio.h"

#define MAX_NANDS       4

#define NAND_PAGE_SIZE          2048

#define NAND_TOTAL_SIZE         2112
#define NAND_SUBBLOCK_DATA_SIZE 512

#define NAND_PAGES_PER_BLOCK    8

static bool stc_bGPMIDriverInitialized = false;
static bool stc_bGPMIQueued = false;

bool g_bPack16Bit =  false;

static uint32_t u32GpmiCntr = 0;


extern FILE *g_pNANDFiles[4];
uint8_t g_NANDWorkingBuffer[NAND_TOTAL_SIZE];

char *g_pszFilenamePrefix = "NAND";

#define g_uSector (&g_NANDWorkingBuffer[0])

FILE *g_pNANDFiles[4]= {NULL};
NANDInfo_t Info = {256,NAND_PAGES_PER_BLOCK, SUCCESS };

#define __PRINT_PREP_PROGRESS

//! Convert each of the output binary data files for the NANDs into an
//! ASCII hex dump. This is solely to make it easy for us poor, helpless humans
//! to examine the data.
//!
//! If any of the potentially four input files does not exist, the corresponding
//! ASCII output file will not be created.
//!
//! The \a bIdentifyRedundantArea parameters will cause the output lines that
//! correspond to the redundant areas of each sector to be wrapped in
//! parentheses.
//!
//! The \a pszPostfix string parameter will be appended to the output filename.
//! It must include the file extension, if any. Normally this would be ".txt".
void convert_to_ascii(const char * pszPostfix, bool bIdentifyRedundantArea)
{
    int i;
	for(i=0;i<MAX_NANDS;i++)
	{
		char szFilename[255];
		FILE *pTxtFile = NULL;
		FILE *pBinFile = NULL;
        
		sprintf(szFilename,"%s%01d.dat","NAND", i);

		pBinFile = fopen(szFilename, "rb");
		if(pBinFile)
		{
			fseek(pBinFile,0,SEEK_SET);
			sprintf(szFilename,"%s%01d%s",g_pszFilenamePrefix, i, pszPostfix);
			pTxtFile = fopen(szFilename, "wt");
			fseek(pTxtFile,0,SEEK_SET);

			while(!feof(pBinFile))
			{
				int row,column;
                int subblocks;
                int rows;
				fread(g_uSector,NAND_TOTAL_SIZE,1,pBinFile);
				subblocks = NAND_TOTAL_SIZE / NAND_SUBBLOCK_DATA_SIZE;
				rows = subblocks * 32;
				
				// print sector data
				for(row=0;row<rows;row++)
				{
					if(row)
						fprintf(pTxtFile,"\n");
					for(column=0;column<16;column++)
					{
						// If packing 16 bits, assume it will always be MSB first then LSB.
						if (g_bPack16Bit)
						{
							fprintf(pTxtFile,"%04X ", ((g_uSector[row*16+column+1]<<8)+g_uSector[row*16+column]));
							column++;
						} else
						{
							fprintf(pTxtFile,"%02X ", g_uSector[row*16+column]);
						}
					}
				}
				
				// print sector redundant
				for (row=0; row < subblocks; ++row)
				{
				    fprintf(pTxtFile, "\n");
				    if (bIdentifyRedundantArea)
				    {
					    fprintf(pTxtFile, "  ( ");
					}
					for(column=0;column<16;column++)
					{
						if (g_bPack16Bit)
						{
							fprintf(pTxtFile,"%04X ", ((g_uSector[subblocks * NAND_SUBBLOCK_DATA_SIZE + row*16+column+1]<<8)+g_uSector[subblocks * NAND_SUBBLOCK_DATA_SIZE + row*16+column]));
							column++;
						} else
						{
							fprintf(pTxtFile,"%02X ", g_uSector[subblocks * NAND_SUBBLOCK_DATA_SIZE + row*16+column]);
						}
					}
					if (bIdentifyRedundantArea)
					{
					    fprintf(pTxtFile, " )");
					}
				}
				fprintf(pTxtFile,"\n");
			}
			fclose(pTxtFile);
			fclose(pBinFile);
		}
	}
}


int  rom_nand_hal_ReadPage(uint32_t uNand, uint32_t uSectorOnNand, uint8_t *pBuffer)
{
    if(!stc_bGPMIDriverInitialized)
    {
        rom_nand_hal_GetNandInfo(uNand);
    }
    printf("Reading Sector %d on NAND %d.\n", uSectorOnNand, uNand);
    //fseek(g_pNANDFiles[uNand],NAND_PAGE_SIZE*uSectorOnNand,SEEK_SET);
    fseek(g_pNANDFiles[uNand],NAND_TOTAL_SIZE*uSectorOnNand,SEEK_SET);
    fread(pBuffer, NAND_PAGE_SIZE, 1, g_pNANDFiles[uNand]);
    stc_bGPMIQueued = true;
    return 0;
}

RtStatus_t rom_nand_hal_CurrentGpmiDmaStatus(uint32_t u32NandDeviceNumber)
{
    if(stc_bGPMIQueued && u32GpmiCntr++ > 2)
    {
        stc_bGPMIQueued = false;
        return SUCCESS;
    } else
    {
        return ERROR_ROM_NAND_DMA_BUSY;
    }
    
}


void rom_nand_hal_GpmiSetNandTiming(void * pNANDTiming, uint32_t GpmiClkPeriod)
{
}

RtStatus_t rom_nand_hal_ReadNandID(uint32_t u32NandDeviceNumber, 
                                   uint8_t * pReadIDCode)
{
    return 0;
}

RtStatus_t rom_nand_hal_CheckECCStatus(uint32_t u32NandDeviceNumber, 
                                       uint32_t u32Threshold)
{
    return 0;
}

int rom_nand_hal_WaitForReadComplete(uint32_t u32NandDeviceNumber, 
                                     uint32_t u32Threshold)
{
    if(stc_bGPMIQueued)
    {
        stc_bGPMIQueued = false;
    }
    return 0;
}

rom_nand_hal_Shutdown(void)
{
    if(stc_bGPMIDriverInitialized)
    {
        fclose(g_pNANDFiles[0]);
        fclose(g_pNANDFiles[1]);
        fclose(g_pNANDFiles[2]);
        fclose(g_pNANDFiles[3]);
    }
    stc_bGPMIDriverInitialized = false;

    // now go through each output data file and convert to ASCII.
	//convert_to_ascii(".hex", false);
	//convert_to_ascii(".txt", true);
}


NANDInfo_t *rom_nand_hal_GetNandInfo(uint32_t uNand)
{    
    return &Info;
}

RtStatus_t rom_nand_hal_Init(uint32_t * p32NandMask)
{
    rom_nand_hal_HWInit(p32NandMask);
    return SUCCESS;
}

RtStatus_t rom_nand_hal_HWInit(uint32_t * p32NandMask)
{
    if(!stc_bGPMIDriverInitialized)
    {
        g_pNANDFiles[0]=fopen("NAND0.dat","r+b");
        if(!g_pNANDFiles[0])
            g_pNANDFiles[0]=fopen("NAND0.dat","w+b");
        g_pNANDFiles[1]=fopen("NAND1.dat","r+b");
        if(!g_pNANDFiles[1])
            g_pNANDFiles[1]=fopen("NAND1.dat","w+b");        
        stc_bGPMIDriverInitialized  = true;
    }
    return SUCCESS;
}


#ifdef WRITES_ALLOWED

RtStatus_t rom_nand_hal_EraseBlock(uint32_t uNand, uint32_t uBlockNum)
{
    int i;
#ifdef __PRINT_PREP_PROGRESS
    printf("Erasing Block:   (%02d,%02d).\n", uNand, uBlockNum);
#endif

    for(i=0;i<NAND_TOTAL_SIZE;i++)
    {
        g_NANDWorkingBuffer[i]=0xff;
    }
    fseek(g_pNANDFiles[uNand], uBlockNum * NAND_PAGES_PER_BLOCK * NAND_TOTAL_SIZE, SEEK_SET);
    for(i=0;i<NAND_PAGES_PER_BLOCK;i++)
        fwrite(g_NANDWorkingBuffer,NAND_TOTAL_SIZE,1,g_pNANDFiles[uNand]);

    return SUCCESS;
}


RtStatus_t rom_nand_hal_WriteSector(uint32_t uNand, uint32_t uSector, uint8_t *pBuffer)
{
    int i;
    fseek(g_pNANDFiles[uNand], uSector*NAND_TOTAL_SIZE, SEEK_SET);
    fread(g_NANDWorkingBuffer, NAND_PAGE_SIZE, 1, g_pNANDFiles[uNand]);
    for(i=0;i<NAND_PAGE_SIZE;i++)
    {
        g_NANDWorkingBuffer[i] &= pBuffer[i];
    }
    fseek(g_pNANDFiles[uNand], uSector*NAND_TOTAL_SIZE, SEEK_SET);
    fwrite(g_NANDWorkingBuffer, NAND_PAGE_SIZE, 1, g_pNANDFiles[uNand]);

    return SUCCESS;
}

void rom_nand_hal_StartDma(void *pDmaChain, uint32_t u32NandDeviceNumber) {}

RtStatus_t rom_nand_hal_WaitDma(uint32_t u32uSecTimeout, 
                                uint32_t u32NandDeviceNumber) 
{
    return 0;
}

void rom_nand_hal_ConfigurePinmux(bool bUse16BitData, 
                                  uint32_t u32NumberOfNANDs,
                                   //uint32_t efAltCEPinConfig,
                                   uint32_t efEnableIntPullups,
                                   bool bUse1_8V_Drive)
{
}

void rom_nand_hal_InitReadDma(  void * pReadDmaDescriptor, 
                                uint32_t u32NumRowBytes, 
                                uint32_t u32BusWidth,
                                uint32_t u32ECCSize,
                                uint32_t u32ReadCode1,
                                uint32_t u32ReadCode2) 
{
}

void rom_nand_hal_UpdateDmaDescriptor(void)
{
}

RtStatus_t rom_nand_hal_ReadNand(void * pReadDmaDescriptor, 
                                 uint32_t u32NandDeviceNumber, 
                                 uint32_t u32ColumnOffset, 
                                 uint32_t u32PageNum, 
                                 uint32_t u32ReadSize,
                                 uint8_t *p8PageBuf,
                                 uint8_t *p8AuxillaryBuf)
{
    return 0;
}

RtStatus_t rom_nand_hal_ResetNAND(uint32_t u32NandDeviceNumber)
{
    return 0;
}

#endif // #ifdef WRITES_ALLOWED
