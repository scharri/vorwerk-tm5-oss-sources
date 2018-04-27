#ifndef regsusb20inc_h_
#define regsusb20inc_h_
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;  Copyright(C) SigmaTel, Inc. 2002-2003
//;  File        : regsusb20ip.inc
//;  Description : USB20 IP Register definition
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

// The following naming conventions are followed in this file.
// All registers are named using the format...
//     HW_<module>_<regname>
// where <module> is the module name which can be any of the following...
//     USB20
// (Note that when there is more than one copy of a particular module, the
// module name includes a number starting from 0 for the first instance of
// that module)
// <regname> is the specific register within that module
// We also define the following...
//     HW_<module>_<regname>_BITPOS
// which defines the starting bit (i.e. LSB) of a multi bit field
//     HW_<module>_<regname>_SETMASK
// which does something else, and
//     HW_<module>_<regname>_CLRMASK
// which does something else.
// Other rules
//     All caps
//     Numeric identifiers start at 0

// Environment variables. Include this first.
#include "rom_types.h"

#ifndef MAX_NUM_EP
#define MAX_NUM_EP  5
#endif 

#pragma pack(1)
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//   USB2.0 ARC Registers 
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#define HW_ARC_BASE_ADDR            (0x80080000)

#define HW_ARC_ID_ADDR              (HW_ARC_BASE_ADDR + 0x0000)
#define HW_ARC_HCSPARAMS_ADDR       (HW_ARC_BASE_ADDR + 0x0104)
#define HW_ARC_USBCMD_ADDR          (HW_ARC_BASE_ADDR + 0x0140)
#define HW_ARC_USBSTS_ADDR          (HW_ARC_BASE_ADDR + 0x0144)
#define HW_ARC_USBINTR_ADDR         (HW_ARC_BASE_ADDR + 0x0148)
#define HW_ARC_FRINDEX_ADDR         (HW_ARC_BASE_ADDR + 0x014C)
#define HW_ARC_DEVADDR_ADDR         (HW_ARC_BASE_ADDR + 0x0154)
#define HW_ARC_ENDPTLISTADDR_ADDR   (HW_ARC_BASE_ADDR + 0x0158)
#define HW_ARC_PORTSC1_ADDR         (HW_ARC_BASE_ADDR + 0x0184)
#define HW_ARC_OTGSC_ADDR           (HW_ARC_BASE_ADDR + 0x01a4)
#define HW_ARC_USBMODE_ADDR         (HW_ARC_BASE_ADDR + 0x01a8)
#define HW_ARC_ENDPTSETUPSTAT_ADDR  (HW_ARC_BASE_ADDR + 0x01ac)
#define HW_ARC_ENDPTPRIME_ADDR      (HW_ARC_BASE_ADDR + 0x01b0)
#define HW_ARC_ENDPTFLUSH_ADDR      (HW_ARC_BASE_ADDR + 0x01b4)
#define HW_ARC_ENDPTSTATUS_ADDR     (HW_ARC_BASE_ADDR + 0x01b8)
#define HW_ARC_ENDPTCOMPLETE_ADDR   (HW_ARC_BASE_ADDR + 0x01bc)
#define HW_ARC_ENDPTCTRL0_ADDR      (HW_ARC_BASE_ADDR + 0x01c0)
#define HW_ARC_ENDPTCTRL1_ADDR      (HW_ARC_BASE_ADDR + 0x01c4)
#define HW_ARC_ENDPTCTRL2_ADDR      (HW_ARC_BASE_ADDR + 0x01c8)
#define HW_ARC_ENDPTCTRL3_ADDR      (HW_ARC_BASE_ADDR + 0x01cc)
#define HW_ARC_ENDPTCTRL4_ADDR      (HW_ARC_BASE_ADDR + 0x01d0)
#define HW_ARC_ENDPTCTRL5_ADDR      (HW_ARC_BASE_ADDR + 0x01d4)
#define HW_ARC_ENDPTCTRL6_ADDR      (HW_ARC_BASE_ADDR + 0x01d8)
#define HW_ARC_ENDPTCTRL7_ADDR      (HW_ARC_BASE_ADDR + 0x01dc)
#define HW_ARC_ENDPTCTRL8_ADDR      (HW_ARC_BASE_ADDR + 0x01e0)
#define HW_ARC_ENDPTCTRL9_ADDR      (HW_ARC_BASE_ADDR + 0x01e4)
#define HW_ARC_ENDPTCTRL10_ADDR     (HW_ARC_BASE_ADDR + 0x01e8)
#define HW_ARC_ENDPTCTRL11_ADDR     (HW_ARC_BASE_ADDR + 0x01ec)
#define HW_ARC_ENDPTCTRL12_ADDR     (HW_ARC_BASE_ADDR + 0x01f0)
#define HW_ARC_ENDPTCTRL13_ADDR     (HW_ARC_BASE_ADDR + 0x01f4)
#define HW_ARC_ENDPTCTRL14_ADDR     (HW_ARC_BASE_ADDR + 0x01f8)
#define HW_ARC_ENDPTCTRL15_ADDR     (HW_ARC_BASE_ADDR + 0x01fc)

#define HW_ARC_ENDPTCTRL_ADDR(n)    (HW_ARC_BASE_ADDR + 0x01c0+((n)*4))


/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register Host Control Structural Parameters (HW_ARC_HCSPARAMS)

#define HW_ARC_HCSPARAMS_NPORTS_BITPOS (0)
#define HW_ARC_HCSPARAMS_PPC_BITPOS (4)
#define HW_ARC_HCSPARAMS_NPCC_BITPOS (8)
#define HW_ARC_HCSPARAMS_NCC_BITPOS (12)
#define HW_ARC_HCSPARAMS_PI_BITPOS (16)
#define HW_ARC_HCSPARAMS_NPTT_BITPOS (20)
#define HW_ARC_HCSPARAMS_NTT_BITPOS (24)

