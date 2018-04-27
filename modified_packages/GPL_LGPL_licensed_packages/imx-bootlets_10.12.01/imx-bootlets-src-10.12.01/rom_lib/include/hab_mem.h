/* HAB memory on OCRAM */
extern unsigned char __linker_sym_hab_persist_start[];
extern unsigned char __linker_sym_hab_persist_end[];
extern unsigned char __linker_sym_hab_dma_start[];
extern unsigned char __linker_sym_hab_dma_end[];
extern unsigned char __linker_sym_hab_scratch_start[];
extern unsigned char __linker_sym_hab_scratch_end[];

#define HAB_HAL_PERSISTANT_START      ((int)__linker_sym_hab_persist_start)
#define HAB_HAL_PERSISTANT_END        ((int)__linker_sym_hab_persist_end)
#define HAB_HAL_DMA_START             ((int)__linker_sym_hab_dma_start)
#define HAB_HAL_DMA_END               ((int)__linker_sym_hab_dma_end)
#define HAB_HAL_SCRATCH_START         ((int)__linker_sym_hab_scratch_start)
#define HAB_HAL_SCRATCH_END           ((int)__linker_sym_hab_scratch_end)
