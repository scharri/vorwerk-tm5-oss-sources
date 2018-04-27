#ifndef HAB_HAL_H
#define HAB_HAL_H
/*===========================================================================*/
/**
    @file    hab_hal.h

    @brief   HAB hardware abstraction layer


@verbatim
=============================================================================

              Freescale Semiconductor Confidential Proprietary
        (c) Freescale Semiconductor, Inc. 2007, 2008, 2009. All rights reserved.

Presence of a copyright notice is not an acknowledgement of
publication. This software file listing contains information of
Freescale Semiconductor, Inc. that is of a confidential and
proprietary nature and any viewing or use of this file is prohibited
without specific written permission from Freescale Semiconductor, Inc.

=============================================================================
Revision History:

                   Modification Date   Tracking
Author               (dd-mmm-yyyy)      Number    Description of Changes
---------------      -------------    ----------  -----------------------
David Hartley        11-Dec-2007      ENGR55511   Initial version
David Hartley        29-Jul-2008      ENGR82581   Add CMS, X.509v3, SHA-512
                                                  and ECDSA over P-521
David Hartley        26-Aug-2008      ENGR88931   Restore PKCS#1 & SHA-256
                                                  Suppress ECDSA & SHA-512
David Hartley        07-Nov-2008      ENGR92335   Implementation updates
                                                  Restore SHA-1, suppress
                                                  SHA-256
Ram Korukonda        05-Mar-2009      ENGR106041  Modified the boot_cfg enum
                                                  and added sw_state and sw_fp
                                                  plugins
Srinivasa Rao Uppala 30-Mar-2009      ENGR109813  Modified DCD commands
                                                  to use HAL functions 
David Hartley        28-Apr-2009      ENGR111969  Adjust storage areas
Rod Ziolkowski       07-Jul-2009      ENGR113882  Restore SW SHA-256 plugin
David Hartley        27-Jul-2009      ENGR113884  Add DCP support
David Hartley        04-Aug-2009      ENGR111476  Byte array conversion
David Hartley        27-Aug-2009      ENGR113895  Make mandatory signature
                                                  binding configurable
David Hartley        02-Sep-2009      ENGR116107  Add support for SCCv2
Rod Ziolkowski                        ENGR116108  Add support for RTICv3
                                      ENGR116110  Add support for SAHARAv4LT
                                      ENGR116111  Add support for SRTC
                                      ENGR116112  Add support for CSU
David Hartley        11-Nov-2009      ENGR117777  Support 128-bit security
                                         
=============================================================================
Portability: 

These definitions are customised for 32 bit cores of either endianness.

=============================================================================
@endverbatim */

/*===========================================================================
                            INCLUDE FILES
=============================================================================*/

#include "hab_types.h"          /* For types, constants, macros */

/** @cond rom */

/** @addtogroup hal
 *  @{
 */

/*===========================================================================
                              CONSTANTS
=============================================================================*/

/** @internal Width of field denoting bit within fuse row */
#define HAB_HAL_BIT_WIDTH 3
/** @internal Mask for field denoting bit within fuse row
 *@hideinitializer
 */
#define HAB_HAL_BIT_MASK \
    (((uint32_t)1 << HAB_HAL_BIT_WIDTH) - 1)

/** Minimal allowed size for persistent storage */
#define HAB_HAL_PERSISTENT_MIN_BYTES ((size_t)2304)

/** Minimal allowed size for scratch storage */
#define HAB_HAL_SCRATCH_MIN_BYTES ((size_t)5632)

/** Minimal allowed size for DMA storage */
#define HAB_HAL_DMA_MIN_BYTES ((size_t)256)

/** @rom DMA storage requirement
 * @ingroup dcp
 *
 * This figure is derived in two parts:
 * - each data block needs an 8-word work packet (descriptor)
 * - at least 40 bytes are required for SHA-256 result and memory manager
 * overhead: 64 bytes allows some small overhead.
 */
#define HAB_DCP_DMA_MIN_BYTES (64 + HAB_DCP_BLOCK_MAX * 32)

/** @rom DMA storage requirement
 * @ingroup scc
 *
 * This figure is derived in several stages, and assumes plaintext and
 * ciphertext buffers are both allocated in the DMA region :
 * - 4 blocks of plaintext required
 * - 4 blocks of ciphertext required
 * - each block is 16 bytes long
 * - the plaintext address must be block-aligned (up to 15 bytes overhead)
 * - the ciphertext address must be block-aligned (up to 3 bytes overhead)
 * - at least 8 bytes of memory manager overhead: allow 32 for comfort
 */
#define HAB_SCC_DMA_MIN_BYTES ( (4+4)*16 + 15 + 3 + 32)

/** @rom DMA storage requirement
 * @ingroup sah
 *
 * This figure is derived in several parts:
 * - each hash operation needs a 6-word descriptor structure
 * - each data block needs a 3-word link structure
 * - the result needs a 3-word link structure
 * - at least 40 bytes are required for SHA-256 result and memory manager
 * overhead: 64 bytes allows some small overhead.
 */