#define HW_ARC_HCSPARAMS_NPORTS_SETMASK (15<<HW_ARC_HCSPARAMS_NPORTS_BITPOS)
#define HW_ARC_HCSPARAMS_PPC_SETMASK (1<<HW_ARC_HCSPARAMS_PPC_BITPOS)        
#define HW_ARC_HCSPARAMS_NPCC_SETMASK (15<<HW_ARC_HCSPARAMS_NPCC_BITPOS)  
#define HW_ARC_HCSPARAMS_NCC_SETMASK (15<<HW_ARC_HCSPARAMS_NCC_BITPOS)       
#define HW_ARC_HCSPARAMS_PI_SETMASK (1<<HW_ARC_HCSPARAMS_PI_BITPOS)     
#define HW_ARC_HCSPARAMS_NPTT_SETMASK (15<<HW_ARC_HCSPARAMS_NPTT_BITPOS)  
#define HW_ARC_HCSPARAMS_NTT_SETMASK (15<<HW_ARC_HCSPARAMS_NTT_BITPOS)       

#define HW_ARC_HCSPARAMS_NPORTS_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_NPORTS_SETMASK)
#define HW_ARC_HCSPARAMS_PPC_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_PPC_SETMASK)
#define HW_ARC_HCSPARAMS_NPCC_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_NPCC_SETMASK)
#define HW_ARC_HCSPARAMS_NCC_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_NCC_SETMASK)
#define HW_ARC_HCSPARAMS_PI_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_PI_SETMASK)  
#define HW_ARC_HCSPARAMS_NPTT_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_NPTT_SETMASK)
#define HW_ARC_HCSPARAMS_NTT_CLRMASK (~(WORD)HW_ARC_HCSPARAMS_NTT_SETMASK)

typedef union               
{
    struct {
        unsigned int N_PORTS         :4;
        unsigned int PPC             :1;
        unsigned int                 :3;
        unsigned int N_PCC           :4;
        unsigned int N_CC            :4;
        unsigned int PI              :1;
        unsigned int                 :3;
        unsigned int N_PTT           :4;
        unsigned int N_TT            :4;
        unsigned int                 :4;
    }; 
    unsigned int u32;
} hcsparams_type;
#define HW_ARC_HCSPARAMS (*(volatile hcsparams_type*) (HW_ARC_HCSPARAMS_ADDR))    

/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Command (HW_ARC_USBCMD)

#define HW_ARC_USBCMD_RS_BITPOS (0)
#define HW_ARC_USBCMD_RST_BITPOS (1)
#define HW_ARC_USBCMD_FS0_BITPOS (2)
#define HW_ARC_USBCMD_FS1_BITPOS (3)
#define HW_ARC_USBCMD_PSE_BITPOS (4)
#define HW_ARC_USBCMD_ASE_BITPOS (5)
#define HW_ARC_USBCMD_IAA_BITPOS (6)
#define HW_ARC_USBCMD_LR_BITPOS (7)
#define HW_ARC_USBCMD_ASP0_BITPOS (8)
#define HW_ARC_USBCMD_ASP1_BITPOS (9)
#define HW_ARC_USBCMD_ASPE_BITPOS (11)
#define HW_ARC_USBCMD_FS2_BITPOS (15)
#define HW_ARC_USBCMD_ITC_BITPOS (16)

#define HW_ARC_USBCMD_RS_SETMASK (1<<HW_ARC_USBCMD_RS_BITPOS)                
#define HW_ARC_USBCMD_RST_SETMASK (1<<HW_ARC_USBCMD_RST_BITPOS)   
#define HW_ARC_USBCMD_FS0_SETMASK (1<<HW_ARC_USBCMD_FS0_BITPOS)   
#define HW_ARC_USBCMD_FS1_SETMASK (1<<HW_ARC_USBCMD_FS1_BITPOS)   
#define HW_ARC_USBCMD_PSE_SETMASK (1<<HW_ARC_USBCMD_PSE_BITPOS)   
#define HW_ARC_USBCMD_ASE_SETMASK (1<<HW_ARC_USBCMD_ASE_BITPOS)   
#define HW_ARC_USBCMD_IAA_SETMASK (1<<HW_ARC_USBCMD_IAA_BITPOS)   
#define HW_ARC_USBCMD_LR_SETMASK (1<<HW_ARC_USBCMD_LR_BITPOS)        
#define HW_ARC_USBCMD_ASP0_SETMASK (1<<HW_ARC_USBCMD_ASP0_BITPOS)
#define HW_ARC_USBCMD_ASP1_SETMASK (1<<HW_ARC_USBCMD_ASP1_BITPOS)
#define HW_ARC_USBCMD_ASPE_SETMASK (1<<HW_ARC_USBCMD_ASPE_BITPOS)
#define HW_ARC_USBCMD_FS2_SETMASK (1<<HW_ARC_USBCMD_FS2_BITPOS)    
#define HW_ARC_USBCMD_ITC_SETMASK (255<<HW_ARC_USBCMD_ITC_BITPOS)

#define HW_ARC_USBCMD_RS_CLRMASK (~(WORD)HW_ARC_USBCMD_RS_SETMASK)     
#define HW_ARC_USBCMD_RST_CLRMASK (~(WORD)HW_ARC_USBCMD_RST_SETMASK)    
#define HW_ARC_USBCMD_FS0_CLRMASK (~(WORD)HW_ARC_USBCMD_FS0_SETMASK)    
#define HW_ARC_USBCMD_FS1_CLRMASK (~(WORD)HW_ARC_USBCMD_FS1_SETMASK)    
#define HW_ARC_USBCMD_PSE_CLRMASK (~(WORD)HW_ARC_USBCMD_PSE_SETMASK)    
#define HW_ARC_USBCMD_ASE_CLRMASK (~(WORD)HW_ARC_USBCMD_ASE_SETMASK)    
#define HW_ARC_USBCMD_IAA_CLRMASK (~(WORD)HW_ARC_USBCMD_IAA_SETMASK)    
#define HW_ARC_USBCMD_LR_CLRMASK (~(WORD)HW_ARC_USBCMD_LR_SETMASK) 
#define HW_ARC_USBCMD_ASP0_CLRMASK (~(WORD)HW_ARC_USBCMD_ASP0_SETMASK)
#define HW_ARC_USBCMD_ASP1_CLRMASK (~(WORD)HW_ARC_USBCMD_ASP1_SETMASK)
#define HW_ARC_USBCMD_ASPE_CLRMASK (~(WORD)HW_ARC_USBCMD_ASPE_SETMASK)
#define HW_ARC_USBCMD_FS2_CLRMASK (~(WORD)HW_ARC_USBCMD_FS2_SETMASK)    
#define HW_ARC_USBCMD_ITC_CLRMASK (~(WORD)HW_ARC_USBCMD_ITC_SETMASK)    

