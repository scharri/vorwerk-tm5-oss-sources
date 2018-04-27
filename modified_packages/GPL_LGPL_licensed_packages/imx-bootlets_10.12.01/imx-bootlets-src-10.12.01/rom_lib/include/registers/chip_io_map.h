//
//  DO NOT MANUAL EDIT THIS FILE!
//  generated from 'chip/integration/sysdef.xls' by 'chip/integration/sysdef/sysdef.pl'
//

#define              REGS_USBCTRL0_BASE (           REGS_BASE + 0x00080000)
#define              REGS_USBCTRL0_END  (  REGS_USBCTRL0_BASE + 0x0000FFFF)
#define              REGS_USBCTRL1_BASE (           REGS_BASE + 0x00090000)
#define              REGS_USBCTRL1_END  (  REGS_USBCTRL1_BASE + 0x0000FFFF)
#define                 REGS_DFLPT_BASE (           REGS_BASE + 0x000C0000)
#define                 REGS_DFLPT_END  (     REGS_DFLPT_BASE + 0x0000FFFF)
#define                  REGS_DRAM_BASE (           REGS_BASE + 0x000E0000)
#define                  REGS_DRAM_END  (      REGS_DRAM_BASE + 0x0000FFFF)
#define                  REGS_ENET_BASE (           REGS_BASE + 0x000F0000)
#define                  REGS_ENET_END  (      REGS_ENET_BASE + 0x0000FFFF)
#define                 REGS_ICOLL_BASE (           REGS_BASE + 0x00000000)
#define                 REGS_ICOLL_END  (     REGS_ICOLL_BASE + 0x00001FFF)
#define                 REGS_HSADC_BASE (           REGS_BASE + 0x00002000)
#define                 REGS_HSADC_END  (     REGS_HSADC_BASE + 0x00001FFF)
#define                  REGS_APBH_BASE (           REGS_BASE + 0x00004000)
#define                  REGS_APBH_END  (      REGS_APBH_BASE + 0x00001FFF)
#define               REGS_PERFMON_BASE (           REGS_BASE + 0x00006000)
#define               REGS_PERFMON_END  (   REGS_PERFMON_BASE + 0x000007FF)
#define                   REGS_BCH_BASE (           REGS_BASE + 0x0000A000)
#define                   REGS_BCH_END  (       REGS_BCH_BASE + 0x00001FFF)
#define                  REGS_GPMI_BASE (           REGS_BASE + 0x0000C000)
#define                  REGS_GPMI_END  (      REGS_GPMI_BASE + 0x00001FFF)
#define                  REGS_SSP0_BASE (           REGS_BASE + 0x00010000)
#define                  REGS_SSP0_END  (      REGS_SSP0_BASE + 0x00001FFF)
#define                  REGS_SSP1_BASE (           REGS_BASE + 0x00012000)
#define                  REGS_SSP1_END  (      REGS_SSP1_BASE + 0x00001FFF)
#define                  REGS_SSP2_BASE (           REGS_BASE + 0x00014000)
#define                  REGS_SSP2_END  (      REGS_SSP2_BASE + 0x00001FFF)
#define                  REGS_SSP3_BASE (           REGS_BASE + 0x00016000)
#define                  REGS_SSP3_END  (      REGS_SSP3_BASE + 0x00001FFF)
#define               REGS_PINCTRL_BASE (           REGS_BASE + 0x00018000)
#define               REGS_PINCTRL_END  (   REGS_PINCTRL_BASE + 0x00001FFF)
#define                REGS_DIGCTL_BASE (           REGS_BASE + 0x0001C000)
#define                REGS_DIGCTL_END  (    REGS_DIGCTL_BASE + 0x00001FFF)
#define                   REGS_ETM_BASE (           REGS_BASE + 0x00022000)
#define                   REGS_ETM_END  (       REGS_ETM_BASE + 0x00001FFF)
#define                  REGS_APBX_BASE (           REGS_BASE + 0x00024000)
#define                  REGS_APBX_END  (      REGS_APBX_BASE + 0x00001FFF)
#define                   REGS_DCP_BASE (           REGS_BASE + 0x00028000)
#define                   REGS_DCP_END  (       REGS_DCP_BASE + 0x00001FFF)
#define                   REGS_PXP_BASE (           REGS_BASE + 0x0002A000)
#define                   REGS_PXP_END  (       REGS_PXP_BASE + 0x00001FFF)
#define                 REGS_OCOTP_BASE (           REGS_BASE + 0x0002C000)
#define                 REGS_OCOTP_END  (     REGS_OCOTP_BASE + 0x00001FFF)
#define              REGS_AXI_AHB0_BASE (           REGS_BASE + 0x0002E000)
#define              REGS_AXI_AHB0_END  (  REGS_AXI_AHB0_BASE + 0x00001FFF)
#define                 REGS_LCDIF_BASE (           REGS_BASE + 0x00030000)
#define                 REGS_LCDIF_END  (     REGS_LCDIF_BASE + 0x00001FFF)
#define                  REGS_CAN0_BASE (           REGS_BASE + 0x00032000)
#define                  REGS_CAN0_END  (      REGS_CAN0_BASE + 0x00001FFF)
#define                  REGS_CAN1_BASE (           REGS_BASE + 0x00034000)
#define                  REGS_CAN1_END  (      REGS_CAN1_BASE + 0x00001FFF)
#define                REGS_SIMDBG_BASE (           REGS_BASE + 0x0003C000)
#define                REGS_SIMDBG_END  (    REGS_SIMDBG_BASE + 0x000001FF)
#define            REGS_SIMGPMISEL_BASE (           REGS_BASE + 0x0003C200)
#define            REGS_SIMGPMISEL_END  (REGS_SIMGPMISEL_BASE + 0x000000FF)
#define             REGS_SIMSSPSEL_BASE (           REGS_BASE + 0x0003C300)
#define             REGS_SIMSSPSEL_END  ( REGS_SIMSSPSEL_BASE + 0x000000FF)
#define             REGS_SIMMEMSEL_BASE (           REGS_BASE + 0x0003C400)
#define             REGS_SIMMEMSEL_END  ( REGS_SIMMEMSEL_BASE + 0x000000FF)
#define               REGS_GPIOMON_BASE (           REGS_BASE + 0x0003C500)
#define               REGS_GPIOMON_END  (   REGS_GPIOMON_BASE + 0x000000FF)
#define               REGS_SIMENET_BASE (           REGS_BASE + 0x0003C700)
#define               REGS_SIMENET_END  (   REGS_SIMENET_BASE + 0x000000FF)
#define               REGS_ARMJTAG_BASE (           REGS_BASE + 0x0003C800)
#define               REGS_ARMJTAG_END  (   REGS_ARMJTAG_BASE + 0x000000FF)
#define               REGS_CLKCTRL_BASE (           REGS_BASE + 0x00040000)
#define               REGS_CLKCTRL_END  (   REGS_CLKCTRL_BASE + 0x00001FFF)
#define                 REGS_SAIF0_BASE (           REGS_BASE + 0x00042000)
#define                 REGS_SAIF0_END  (     REGS_SAIF0_BASE + 0x00001FFF)
#define                 REGS_POWER_BASE (           REGS_BASE + 0x00044000)
#define                 REGS_POWER_END  (     REGS_POWER_BASE + 0x00001FFF)
#define                 REGS_SAIF1_BASE (           REGS_BASE + 0x00046000)
#define                 REGS_SAIF1_END  (     REGS_SAIF1_BASE + 0x00001FFF)
#define                 REGS_LRADC_BASE (           REGS_BASE + 0x00050000)
#define                 REGS_LRADC_END  (     REGS_LRADC_BASE + 0x00001FFF)
#define                 REGS_SPDIF_BASE (           REGS_BASE + 0x00054000)
#define                 REGS_SPDIF_END  (     REGS_SPDIF_BASE + 0x00001FFF)
#define                   REGS_RTC_BASE (           REGS_BASE + 0x00056000)
#define                   REGS_RTC_END  (       REGS_RTC_BASE + 0x00001FFF)
#define                  REGS_I2C0_BASE (           REGS_BASE + 0x00058000)
#define                  REGS_I2C0_END  (      REGS_I2C0_BASE + 0x00001FFF)
#define                  REGS_I2C1_BASE (           REGS_BASE + 0x0005A000)
#define                  REGS_I2C1_END  (      REGS_I2C1_BASE + 0x00001FFF)
#define                   REGS_PWM_BASE (           REGS_BASE + 0x00064000)
#define                   REGS_PWM_END  (       REGS_PWM_BASE + 0x00001FFF)
#define                REGS_TIMROT_BASE (           REGS_BASE + 0x00068000)
#define                REGS_TIMROT_END  (    REGS_TIMROT_BASE + 0x00001FFF)
#define              REGS_UARTAPP0_BASE (           REGS_BASE + 0x0006A000)
#define              REGS_UARTAPP0_END  (  REGS_UARTAPP0_BASE + 0x00001FFF)
#define              REGS_UARTAPP1_BASE (           REGS_BASE + 0x0006C000)
#define              REGS_UARTAPP1_END  (  REGS_UARTAPP1_BASE + 0x00001FFF)
#define              REGS_UARTAPP2_BASE (           REGS_BASE + 0x0006E000)
#define              REGS_UARTAPP2_END  (  REGS_UARTAPP2_BASE + 0x00001FFF)
#define              REGS_UARTAPP3_BASE (           REGS_BASE + 0x00070000)
#define              REGS_UARTAPP3_END  (  REGS_UARTAPP3_BASE + 0x00001FFF)
#define              REGS_UARTAPP4_BASE (           REGS_BASE + 0x00072000)
#define              REGS_UARTAPP4_END  (  REGS_UARTAPP4_BASE + 0x00001FFF)
#define               REGS_UARTDBG_BASE (           REGS_BASE + 0x00074000)
#define               REGS_UARTDBG_END  (   REGS_UARTDBG_BASE + 0x00001FFF)
#define               REGS_USBPHY0_BASE (           REGS_BASE + 0x0007C000)
#define               REGS_USBPHY0_END  (   REGS_USBPHY0_BASE + 0x00001FFF)
#define               REGS_USBPHY1_BASE (           REGS_BASE + 0x0007E000)
#define               REGS_USBPHY1_END  (   REGS_USBPHY1_BASE + 0x00001FFF)