#define HAB_SAHARA_DMA_MIN_BYTES (24 + HAB_SAHARA_BLOCK_MAX * 12 + 12 + 64)

/*===========================================================================
                                MACROS
=============================================================================*/

/** @rom Plugin table entry.
 *@hideinitializer
 *
 * This macro defines an entry in the plugin table. It is intended for
 * initialising #hab_hal_plugin.
 *
 * @param[in] plugin The plugin to be included in the build.
 *
 * @warning Do not use any @ref eng tag associated with the plugin; use the
 * name of the plugin as specified in the list of exported plugins.
 */
#define HAB_PLUGIN(plugin)                      \
    &HAB_PLUGIN_NAME(plugin)

/** @internal Export plugin.
 *@hideinitializer
 *
 * This macro declares plugin for possible inclusion in the ROM build.
 *
 * @param[in] plugin The plugin to be included in the build.
 *
 * @warning Do not use any @ref eng tag associated with the plugin; use the
 * name of the plugin as specified in the documentation for the required
 * command, protocol or @ref shw.  The @a plugin parameter must be valid as
 * part of a C variable name (no spaces etc.).
 */
#define HAB_EXPORT_PLUGIN(plugin)                               \
    extern const struct hab_plugin HAB_PLUGIN_NAME(plugin)

/** @internal Plugin internal name constructor.
 *@hideinitializer
 */
#define HAB_PLUGIN_NAME(p)                      \
    hab_plg_##p##_table

/** @rom Peripheral table entry
 *@hideinitializer
 *
 * This macro defines an entry in the peripheral table. It is intended for
 * initializing #hab_hal_peripheral.
 *
 * @param[in] base Base address of the writable peripheral register set.
 *
 * @param[in] last Last address of the writable peripheral register set
 * (inclusive).
 */
#define HAB_PERIPHERAL(base, last)              \
    { (base), (last) }

/** @rom Memory table entry
 *@hideinitializer
 *
 * This macro defines an entry in the memory table. It is intended for
 * initialising #hab_hal_memory.
 *
 * @param[in] base Base address of the memory region.
 *
 * @param[in] last Last address of the memory region (inclusive).
 */
#define HAB_MEMORY(base, last)                  \
    { (base), (last) }

/** @rom Detector table entry
 *@hideinitializer
 *
 * This macro defines an entry in an alarm detector table.  It is intended for
 * initialising #hab_hal_detector.
 *
 * @param[in] src The engine tag for the alarm source.
 *
 * @param[in] det The engine tag for the alarm detector.
 *
 * @param[in] idx The index of that alarm source within the alarm detector
 * registers.
 *
 * @remark To understand the mapping between the value entered in @a idx and
 * the bits in the alarm detector registers, see the documentation for the
 * alarm detector in the @ref shw section.
 *
 * @remark One alarm source may appear in entries for multiple detectors,
 * depending on the IC configuration.
 *
 */
#define HAB_DETECTOR_INDEX(src, det, idx)       \
    { (src), (det), (idx) }

/** @rom @name Enable fuse macros
 * 
 * @anchor enbl
 */
/*@{*/

/** @rom Engine always enabled (no fuse).
 * @hideinitializer
 *
 * This macro defines data to check if a SHW engine is enabled.  It is
 * intended to initialise fields such as #hab_hal_preconfigure.scc2_enabled
 * for use by hab_hal_ict.preconfigure().
 *
 * @return Engine enabled query data
 */
#define HAB_ENABLED_ALWAYS()                    \
    {NULL, 0, 0}

/** @rom Engine never enabled (no fuse).
 * @hideinitializer
 *
 * This macro defines data to check if a SHW engine is enabled.  It is
 * intended to initialise fields such as #hab_hal_preconfigure.scc2_enabled
 * for use by hab_hal_ict.preconfigure().
 *
 * @return Engine enabled query data
 */
#define HAB_ENABLED_NEVER()                     \
    {NULL, 0, ~0}

/** @rom Engine enabled when fuse blown.
 * @hideinitializer
 *
 * This macro defines data to check if a SHW engine is enabled.  It is
 * intended to initialise fields such as #hab_hal_preconfigure.scc2_enabled
 * for use by hab_hal_ict.preconfigure().
 *
 * @param[in] row_addr Address of enable fuse
 *
 * @param[in] bit Fuse bit for enable fuse
 *
 * @return Engine enabled query data
 */
#define HAB_ENABLED_WHEN(row_addr, bit)                                 \
    {(const uint32_t*)(row_addr), 1 << (bit), 1 << (bit)}