typedef union               
{
    struct {
        unsigned int RS     :1;
        unsigned int RST             :1;
        unsigned int FS0             :1;
        unsigned int FS1             :1;
        unsigned int PSE             :1;
        unsigned int ASE             :1;
        unsigned int IAA             :1;
        unsigned int LR              :1;
        unsigned int ASP0            :1;
        unsigned int ASP1            :1;
        unsigned int                 :1;
        unsigned int ASPE            :1;
        unsigned int                 :3;
        unsigned int FS2             :1;
        unsigned int ITC             :8;
        unsigned int                 :8;
    }; 
    unsigned int u32;
} usbcmd_type;
#define HW_ARC_USBCMD (*(volatile usbcmd_type *)(HW_ARC_USBCMD_ADDR))    

/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Status (HW_ARC_USBSTS)

#define HW_ARC_USBSTS_UI_BITPOS (0)
#define HW_ARC_USBSTS_UEI_BITPOS (1)
#define HW_ARC_USBSTS_PCI_BITPOS (2)
#define HW_ARC_USBSTS_FRI_BITPOS (3)
#define HW_ARC_USBSTS_SEI_BITPOS (4)
#define HW_ARC_USBSTS_AAI_BITPOS (5)
#define HW_ARC_USBSTS_URI_BITPOS (6)
#define HW_ARC_USBSTS_SRI_BITPOS (7)
#define HW_ARC_USBSTS_SLI_BITPOS (8)
#define HW_ARC_USBSTS_HCH_BITPOS (12)
#define HW_ARC_USBSTS_RCL_BITPOS (13)
#define HW_ARC_USBSTS_PS_BITPOS (14)
#define HW_ARC_USBSTS_AS_BITPOS (15)

#define HW_ARC_USBSTS_UI_SETMASK (1<<HW_ARC_USBSTS_UI_BITPOS)    
#define HW_ARC_USBSTS_UEI_SETMASK (1<<HW_ARC_USBSTS_UEI_BITPOS)
#define HW_ARC_USBSTS_PCI_SETMASK (1<<HW_ARC_USBSTS_PCI_BITPOS)
#define HW_ARC_USBSTS_FRI_SETMASK (1<<HW_ARC_USBSTS_FRI_BITPOS)
#define HW_ARC_USBSTS_SEI_SETMASK (1<<HW_ARC_USBSTS_SEI_BITPOS)
#define HW_ARC_USBSTS_AAI_SETMASK (1<<HW_ARC_USBSTS_AAI_BITPOS)
#define HW_ARC_USBSTS_URI_SETMASK (1<<HW_ARC_USBSTS_URI_BITPOS)
#define HW_ARC_USBSTS_SRI_SETMASK (1<<HW_ARC_USBSTS_SRI_BITPOS)
#define HW_ARC_USBSTS_SLI_SETMASK (1<<HW_ARC_USBSTS_SLI_BITPOS)
#define HW_ARC_USBSTS_HCH_SETMASK (1<<HW_ARC_USBSTS_HCH_BITPOS)
#define HW_ARC_USBSTS_RCL_SETMASK (1<<HW_ARC_USBSTS_RCL_BITPOS)
#define HW_ARC_USBSTS_PS_SETMASK (1<<HW_ARC_USBSTS_PS_BITPOS)    
#define HW_ARC_USBSTS_AS_SETMASK (1<<HW_ARC_USBSTS_AS_BITPOS)    

#define HW_ARC_USBSTS_UI_CLRMASK (~(WORD)HW_ARC_USBSTS_UI_SETMASK)
#define HW_ARC_USBSTS_UEI_CLRMASK (~(WORD)HW_ARC_USBSTS_UEI_SETMASK)
#define HW_ARC_USBSTS_PCI_CLRMASK (~(WORD)HW_ARC_USBSTS_PCI_SETMASK)
#define HW_ARC_USBSTS_FRI_CLRMASK (~(WORD)HW_ARC_USBSTS_FRI_SETMASK)
#define HW_ARC_USBSTS_SEI_CLRMASK (~(WORD)HW_ARC_USBSTS_SEI_SETMASK)
#define HW_ARC_USBSTS_AAI_CLRMASK (~(WORD)HW_ARC_USBSTS_AAI_SETMASK)
#define HW_ARC_USBSTS_URI_CLRMASK (~(WORD)HW_ARC_USBSTS_URI_SETMASK)
#define HW_ARC_USBSTS_SRI_CLRMASK (~(WORD)HW_ARC_USBSTS_SRI_SETMASK)
#define HW_ARC_USBSTS_SLI_CLRMASK (~(WORD)HW_ARC_USBSTS_SLI_SETMASK)
#define HW_ARC_USBSTS_HCH_CLRMASK (~(WORD)HW_ARC_USBSTS_HCH_SETMASK)
#define HW_ARC_USBSTS_RCL_CLRMASK (~(WORD)HW_ARC_USBSTS_RCL_SETMASK)
#define HW_ARC_USBSTS_PS_CLRMASK (~(WORD)HW_ARC_USBSTS_PS_SETMASK)
#define HW_ARC_USBSTS_AS_CLRMASK (~(WORD)HW_ARC_USBSTS_AS_SETMASK)


typedef union               
{
    struct {
        unsigned int UI              :1;
        unsigned int UEI             :1;
        unsigned int PCI             :1;
        unsigned int FRI             :1;
        unsigned int SEI             :1;
        unsigned int AAI             :1;
        unsigned int URI             :1;
        unsigned int SRI             :1;
        unsigned int SLI             :1;
        unsigned int                 :3;
        unsigned int HCH             :1;
        unsigned int RCL             :1;
        unsigned int PS              :1;
        unsigned int AS              :1;
        unsigned int                 :16;  
    }; 
    unsigned int u32;
} usbsts_type;
#define HW_ARC_USBSTS (*(volatile usbsts_type *) (HW_ARC_USBSTS_ADDR))    

/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Interrupt Enable (HW_ARC_USBINTR)

#define HW_ARC_USBINTR_UE_BITPOS (0)
#define HW_ARC_USBINTR_UEE_BITPOS (1)
#define HW_ARC_USBINTR_PCE_BITPOS (2)
#define HW_ARC_USBINTR_FRE_BITPOS (3)
#define HW_ARC_USBINTR_SEE_BITPOS (4)
#define HW_ARC_USBINTR_AAE_BITPOS (5)
#define HW_ARC_USBINTR_URE_BITPOS (6)
#define HW_ARC_USBINTR_SRE_BITPOS (7)
#define HW_ARC_USBINTR_SLE_BITPOS (8)

#define HW_ARC_USBINTR_UE_SETMASK (1<<HW_ARC_USBINTR_UE_BITPOS)   
#define HW_ARC_USBINTR_UEE_SETMASK (1<<HW_ARC_USBINTR_UEE_BITPOS)
#define HW_ARC_USBINTR_PCE_SETMASK (1<<HW_ARC_USBINTR_PCE_BITPOS)
#define HW_ARC_USBINTR_FRE_SETMASK (1<<HW_ARC_USBINTR_FRE_BITPOS)
#define HW_ARC_USBINTR_SEE_SETMASK (1<<HW_ARC_USBINTR_SEE_BITPOS)
#define HW_ARC_USBINTR_AAE_SETMASK (1<<HW_ARC_USBINTR_AAE_BITPOS)
#define HW_ARC_USBINTR_URE_SETMASK (1<<HW_ARC_USBINTR_URE_BITPOS)
#define HW_ARC_USBINTR_SRE_SETMASK (1<<HW_ARC_USBINTR_SRE_BITPOS)
#define HW_ARC_USBINTR_SLE_SETMASK (1<<HW_ARC_USBINTR_SLE_BITPOS)

#define HW_ARC_USBINTR_UE_CLRMASK (~(WORD)HW_ARC_USBINTR_UE_SETMASK)
#define HW_ARC_USBINTR_UEE_CLRMASK (~(WORD)HW_ARC_USBINTR_UEE_SETMASK)
#define HW_ARC_USBINTR_PCE_CLRMASK (~(WORD)HW_ARC_USBINTR_PCE_SETMASK)
#define HW_ARC_USBINTR_FRE_CLRMASK (~(WORD)HW_ARC_USBINTR_FRE_SETMASK)
#define HW_ARC_USBINTR_SEE_CLRMASK (~(WORD)HW_ARC_USBINTR_SEE_SETMASK)
#define HW_ARC_USBINTR_AAE_CLRMASK (~(WORD)HW_ARC_USBINTR_AAE_SETMASK)
#define HW_ARC_USBINTR_URE_CLRMASK (~(WORD)HW_ARC_USBINTR_URE_SETMASK)
#define HW_ARC_USBINTR_SRE_CLRMASK (~(WORD)HW_ARC_USBINTR_SRE_SETMASK)
#define HW_ARC_USBINTR_SLE_CLRMASK (~(WORD)HW_ARC_USBINTR_SLE_SETMASK)


typedef union               
{
    struct {
        unsigned int UE              :1;
        unsigned int UEE             :1;
        unsigned int PCE             :1;
        unsigned int FRE             :1;
        unsigned int SEE             :1;
        unsigned int AAE             :1;
        unsigned int URE             :1;
        unsigned int SRE             :1;
        unsigned int SLE             :1;
        unsigned int                 :23;  
    }; 
    unsigned int u32;
} usbintr_type;
#define HW_ARC_USBINTR (*(volatile usbintr_type *) (HW_ARC_USBINTR_ADDR))    


/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Device Controller Device Address (HW_ARC_DEVADDR)

#define HW_ARC_DEVADDR_ADD_BITPOS (25)

#define HW_ARC_DEVADDR_ADD_SETMASK (127<<HW_ARC_DEVADDR_ADD_BITPOS)    

#define HW_ARC_DEVEADDR_ADD_CLRMASK (~(WORD)HW_ARC_DEVADDR_ADD_SETMASK)   

typedef union               
{
    struct {
        unsigned int                 :25;
        unsigned int ADD             :7;
    }; 
    unsigned int u32;
} devaddr_type;
#define HW_ARC_DEVADDR (*(volatile devaddr_type*) (HW_ARC_DEVADDR_ADDR))    


/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Device Controller Endpoint List Address (HW_ARC_ENDPTLISTADDR)

#define HW_ARC_ENDPTLISTADDR_ADD_BITPOS (11)

#define HW_ARC_ENDPTLISTADDR_ADD_SETMASK (0x400000<<HW_ARC_ENDPTLISTADDR_ADD_BITPOS) 

#define HW_ARC_ENDPTLISTADDR_ADD_CLRMASK (~(WORD)HW_ARC_ENDPTLISTADDR_ADD_SETMASK) 

typedef union               
{
    struct {
        void* ADDR            ;
    }; 
    unsigned int u32;
} endptlistaddr_type;
#define HW_ARC_ENDPTLISTADDR (*(volatile endptlistaddr_type *) (HW_ARC_ENDPTLISTADDR_ADDR))    


/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Port Status Control 1 (HW_ARC_PORTSC1)