/** @rom Engine enabled unless fuse blown.
 * @hideinitializer
 *
 * This macro defines data to check if a SHW engine is enabled.  It is
 * intended to initialise fields such as #hab_hal_preconfigure.scc2_enabled
 * for use by hab_hal_ict.preconfigure().
 *
 * @param[in] row_addr Address of disable fuse
 *
 * @param[in] bit Fuse bit for disable fuse
 *
 * @return Engine enabled query data
 */
#define HAB_ENABLED_UNLESS(row_addr, bit)                               \
    {(const uint32_t*)(row_addr), 1 << (bit), 0}
/*@}*/

/** @internal Check engine enabled
 * @hideinitializer
 *
 * Used by hab_hal_ict.preconfigure() functions in conjunction with the data
 * set by ROM in #hab_hal_ict.preconfigure_data to determine whether certain
 * SHW engines are enabled.  Since there may be no stack available,
 * hab_hal_ict.preconfigure() is unable to use other HAL functions for this
 * purpose.
 *
 * @param[in] q Engine enabled query data of type #hab_hal_enbl_t
 *
 * @retval TRUE if SHW engine enabled
 * @retval FALSE otherwise
 */
#define HAB_HAL_ENABLED(q)                                      \
    ( (q).sense == ( (q).mask & ( (q).addr ? *(q).addr : 0 )))

/** @rom Fuse field size.
 * @hideinitializer
 *
 * This macro computes the size of the buffer required to hold a fuse field
 * according to the formula for the @ref bnd_mid.
 *
 * @param[in] bit position of first fuse (bit 7 is most-significant or
 * left-most bit).
 *
 * @param[in] fuses number of fuses in bitfield.
 *
 * @pre 0 <= @a bit < 8
 *
 * @pre 0 < @a fuses
 *
 * @return Required buffer size.
 */
#define HAB_HAL_FUSE_FIELD_BYTES(bit, fuses)                            \
    (1 + (((fuses) <= (bit) + 1) ? 0                                    \
          : (((fuses) - (bit) - 1 + HAB_HAL_BIT_MASK) >> HAB_HAL_BIT_WIDTH)))

/*===========================================================================
                                ENUMS
=============================================================================*/

/** @rom Boot configuration */
typedef enum hab_hal_boot_cfg { 
  HAB_HAL_BOOT_CFG_QUASIEXTERNAL = 0x00, /**< @rom Quasi-External boot  */
  HAB_HAL_BOOT_CFG_INTERNAL = 0x0f       /**< @rom Internal boot  */
} hab_hal_boot_cfg_t;

/** @rom Test mode configuration */
typedef enum hab_hal_test_cfg { 
  HAB_HAL_TEST_CFG_NONSEC = 0x00, /**< @rom Non-security test modes */
  HAB_HAL_TEST_CFG_KEYGEN = 0xcc, /**< @rom Key generation test mode */
  HAB_HAL_TEST_CFG_SECURE = 0xf0 /**< @rom Other security test modes */
} hab_hal_test_cfg_t;

/** @rom IC configuration flags */
typedef enum hab_hal_ic_flg { 
  HAB_HAL_IC_FLG_BND_ENFORCED = 0x01 /**< @rom Mandatory binding enforced */
} hab_hal_ic_flg_t;

/*===========================================================================
                    STRUCTURES AND OTHER TYPEDEFS
=============================================================================*/

/** @rom Memory region. */
typedef struct hab_hal_region {
    /** @rom Base address. */
    void* base;
    /** @rom Last address: inclusive */
    void* last;
} hab_hal_region_t;

/** @rom SHW enabled query */
typedef struct hab_hal_enbl {
    /** @rom Fuse row address: NULL if hard-wired */
    const uint32_t* addr;
    /** @rom Mask to select bit: 0 if hard-wired */
    uint8_t mask;
    /** @rom Sense: 0 if hard-wired enabled, ~0 if hard-wired disabled, 0 if
     * enabled unless fuse blown, match mask if enabled when fuse blown
     */
    uint8_t sense;
} hab_hal_enbl_t;

/** @rom Preconfiguration data */
typedef struct hab_hal_preconfigure {
    /** @rom Data to check if SCCv2 enabled: see @ref enbl "Enable fuse
     * macros" */
    hab_hal_enbl_t scc2_enabled;
    /** @rom Data to check if CSU enabled: see @ref enbl "Enable fuse
     * macros" */
    hab_hal_enbl_t csu_enabled;
    /** @rom CSU alarm route configuration to disable reset controller
     *  and SRTC alarm routes for all alarm inputs. */
    uint8_t alarm_route_dis;
    /** @rom First SCCv2 partition to allocate. */
    uint8_t scc2_start;
    /** @rom Number of SCCv2 partitions to allocate. */
    uint8_t scc2_count;
    /* @rom Reserved for padding */
    uint8_t reserved;
} hab_hal_preconfigure_t;