#define HW_ARC_PORTSC1_CCS_BITPOS (0)
#define HW_ARC_PORTSC1_CSC_BITPOS (1)
#define HW_ARC_PORTSC1_PE_BITPOS (2)
#define HW_ARC_PORTSC1_PEC_BITPOS (3)
#define HW_ARC_PORTSC1_OCA_BITPOS (4)
#define HW_ARC_PORTSC1_OCC_BITPOS (5)
#define HW_ARC_PORTSC1_FPR_BITPOS (6)
#define HW_ARC_PORTSC1_SUSP_BITPOS (7)
#define HW_ARC_PORTSC1_PR_BITPOS (8)
#define HW_ARC_PORTSC1_HSP_BITPOS (9)
#define HW_ARC_PORTSC1_LS_BITPOS (10)
#define HW_ARC_PORTSC1_PP_BITPOS (12)
#define HW_ARC_PORTSC1_PO_BITPOS (13)
#define HW_ARC_PORTSC1_PIC_BITPOS (14)
#define HW_ARC_PORTSC1_PTC_BITPOS (16)
#define HW_ARC_PORTSC1_WKCN_BITPOS (20)
#define HW_ARC_PORTSC1_WKDS_BITPOS (21)
#define HW_ARC_PORTSC1_WKOC_BITPOS (22)
#define HW_ARC_PORTSC1_PHCD_BITPOS (23)
#define HW_ARC_PORTSC1_PFSC_BITPOS (24)
#define HW_ARC_PORTSC1_PSPD_BITPOS (26)
#define HW_ARC_PORTSC1_PTW_BITPOS (29)
#define HW_ARC_PORTSC1_STS_BITPOS (30)
#define HW_ARC_PORTSC1_PTS_BITPOS (31)

#define HW_ARC_PORTSC1_CCS_SETMASK (1<<HW_ARC_PORTSC1_CCS_BITPOS)  
#define HW_ARC_PORTSC1_CSC_SETMASK (1<<HW_ARC_PORTSC1_CSC_BITPOS)  
#define HW_ARC_PORTSC1_PE_SETMASK (1<<HW_ARC_PORTSC1_PE_BITPOS)   
#define HW_ARC_PORTSC1_PEC_SETMASK (1<<HW_ARC_PORTSC1_PEC_BITPOS)  
#define HW_ARC_PORTSC1_OCA_SETMASK (1<<HW_ARC_PORTSC1_OCA_BITPOS)  
#define HW_ARC_PORTSC1_OCC_SETMASK (1<<HW_ARC_PORTSC1_OCC_BITPOS)  
#define HW_ARC_PORTSC1_FPR_SETMASK (1<<HW_ARC_PORTSC1_FPR_BITPOS)  
#define HW_ARC_PORTSC1_SUSP_SETMASK (1<<HW_ARC_PORTSC1_SUSP_BITPOS)
#define HW_ARC_PORTSC1_PR_SETMASK (1<<HW_ARC_PORTSC1_PR_BITPOS)   
#define HW_ARC_PORTSC1_HSP_SETMASK (1<<HW_ARC_PORTSC1_HSP_BITPOS)  
#define HW_ARC_PORTSC1_LS_SETMASK (3<<HW_ARC_PORTSC1_LS_BITPOS)   
#define HW_ARC_PORTSC1_PP_SETMASK (1<<HW_ARC_PORTSC1_PP_BITPOS)   
#define HW_ARC_PORTSC1_PO_SETMASK (1<<HW_ARC_PORTSC1_PO_BITPOS)   
#define HW_ARC_PORTSC1_PIC_SETMASK (3<<HW_ARC_PORTSC1_PIC_BITPOS)  
#define HW_ARC_PORTSC1_PTC_SETMASK (15<<HW_ARC_PORTSC1_PTC_BITPOS) 
#define HW_ARC_PORTSC1_WKCN_SETMASK (1<<HW_ARC_PORTSC1_WKCN_BITPOS)
#define HW_ARC_PORTSC1_WKDS_SETMASK (1<<HW_ARC_PORTSC1_WKDS_BITPOS)
#define HW_ARC_PORTSC1_WKOC_SETMASK (1<<HW_ARC_PORTSC1_WKOC_BITPOS)
#define HW_ARC_PORTSC1_PHCD_SETMASK (1<<HW_ARC_PORTSC1_PHCD_BITPOS)

// We need to equate the following label like this due to a sign extension problem
// if equated like so (1<<HW_ARC_PORTSC1_PFSC_SETMASK)
#define HW_ARC_PORTSC1_PFSC_SETMASK (0x01000000)

#define HW_ARC_PORTSC1_PSPD_SETMASK (3<<HW_ARC_PORTSC1_PSPD_BITPOS)
#define HW_ARC_PORTSC1_PTW_SETMASK (1<<HW_ARC_PORTSC1_PTW_BITPOS)  
#define HW_ARC_PORTSC1_STS_SETMASK (1<<HW_ARC_PORTSC1_STS_BITPOS)  
#define HW_ARC_PORTSC1_PTS_SETMASK (1<<HW_ARC_PORTSC1_PTS_BITPOS)  

#define HW_ARC_PORTSC1_CCS_CLRMASK (~(WORD)HW_ARC_PORTSC1_CCS_SETMASK)   
#define HW_ARC_PORTSC1_CSC_CLRMASK (~(WORD)HW_ARC_PORTSC1_CSC_SETMASK)   
#define HW_ARC_PORTSC1_PE_CLRMASK (~(WORD)HW_ARC_PORTSC1_PE_SETMASK)    
#define HW_ARC_PORTSC1_PEC_CLRMASK (~(WORD)HW_ARC_PORTSC1_PEC_SETMASK)   
#define HW_ARC_PORTSC1_OCA_CLRMASK (~(WORD)HW_ARC_PORTSC1_OCA_SETMASK)   
#define HW_ARC_PORTSC1_OCC_CLRMASK (~(WORD)HW_ARC_PORTSC1_OCC_SETMASK)   
#define HW_ARC_PORTSC1_FPR_CLRMASK (~(WORD)HW_ARC_PORTSC1_FPR_SETMASK)   
#define HW_ARC_PORTSC1_SUSP_CLRMASK (~(WORD)HW_ARC_PORTSC1_SUSP_SETMASK)
#define HW_ARC_PORTSC1_PR_CLRMASK (~(WORD)HW_ARC_PORTSC1_PR_SETMASK)    
#define HW_ARC_PORTSC1_HSP_CLRMASK (~(WORD)HW_ARC_PORTSC1_HSP_SETMASK)   
#define HW_ARC_PORTSC1_LS_CLRMASK (~(WORD)HW_ARC_PORTSC1_LS_SETMASK)    
#define HW_ARC_PORTSC1_PP_CLRMASK (~(WORD)HW_ARC_PORTSC1_PP_SETMASK)    
#define HW_ARC_PORTSC1_PO_CLRMASK (~(WORD)HW_ARC_PORTSC1_PO_SETMASK)    
#define HW_ARC_PORTSC1_PIC_CLRMASK (~(WORD)HW_ARC_PORTSC1_PIC_SETMASK)   
#define HW_ARC_PORTSC1_PTC_CLRMASK (~(WORD)HW_ARC_PORTSC1_PTC_SETMASK)   
#define HW_ARC_PORTSC1_WKCN_CLRMASK (~(WORD)HW_ARC_PORTSC1_WKCN_SETMASK)
#define HW_ARC_PORTSC1_WKDS_CLRMASK (~(WORD)HW_ARC_PORTSC1_WKDS_SETMASK)
#define HW_ARC_PORTSC1_WKOC_CLRMASK (~(WORD)HW_ARC_PORTSC1_WKOC_SETMASK)
#define HW_ARC_PORTSC1_PHCD_CLRMASK (~(WORD)HW_ARC_PORTSC1_PHCD_SETMASK)
#define HW_ARC_PORTSC1_PFSC_CLRMASK (~(WORD)HW_ARC_PORTSC1_PFSC_SETMASK)
#define HW_ARC_PORTSC1_PSPD_CLRMASK (~(WORD)HW_ARC_PORTSC1_PSPD_SETMASK)
#define HW_ARC_PORTSC1_PTW_CLRMASK (~(WORD)HW_ARC_PORTSC1_PTW_SETMASK)   
#define HW_ARC_PORTSC1_STS_CLRMASK (~(WORD)HW_ARC_PORTSC1_STS_SETMASK)   
#define HW_ARC_PORTSC1_PTS_CLRMASK (~(WORD)HW_ARC_PORTSC1_PTS_SETMASK)   

typedef union               
{
    struct {
        unsigned int CCS             :1;
        unsigned int CSC             :1;
        unsigned int PE              :1;
        unsigned int PEC             :1;
        unsigned int OCA             :1;
        unsigned int OCC             :1;
        unsigned int FPR             :1;
        unsigned int SUSP            :1;
        unsigned int PR              :1;
        unsigned int HSP             :1;
        unsigned int LS              :2;
        unsigned int PP              :1;
        unsigned int PO              :1;
        unsigned int PIC             :2;
        unsigned int PTC             :4;
        unsigned int WKCN            :1;
        unsigned int WKDS            :1;
        unsigned int WKOC            :1;
        unsigned int PHCD            :1;
        unsigned int PFSC            :1;
        unsigned int                 :1;
        unsigned int PSPD            :2;
        unsigned int                 :1;
        unsigned int PTW             :1;
        unsigned int STS             :1;
        unsigned int PTS             :1;
    }; 
    unsigned int u32;
} portsc1_type;
#define HW_ARC_PORTSC1 (*(volatile portsc1_type *) (HW_ARC_PORTSC1_ADDR))    




/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register On the Go status and control register(HW_ARC_OTGSC_ADDR)
typedef union               
{
    struct {
        unsigned int VD                 :1;
        unsigned int VC                 :1;
        unsigned int HAAR               :1;
        unsigned int OT                 :1;
        unsigned int DP                 :1;
        unsigned int IDPU               :1;
        unsigned int HADP               :1;
        unsigned int HABA               :1;
        unsigned int ID                 :1;
        unsigned int AVV                :1;
        unsigned int ASV                :1;
        unsigned int BSV                :1;
        unsigned int BSE                :1;
        unsigned int _1msT              :1;
        unsigned int DPS                :1;
        unsigned int Rsvd               :1;
        unsigned int IDIS               :1;
        unsigned int AVVIS              :1;
        unsigned int ASVIS              :1;
        unsigned int BSVIS              :1;
        unsigned int BSEIS              :1;
        unsigned int _1msS              :1;
        unsigned int DPIS               :1;
        unsigned int Rsvd2              :1;
        unsigned int IDIE               :1;
        unsigned int AVVIE              :1;
        unsigned int ASVIE              :1;
        unsigned int BSVIE              :1;
        unsigned int BSEIE              :1;
        unsigned int _1msE              :1;
        unsigned int DPIE               :1;
        unsigned int Rsvd3              :1;
    }; 
    unsigned int u32;
} otgsc_type;
#define HW_ARC_OTGSC (*(volatile otgsc_type *) (HW_ARC_OTGSC_ADDR))    

/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register USB Device Mode (HW_ARC_USBMODE)

#define HW_ARC_USBMODE_CM_BITPOS (0)
#define HW_ARC_USBMODE_ES_BITPOS (2)

#define USBMODE_IDLE        0x0
#define USBMODE_RESERVED    0x1
#define USBMODE_DEVICE      0x2
#define USBMODE_HOST        0x3

#define HW_ARC_USBMODE_CM_SETMASK (3<<HW_ARC_USBMODE_CM_BITPOS)   
#define HW_ARC_USBMODE_ES_SETMASK (1<<HW_ARC_USBMODE_ES_BITPOS)   