/** @rom IC configuration table structure */
typedef struct hab_hal_ict {
    /** @rom Security configuration prior to running crt0.
     * @ingroup pre_cfg
     *
     * @par Purpose
     *
     * This function handles @ref shw elements which must be configured
     * immediately after reset and before running the C run-time initialisation
     * functions in crt0.
     *
     * @par Operation
     *
     * Several different variants of this function are available, as described
     * in the @ref shw sections.  See the relevant variant for the
     * operations performed.
     *
     * All variants of this function initialise the audit log.
     *
     * @remark This function does not use the stack.
     *
     * @warning The ROM is reponsible for initialising this function pointer
     * with the correct preconfiguration function according to the plugins
     * loaded for each IC.
     *
     * @retval None
     */
    void (*preconfigure)(void);
    /** @rom Preconfiguration data interpreted by preconfigure(): the format
     * is specific to the particular variant of preconfigure() used. 
     */
    const void* preconfigure_data;
    /** @rom Region reserved for HAB data guaranteed not to be overwritten
     *  until no further HAB library calls will be made: must be at least
     *  #HAB_HAL_PERSISTENT_MIN_BYTES in size, word-aligned and
     *  byte-accessible for read & write.
     */
    hab_hal_region_t persistent;
    /** @rom Region reserved for HAB data which may be overwritten between HAB
     *  library calls: must be at least #HAB_HAL_SCRATCH_MIN_BYTES in size,
     *  word-aligned and byte-accessible for read & write.
     */
    hab_hal_region_t scratch;
    /** @rom Region accessible to @ref shw DMA engines which may be
     *  overwritten between HAB library calls: must be at least
     *  #HAB_HAL_DMA_MIN_BYTES in size, word-aligned and word-accessible for
     *  read and write (stack is used if this region is empty).
     */
    hab_hal_region_t dma;
    /** @rom IC configuration flags. */
    hab_hal_ic_flg_t flags;
    /** @rom Address to which result of fabrication test may be written. */
    uint32_t* fab_test_point;
} hab_hal_ict_t;

/** @internal Detector table entry */
typedef struct hab_hal_detector_index {
    uint8_t src;                /**< alarm source engine */
    uint8_t det;                /**< alarm detector engine */
    uint8_t idx;                /**< index within alert detector registers */
} hab_hal_detector_index_t;

/** @internal Plugin (declaration only) */
struct hab_plugin;

/*===========================================================================
                     GLOBAL VARIABLE DECLARATIONS
=============================================================================*/

/** @rom IC configuration table 
 *
 * This table is used by functions in the HAB library to determine IC-specific
 * addresses and other information.
 */
extern const hab_hal_ict_t hab_hal_ict;

/** @rom Plugin table.
 *
 * This table is used by HAB to find command handlers, protocol handlers,
 * algorithm engines and other @ref shw functionality.  This table also serves
 * to determine which HAB plugins are included in the ROM build.
 *
 * @remark Use HAB_PLUGIN() for the entries as in the following example:
@verbatim
        const struct hab_plugin* const hab_hal_plugin[] = {
                HAB_PLUGIN(cmd_base),
                ...
                HAB_PLUGIN(x509)
        };
@endverbatim
 */
extern const struct hab_plugin* const hab_hal_plugin[];

/** @rom Entries in plugin table: initialise with HAB_ENTRIES_IN() */
extern const uint32_t hab_hal_plugin_count;

/** @rom Peripheral table
 *
 * This is a table of properties for peripherals.  In this version of HAB, the
 * following properties are included:
 * - memory regions which are allowed by hab_rvt.check_target() using
 * #HAB_TGT_PERIPHERAL.
 *
 * @remark Use HAB_PERIPHERAL() for the entries as in the following example:
@verbatim
        const hab_hal_region_t hab_hal_peripheral[] = {
                // All registers in PLL
                HAB_PERIPHERAL(pll_base, pll_last),
                ...
                // Command, Control, DMA Throttle & WDOG registers in RTIC
                HAB_PERIPHERAL(rtic_base + RTIC_CMD_OFFSET, 
                               rtic_base + RTIC_DMA_OFFSET + 3),
                HAB_PERIPHERAL(rtic_base + RTIC_WDOG_OFFSET, 
                               rtic_base + RTIC_WDOG_OFFSET + 3)
        };
@endverbatim
 *
 * @remark Register addresses in the peripheral table are usable for @ref dcd
 * commands.
 *
 * @remark It is possible to enable access to only selected registers within a
 * peripheral as illustrated by the RTIC register examples above.
 */
extern const hab_hal_region_t hab_hal_peripheral[];

/** @rom Entries in peripheral table: initialise with HAB_ENTRIES_IN() */
extern const uint32_t hab_hal_peripheral_count;