#define HW_ARC_USBMODE_CM_CLRMASK (~(WORD)HW_ARC_USBMODE_CM_SETMASK) 
#define HW_ARC_USBMODE_ES_CLRMASK (~(WORD)HW_ARC_USBMODE_ES_SETMASK)    

typedef union               
{
    struct {
        unsigned int CM              :2;
        unsigned int ES              :1;
        unsigned int                 :29;  
    }; 
    unsigned int u32;
} usbmode_type;
#define HW_ARC_USBMODE (*(volatile usbmode_type *) (HW_ARC_USBMODE_ADDR))    

/////////////////////////////////////////////////////////////////////////////////
//  The following endpoint equates are common for the following registers

#define ENDPOINT0_BITPOS (0)
#define ENDPOINT1_BITPOS (1)
#define ENDPOINT2_BITPOS (2)
#define ENDPOINT3_BITPOS (3)
#define ENDPOINT4_BITPOS (4)
#define ENDPOINT5_BITPOS (5)
#define ENDPOINT6_BITPOS (6)
#define ENDPOINT7_BITPOS (7)
#define ENDPOINT8_BITPOS (8)
#define ENDPOINT9_BITPOS (9)
#define ENDPOINT10_BITPOS (10)
#define ENDPOINT11_BITPOS (11)
#define ENDPOINT12_BITPOS (12)
#define ENDPOINT13_BITPOS (13)
#define ENDPOINT14_BITPOS (14)
#define ENDPOINT15_BITPOS (15)

#define ENDPOINT0_SETMASK (1<<ENDPOINT0_BITPOS)
#define ENDPOINT1_SETMASK (1<<ENDPOINT1_BITPOS)
#define ENDPOINT2_SETMASK (1<<ENDPOINT2_BITPOS)
#define ENDPOINT3_SETMASK (1<<ENDPOINT3_BITPOS)
#define ENDPOINT4_SETMASK (1<<ENDPOINT4_BITPOS)
#define ENDPOINT5_SETMASK (1<<ENDPOINT5_BITPOS)
#define ENDPOINT6_SETMASK (1<<ENDPOINT6_BITPOS)
#define ENDPOINT7_SETMASK (1<<ENDPOINT7_BITPOS)
#define ENDPOINT8_SETMASK (1<<ENDPOINT8_BITPOS)
#define ENDPOINT9_SETMASK (1<<ENDPOINT9_BITPOS)
#define ENDPOINT10_SETMASK (1<<ENDPOINT10_BITPOS)
#define ENDPOINT11_SETMASK (1<<ENDPOINT11_BITPOS)
#define ENDPOINT12_SETMASK (1<<ENDPOINT12_BITPOS)
#define ENDPOINT13_SETMASK (1<<ENDPOINT13_BITPOS)
#define ENDPOINT14_SETMASK (1<<ENDPOINT14_BITPOS)
#define ENDPOINT15_SETMASK (1<<ENDPOINT15_BITPOS)

#define ENDPOINT0_CLRMASK (~(WORD)ENDPOINT0_SETMASK)    
#define ENDPOINT1_CLRMASK (~(WORD)ENDPOINT1_SETMASK)    
#define ENDPOINT2_CLRMASK (~(WORD)ENDPOINT2_SETMASK)    
#define ENDPOINT3_CLRMASK (~(WORD)ENDPOINT3_SETMASK)    
#define ENDPOINT4_CLRMASK (~(WORD)ENDPOINT4_SETMASK)    
#define ENDPOINT5_CLRMASK (~(WORD)ENDPOINT5_SETMASK)    
#define ENDPOINT6_CLRMASK (~(WORD)ENDPOINT6_SETMASK)    
#define ENDPOINT7_CLRMASK (~(WORD)ENDPOINT7_SETMASK)    
#define ENDPOINT8_CLRMASK (~(WORD)ENDPOINT8_SETMASK)    
#define ENDPOINT9_CLRMASK (~(WORD)ENDPOINT9_SETMASK)    
#define ENDPOINT10_CLRMASK (~(WORD)ENDPOINT10_SETMASK)
#define ENDPOINT11_CLRMASK (~(WORD)ENDPOINT11_SETMASK)
#define ENDPOINT12_CLRMASK (~(WORD)ENDPOINT12_SETMASK)
#define ENDPOINT13_CLRMASK (~(WORD)ENDPOINT13_SETMASK)
#define ENDPOINT14_CLRMASK (~(WORD)ENDPOINT14_SETMASK)
#define ENDPOINT15_CLRMASK (~(WORD)ENDPOINT15_SETMASK)

typedef union               
{
    struct {
        unsigned int EP0              :1;
        unsigned int EP1              :1;
        unsigned int EP2              :1;
        unsigned int EP3              :1;
        unsigned int EP4              :1;
        unsigned int EP5              :1;
        unsigned int EP6              :1;
        unsigned int EP7              :1;
        unsigned int EP8              :1;
        unsigned int EP9              :1;
        unsigned int EP10             :1;
        unsigned int EP11             :1;
        unsigned int EP12             :1;
        unsigned int EP13             :1;
        unsigned int EP14             :1;
        unsigned int EP15             :1;
        unsigned int                  :16;
    }; 
    unsigned short u16[2];
    unsigned int u32;
} endpsetupstat_type;

#define HW_ARC_ENDPTSETUPSTAT (*(volatile endpsetupstat_type *) (HW_ARC_ENDPTSETUPSTAT_ADDR))    

typedef union               
{
    struct {
        unsigned int EP0              :1;
        unsigned int EP1              :1;
        unsigned int EP2              :1;
        unsigned int EP3              :1;
        unsigned int EP4              :1;
        unsigned int EP5              :1;
        unsigned int EP6              :1;
        unsigned int EP7              :1;
        unsigned int EP8              :1;
        unsigned int EP9              :1;
        unsigned int EP10             :1;
        unsigned int EP11             :1;
        unsigned int EP12             :1;
        unsigned int EP13             :1;
        unsigned int EP14             :1;
        unsigned int EP15             :1;
    }; 
    unsigned int u16;
} endpt_type;