/** @rom Memory table
 *
 * This is a table of properties for memory regions.  In this
 * version of HAB, the following properties are included:
 * - memory regions which are allowed by hab_rvt.check_target() using
 * #HAB_TGT_MEMORY.
 *
 * @remark Use
 * HAB_MEMORY() for the entries as in the following example:
@verbatim
        const hab_hal_region_t hab_hal_memory[] = {
                HAB_MEMORY(cs0_base, cs0_last),
                ...
                HAB_MEMORY(csd1_base, csd1_last)
        };
@endverbatim
 *
 * @warning A single memory table is required in the boot ROM.
 *
 * @remark Memory regions in the memory table are usable for @ref dcd
 * commands.
 */
extern const hab_hal_region_t hab_hal_memory[];

/** @rom Entries in memory table: initialise with HAB_ENTRIES_IN() */
extern const uint32_t hab_hal_memory_count;

/** @rom Detector table 
 *@hideinitializer
 *
 * This table is used by functions in the HAB library to determine IC-specific
 * indices for @ref shw engines in alarm or debug registers of an alarm
 * detector engine.
 *
 * @remark Use #HAB_DETECTOR_INDEX() as in the following example, which shows
 * first the CSU alarm detected by the SCC, and then the SCC alarm
 * detected by the CSU:
@verbatim
        const hab_hal_detector_index_t hab_hal_detector[] = {
                HAB_DETECTOR_INDEX(HAB_ENG_CSU, HAB_ENG_SCC, 1),
                HAB_DETECTOR_INDEX(HAB_ENG_SCC, HAB_ENG_CSU, 0),
                ...
                HAB_DETECTOR_INDEX(HAB_ENG_RTIC, HAB_ENG_CSU, 3),
                HAB_DETECTOR_INDEX(HAB_ENG_RTIC, HAB_ENG_SCC, 2),
                ...
        };
@endverbatim
 *
 * @remark Only those entries relating to HAB-supported @ref shw peripherals
 * need to be filled in.  Other alarm detector inputs (e.g. from the system
 * reset controller) can be omitted.
 *
 * @remark The order of entries has no significance. 
 */
extern const hab_hal_detector_index_t hab_hal_detector[];

/** @rom Entries in detector table: initialise with HAB_ENTRIES_IN() */
extern const uint32_t hab_hal_detector_count;

/** @rom @name Available HAB plugins
 *
 * This list shows the plugins exported by the HAB library to the boot ROM.
 *
 * The plugins listed here may be included in #hab_hal_plugin using the
 * HAB_PLUGIN() macro with the same argument.  For example, use
 * HAB_PLUGIN(cmd_base) to include the cmd_base plugin.
 */
/*@{*/
HAB_EXPORT_PLUGIN(cmd_base);    /**< @rom Base commands */
HAB_EXPORT_PLUGIN(cmd_nop);     /**< @rom NOP command */
HAB_EXPORT_PLUGIN(srk);         /**< @rom SRK table format */
HAB_EXPORT_PLUGIN(x509_pkcs1);  /**< @rom X.509v3 certificate format */
HAB_EXPORT_PLUGIN(cms_pkcs1_sha1); /**< @rom CMS signature format */
HAB_EXPORT_PLUGIN(cms_pkcs1_sha256); /**< @rom CMS signature format */
#ifdef HAB_FUTURE
HAB_EXPORT_PLUGIN(wtls);        /**< @rom WTLS certificate format */
HAB_EXPORT_PLUGIN(fsl_sig);     /**< @rom FSL proprietary signature format */
#endif
HAB_EXPORT_PLUGIN(rtic3_sha256); /**< @rom RTIC SHA-256 functionality */
HAB_EXPORT_PLUGIN(rtic3_sha256_nofab); /**< @rom RTIC SHA-256 functionality
                                        *   without fabrication tests
                                        */
HAB_EXPORT_PLUGIN(sahara4_sha256); /**< @rom SAHARA SHA-256 functionality */
HAB_EXPORT_PLUGIN(sahara4_sha256_nofab); /**< @rom Plugin sahara4_sha256
                                          *   without fabrication tests
                                          */
#ifdef HAB_FUTURE
HAB_EXPORT_PLUGIN(sahara4_fp);  /**< @rom SAHARA prime field (RSA)
                                 *   functionality
                                 */
HAB_EXPORT_PLUGIN(rngb);        /**< @rom RNGB fabrication tests */
#endif
HAB_EXPORT_PLUGIN(scc2);        /**< @rom SCCv2 engine */
HAB_EXPORT_PLUGIN(scc2_nofab);  /**< @rom Plugin scc2 without fabrication
                                 *   tests and pre-configuration support
                                 */
HAB_EXPORT_PLUGIN(srtc);        /**< @rom SRTC */
HAB_EXPORT_PLUGIN(cmd_srtc);    /**< @rom SRTC-specific CSF commands */
HAB_EXPORT_PLUGIN(csu);         /**< @rom CSU fabrication tests and
                                 *   pre-configuration support
                                 */