typedef union
{
   struct {
       endpt_type  RX;
       endpt_type  TX;
    }; 
    endpt_type     RXTX[2];//this might not work
    unsigned int u32;
} endptrxtx_type;

#define HW_ARC_ENDPTPRIME    (*(volatile endptrxtx_type *) (HW_ARC_ENDPTPRIME_ADDR))    
#define HW_ARC_ENDPTFLUSH    (*(volatile endptrxtx_type *) (HW_ARC_ENDPTFLUSH_ADDR))    
#define HW_ARC_ENDPTSTATUS   (*(volatile endptrxtx_type *) (HW_ARC_ENDPTSTATUS_ADDR))    
#define HW_ARC_ENDPTCOMPLETE (*(volatile endptrxtx_type *) (HW_ARC_ENDPTCOMPLETE_ADDR))    



/////////////////////////////////////////////////////////////////////////////////
//  USB ARC Register Endpoint control (HW_ARC_ENDPTCTRL)

#define HW_ARC_ENDPTCTRL_RXS_BITPOS (0)
#define HW_ARC_ENDPTCTRL_RXD_BITPOS (1)
#define HW_ARC_ENDPTCTRL_RXT_BITPOS (2)
#define HW_ARC_ENDPTCTRL_RXI_BITPOS (5)
#define HW_ARC_ENDPTCTRL_RXR_BITPOS (6)
#define HW_ARC_ENDPTCTRL_RXE_BITPOS (7)
#define HW_ARC_ENDPTCTRL_TXS_BITPOS (16)
#define HW_ARC_ENDPTCTRL_TXD_BITPOS (17)
#define HW_ARC_ENDPTCTRL_TXT_BITPOS (18)
#define HW_ARC_ENDPTCTRL_TXI_BITPOS (21)
#define HW_ARC_ENDPTCTRL_TXR_BITPOS (22)
#define HW_ARC_ENDPTCTRL_TXE_BITPOS (23)

#define HW_ARC_ENDPTCTRL_RXS_SETMASK (1<<HW_ARC_ENDPTCTRL_RXS_BITPOS)
#define HW_ARC_ENDPTCTRL_RXD_SETMASK (1<<HW_ARC_ENDPTCTRL_RXD_BITPOS)
#define HW_ARC_ENDPTCTRL_RXT_SETMASK (3<<HW_ARC_ENDPTCTRL_RXT_BITPOS)
#define HW_ARC_ENDPTCTRL_RXI_SETMASK (1<<HW_ARC_ENDPTCTRL_RXI_BITPOS)
#define HW_ARC_ENDPTCTRL_RXR_SETMASK (1<<HW_ARC_ENDPTCTRL_RXR_BITPOS)
#define HW_ARC_ENDPTCTRL_RXE_SETMASK (1<<HW_ARC_ENDPTCTRL_RXE_BITPOS)
#define HW_ARC_ENDPTCTRL_TXS_SETMASK (1<<HW_ARC_ENDPTCTRL_TXS_BITPOS)
#define HW_ARC_ENDPTCTRL_TXD_SETMASK (1<<HW_ARC_ENDPTCTRL_TXD_BITPOS)
#define HW_ARC_ENDPTCTRL_TXT_SETMASK (3<<HW_ARC_ENDPTCTRL_TXT_BITPOS)
#define HW_ARC_ENDPTCTRL_TXI_SETMASK (1<<HW_ARC_ENDPTCTRL_TXI_BITPOS)
#define HW_ARC_ENDPTCTRL_TXR_SETMASK (1<<HW_ARC_ENDPTCTRL_TXR_BITPOS)

// We need to equate the following label like this due to a sign extension problem
// if equated like so (1<<HW_ARC_ENDPTCTRL_TXE_BITPOS)
#define HW_ARC_ENDPTCTRL_TXE_SETMASK (0x00800000)
//HW_ARC_ENDPTCTRL_TXE_SETMASK    equ     (1<<HW_ARC_ENDPTCTRL_TXE_BITPOS)

#define HW_ARC_ENDPTCTRL_RXS_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_RXS_SETMASK)
#define HW_ARC_ENDPTCTRL_RXD_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_RXD_SETMASK)
#define HW_ARC_ENDPTCTRL_RXT_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_RXT_SETMASK)
#define HW_ARC_ENDPTCTRL_RXI_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_RXI_SETMASK)
#define HW_ARC_ENDPTCTRL_RXR_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_RXR_SETMASK)
#define HW_ARC_ENDPTCTRL_RXE_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_RXE_SETMASK)
#define HW_ARC_ENDPTCTRL_TXS_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_TXS_SETMASK)
#define HW_ARC_ENDPTCTRL_TXD_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_TXD_SETMASK)
#define HW_ARC_ENDPTCTRL_TXT_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_TXT_SETMASK)
#define HW_ARC_ENDPTCTRL_TXI_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_TXI_SETMASK)
#define HW_ARC_ENDPTCTRL_TXR_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_TXR_SETMASK)
#define HW_ARC_ENDPTCTRL_TXE_CLRMASK (~(WORD)HW_ARC_ENDPTCTRL_TXE_SETMASK)


typedef union               
{
    struct {
        unsigned int RXS             :1;
        unsigned int RXD             :1;
        unsigned int RXT             :2;
        unsigned int                 :1;
        unsigned int RXI             :1;
        unsigned int RXR             :1;
        unsigned int RXE             :1;
        unsigned int                 :8;
        unsigned int TXS             :1;
        unsigned int TXD             :1;
        unsigned int TXT             :2;
        unsigned int                 :1;
        unsigned int TXI             :1;
        unsigned int TXR             :1;
        unsigned int TXE             :1;
        unsigned int                 :8;
    }; 
    unsigned int u32;
} endptctrl_type;

#define HW_ARC_ENDPTCTRL ((volatile endptctrl_type *) (HW_ARC_ENDPTCTRL0_ADDR))    

#pragma pack()

#endif // regsusb20inc_h_