HAB_EXPORT_PLUGIN(dcp2);        /**< @rom DCPv2 base functionality */
HAB_EXPORT_PLUGIN(dcp2_nofab);  /**< @rom Plugin dcp2 without
                                   *   fabrication tests
                                   */
HAB_EXPORT_PLUGIN(dcp2_sha1);   /**< @rom DCP SHA-1 functionality */
HAB_EXPORT_PLUGIN(dcp2_sha256); /**< @rom DCP SHA-256 functionality */
HAB_EXPORT_PLUGIN(sw_sha1);     /**< @rom SW SHA-1 implementation */
HAB_EXPORT_PLUGIN(sw_pkcs1);    /**< @rom SW PKCS#1 implementation */
HAB_EXPORT_PLUGIN(sw_sha256);   /**< @rom SW SHA-256 implementation */
#ifdef HAB_FUTURE
HAB_EXPORT_PLUGIN(sw_sha512);   /**< @rom SW SHA-512 implementation */
HAB_EXPORT_PLUGIN(sw_ecdsa_p521); /**< @rom SW ECDSA over NIST curve P-521 */
#endif
HAB_EXPORT_PLUGIN(sw_keystore); /**< @rom SW Keystore */
HAB_EXPORT_PLUGIN(sw_fp);       /**< @rom SW Prime field Arithmetic */
HAB_EXPORT_PLUGIN(sw_state);    /**< @rom SW state machine */
HAB_EXPORT_PLUGIN(rtl_test0);   /**< @rom RTL Test level 0 */
HAB_EXPORT_PLUGIN(rtl_test1);   /**< @rom RTL Test level 1 */
/*@}*/

/** @rom DCP base address
 * @ingroup dcp
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_dcp_base;

/** @rom SCC base address
 * @ingroup scc
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_scc_base;

/** @rom SCC secure RAM base address
 * @ingroup scc
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_secure_ram_base;

/** @rom RTIC base address
 * @ingroup rtic
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_rtic_base;

/** @rom SAHARA base address
 * @ingroup sah
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_sahara_base;

/** @rom CSU base address
 * @ingroup csu
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_csu_base;

/** @rom SRTC base address
 * @ingroup srtc
 *
 * This symbol must be defined at link time
 */
extern volatile void* const hab_hal_srtc_base;

/*===========================================================================
                         FUNCTION PROTOTYPES
=============================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

/** @rom Check engine enabled
 *
 * @par Purpose
 *
 * Determine if a HW engine is enabled on this IC.  It is used by HAB prior to
 * using an engine.
 * 
 * @par Operation 
 *
 * This function uses IC-specific methods, such as checking fuses.  It may
 * also hard-code responses for some engines which are always enabled or
 * always disabled.
 *
 * @param[in] eng Engine to check
 *
 * @warning If the given engine is not present on this IC, the function
 * returns FALSE.
 *
 * @pre None.
 * @post None.
 *
 * @retval TRUE if the engine is enabled.
 * @retval FALSE otherwise.
 */
extern bool
hab_hal_enabled(uint32_t eng);

/** @rom Translate addresses for DMA
 *
 * @par Purpose
 *
 * On some ICs, a memory buffer accessible to the core may have a different
 * address when accessed by a HW engine with its own DMA, or may not be
 * accessible at all.  This function determines both the accessibility and
 * address translation of a memory buffer to a HW engine.  It is used by HAB
 * prior to programming DMA.
 * 
 * @par Operation 
 *
 * This function uses IC-specific information to determine accessibility and
 * calculate the translated addresses.  If the DMA and core addresses for the
 * memory buffer match, the output and input addresses coincide.
 *
 * @param[in] eng Engine to check
 *
 * @param[in] start Memory buffer address according to the core
 *
 * @param[in] bytes Memory buffer size
 *
 * @warning This function does not consider alignment constraints.
 *
 * @warning If engine @a eng has no DMA capability, this function returns
 * NULL.
 *
 * @pre Engine @a eng is enabled.
 *
 * @post None.
 *
 * @retval "DMA start address" if engine @a eng has DMA and can access the
 * entire memory buffer contiguously,
 *
 * @retval NULL otherwise.
 */
extern void*
hab_hal_dma_address(uint32_t eng, const void* start, size_t bytes);

/** @rom Allocate engine
 *
 * @par Purpose
 *
 * Allocate a HW engine to the current processor.  It is used by HAB prior to
 * using an engine.
 * 
 * @par Operation 
 *
 * This function uses IC-specific methods, such as bus arbiters.  It may also
 * hard-code responses for some engines which are permanently allocated or
 * inaccessible.
 *
 * @param[in] eng Engine to allocate
 *
 * @pre the given engine is present and enabled.
 *
 * @post If successful, the engine is allocated.
 *
 * @retval HAB_SUCCESS if the engine is allocated.
 * @retval HAB_FAILURE otherwise.
 *
 */
extern hab_status_t 
hab_hal_allocate(uint32_t eng);

/** @rom Release engine
 *
 * @par Purpose
 *
 * Release a HW engine allocated to the current processor.  It is used by HAB
 * after using an engine.
 * 
 * @par Operation 
 *
 * This function uses IC-specific methods, such as bus arbiters.  It may also
 * hard-code responses for some engines which are permanently allocated or
 * inaccessible.
 *
 * @param[in] eng Engine to release
 *
 * @pre the given engine is present and enabled.
 *
 * @post If successful, the engine is released if possible.
 *
 * @retval HAB_SUCCESS if the engine is released (or permanently allocated).
 * @retval HAB_FAILURE otherwise.
 *
 */
extern hab_status_t 
hab_hal_release(uint32_t eng);

/** @rom Flush cache
 *
 * @par Purpose
 *
 * Flush any cache lines required by DMA-enabled @ref shw engines.  It is used
 * by HAB prior to starting a DMA-enabled @ref shw engine.
 * 
 * @par Operation 
 *
 * This function uses IC-specific methods, but typically flushes any cache
 * lines covering the DMA memory region defined by hab_hal_ict.dma.  If the
 * processor cache is known to be disabled or configured as write-through,
 * this function can return immediately.
 *
 * @pre None.
 *
 * @post Data written by the processor to the DMA memory region is visible to
 * DMA-enabled @ref shw engines.
 *
 */
extern void
hab_hal_flush_cache(void);

/** @rom Invalidate cache
 *
 * @par Purpose
 *
 * Invalidate any cache lines used by DMA-enabled @ref shw engines.  It is
 * used by HAB prior to reading results from a DMA-enabled @ref shw engine.
 * 
 * @par Operation 
 *
 * This function uses IC-specific methods, but typically invalidates any cache
 * lines covering the DMA memory region defined by hab_hal_ict.dma.  If the
 * processor cache is known to be disabled for the DMA memory region, this
 * function can return immediately.
 *
 * @pre None.
 *
 * @post Data written by DMA-enabled @ref shw engines to the DMA memory region
 * is visible to the processor.
 */
extern void
hab_hal_invalidate_cache(void);

/** @rom Get boot configuration
 *
 * @par Purpose
 *
 * This function provides the boot configuration to HAB.
 * 
 * @par Operation 
 *
 * Implementation is core-specific.
 *
 * @pre None.
 * @post None.
 *
 * @returns A value from #hab_hal_boot_cfg_t.
 */
extern hab_hal_boot_cfg_t
hab_hal_get_boot_cfg(void);

/** @rom Get fabrication test configuration
 *
 * @par Purpose
 *
 * This function provides the fabrication test configuration to HAB.
 * 
 * @par Operation 
 *
 * Implementation is core-specific.
 *
 * @pre None.
 * @post None.
 *
 * @returns A value from #hab_hal_test_cfg_t.
 */
extern hab_hal_test_cfg_t
hab_hal_get_test_cfg(void);

/** @rom Enter failsafe boot mode.
 *
 * @par Purpose
 *
 * This function implements the hab_rvt.failsafe() API, intended for post-ROM
 * boot components to call via the @ref rvt.
 * 
 * @par Operation 
 *
 * Implementation is core-specific.
 *
 * @pre The functionality of hab_rvt.exit() has been performed
 *
 * @warning This function should not return.
 */
extern void
hab_hal_failsafe(void);

/** @rom Read SRK hash
 *
 * @par Purpose
 *
 * This function reads the @ref srk_fuses.  It is used by the HAB as part of
 * the @ref srk_pcl.
 * 
 * @par Operation 
 *
 * This function uses the IC-specific fuse map to locate the relevant fuses
 * and then copies them to the output buffer.  The most-significant fuse bits
 * required should be copied to the first byte in the buffer.
 *
 * For example, if @a bytes = 20, then fuses [159:152] are copied to @a
 * hash[0], fuses [151:144] to @a hash[1] and so on until fuses [7:0] are
 * copied to @a hash[19].
 *
 * @param[out] hash Reference hash value.
 *
 * @param[in] bytes Reference hash size.
 *
 * @post In case of failure, the contents of @a hash are undefined.
 *
 * @retval HAB_SUCCESS if @a hash is not NULL and there are sufficient fuses
 * defined in the fuse map to match the size given by @a bytes.
 *
 * @retval HAB_FAILURE otherwise
 */
extern hab_status_t 
hab_hal_read_srk_hash(uint8_t* hash, size_t bytes);

/** @rom Read SRK mask
 *
 * @par Purpose
 *
 * This function uses IC-specific configuration data to determine if
 * particular entries in the @ref crt_srk are unavailable.  Entries may be
 * unavailable for several reasons, including:
 *
 * - on an IC with multiple cores, different entry ranges may be allocated to
 * each core at IC integration.  Where this is done, the entries outside the
 * allocated range for the executing core must be reported as unavailable.
 *
 * - on an IC which implements SRK revocation fuses, any entries whose
 * corresponding fuse has been blown must be reported as unavailable.
 *
 * @returns A mask with bits set to indicate unavailable @ref crt_srk indices.
 * If bit @a n is set in the return value (bit 0 is the least-significant
 * bit), then the SRK at index @a n is unavailable.
 */
extern uint32_t
hab_hal_read_srk_mask(void);

/** @rom Read security configuration
 *
 * @par Purpose
 *
 * This function uses the IC-specific fuse map to locate the security
 * configuration fuses, and determines the corresponding configuration.
 *
 * @warning Quasi-external boot modes must be disabled on ICs configured
 * as #HAB_CFG_CLOSED.
 *
 * @warning The caller of this function is responsible to check that the
 * return value matches a defined configuration.
 *
 * @returns a value in #hab_config.
 */
extern hab_config_t
hab_hal_read_sec_cfg(void);

/** @rom Read Fabrication UID
 *
 * @par Purpose
 *
 * This function reads the binding value for the #HAB_BND_FID binding
 * object. It is used by the HAB as part of the @ref sig_bnd method.
 * 
 * @par Operation 
 *
 * This function uses the IC-specific fuse map to locate the relevant fuses
 * and then copies them to the output buffer.
 *
 * @param[out] uid Fabrication UID value.
 *
 * @param[in,out] bytes Buffer @a uid size on entry. Fabrication UID size on
 * exit.
 *
 * @post In case of failure, the contents of @a uid are undefined.
 *
 * @retval HAB_SUCCESS if @a uid is not NULL and the entry value of @a bytes
 * is sufficient to hold the fuse field defined in the fuse map.
 *
 * @retval HAB_FAILURE otherwise
 */
extern hab_status_t 
hab_hal_read_fab_uid(uint8_t* uid, size_t* bytes);

/** @rom Read fuse field
 *
 * @par Purpose
 *
 * This function reads a given bitfield from the fuse banks.  It is used by
 * the HAB to read the @ref bnd_mid specified in a @ref cmd_set command as
 * part of the @ref sig_bnd method.
 * 
 * @par Operation 
 *
 * This function locates the specified fuses and then copies them to the
 * output buffer following the method described for the @ref bnd_mid.  The
 * output buffer may be omitted (set to NULL) in order to test the validity of
 * the fuse field specification.
 *
 * @param[out] value Fuse field value.
 *
 * @param[in] bank fuse bank from which to read.
 *
 * @param[in] row starting row number of bitfield within @a bank.
 *
 * @param[in] bit position of first fuse within @a row (bit 7 is
 * most-significant or left-most bit).
 *
 * @param[in] fuses number of fuses in bitfield.
 *
 * @pre If @a value is not NULL, the output buffer is at least
 * HAB_HAL_FUSE_FIELD_BYTES(bit, fuses) long.
 *
 * @post In case of failure, the contents of @a value are undefined.
 *
 * @retval HAB_SUCCESS if the fuse field location corresponds to fuses
 * implemented on the IC.
 *
 * @retval HAB_FAILURE otherwise
 */
extern hab_status_t 
hab_hal_read_fuse_field(uint8_t* value, uint32_t bank, uint32_t row, 
                        uint32_t bit, uint32_t fuses);

/** @rom Read register
 *
 * This function reads a given register and returns the 
 * value based on the data_width(8/16/32 bit)parameter.
 *
 * @param[in] reg register from which to read
 *
 * @param[in] data_width width of the data to be read
 *
 * @pre Parameter @a reg is not NULL.
 *
 * @pre Address @a reg is correctly aligned for size @a data_width.
 *
 * @post None.
 *
 * @returns Register contents.
 */
extern uint32_t 
hab_hal_read_register(const volatile void* reg, hab_data_width_t data_width);

/** @rom Write register
 *
 * This function writes a given value to the register
 * based on the data_width(8/16/32 bit) parameter.
 *
 * @param[out] reg register into which to write
 *
 * @param[in] val value to write
 *
 * @param[in] data_width width of the data to be written
 *
 * @pre Parameter @a reg is not NULL.
 *
 * @pre Address @a reg is correctly aligned for size @a data_width.
 *
 * @post None.
 *
 * @returns None.
 */
extern void
hab_hal_write_register(volatile void* reg, uint32_t val, hab_data_width_t data_width);

/** @rom memcpy replacement - see standard definition */
extern void* hab_hal_memcpy(void* s, const void* ct, size_t n);

/** @rom memcmp replacement - see standard definition */
extern int hab_hal_memcmp(const void* cs, const void* ct, size_t n);

/** @rom memset replacement - see standard definition */
extern void* hab_hal_memset(void* s, unsigned char c, size_t n);

#ifdef __cplusplus
}
#endif

/*  @} hal */

/** @endcond */

#endif /* HAB_HAL_H */
