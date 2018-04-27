#ifndef HAB_H
#define HAB_H
/*===========================================================================*/
/**
    @file    hab.h

    @brief   HAB main interface

@verbatim
=============================================================================

              Freescale Semiconductor Confidential Proprietary
    (c) Freescale Semiconductor, Inc. 2007, 2008, 2009 All rights reserved.

Presence of a copyright notice is not an acknowledgement of
publication. This software file listing contains information of
Freescale Semiconductor, Inc. that is of a confidential and
proprietary nature and any viewing or use of this file is prohibited
without specific written permission from Freescale Semiconductor, Inc.

=============================================================================
Revision History:

               Modification Date   Tracking
Author           (dd-mmm-yyyy)      Number    Description of Changes
---------------  -------------    ----------  -----------------------
David Hartley    11-Dec-2007      ENGR55511   Initial version
David Hartley    29-Jul-2008      ENGR82581   Add CMS, X.509v3, SHA-512
                                              and ECDSA over P-521
David Hartley    07-Nov-2008      ENGR92335   Implementation updates
Rod Ziolkowski   29-Jan-2008      ENGR106570  Update IVT entry to a func ptr
Ram Korukonda    04-Mar-2009      ENGR106041  Updated report_event and 
                                              authenticate_image descriptions
Yi Li            25-Mar-2009      ENGR106040  Editorial correction for SIS 
Rod Ziolkowski   04-May-2009      ENGR112074  Update audit events for data
                                              structures w/ invalid version
David Hartley    15-May-2009      ENGR112382  Avoid excess authenticate_image
                                              logs
David Hartley    19-May-2009      ENGR112551  Assert only 1 byte of boot data
                                              Check IVT self and entry
                                              pointers for NULL.  Clarify
                                              authenticate_image return values
                                              in non-closed configurations
David Hartley    04-Aug-2009      ENGR111476  Byte array conversion
David Hartley    02-Sep-2009      ENGR114783  Add support for export control
Rod Ziolkowski   12-Nov-2009      ENGR117776  Expand scope of report event and
                                              report status APIs outside 
                                              entry and exit.
=============================================================================
Portability: 

These definitions are customised for 32 bit cores of either
endianness.

=============================================================================
@endverbatim */

/*===========================================================================
                            INCLUDE FILES
=============================================================================*/

#include "hab_types.h"          /* Shared types, constants, macros */

/*===========================================================================
                              CONSTANTS
=============================================================================*/

/*===========================================================================
                                MACROS
=============================================================================*/

/*===========================================================================
                                ENUMS
=============================================================================*/

/*===========================================================================
                    STRUCTURES AND OTHER TYPEDEFS
=============================================================================*/

/** Loader callback.
 * @ingroup auth_img
 *
 * @par Purpose
 *
 * This function must be supplied by the library caller if required. It is
 * intended to finalise image loading in those boot modes where only a portion
 * of the image is loaded to a temporary initial location prior to device
 * configuration.
 * 
 * @par Operation 
 *
 * This function is called during hab_rvt.authenticate_image() between running
 * the @ref dcd and @ref csf.  The operation of this function is defined by
 * the caller.
 *
 * @param[in,out] start Initial (possibly partial) image load address on
 * entry.  Final image load address on exit.
 *
 * @param[in,out] bytes Initial (possibly partial) image size on entry.  Final
 * image size on exit.
 *
 * @param[in] boot_data Initial @ref ivt Boot Data load address.
 *
 * @remark The interpretation of the Boot Data is defined by the caller.
 * Different boot components or modes may use different boot data, or even
 * different loader callback functions.
 *
 * @warning It should not be assumed by this function that the Boot Data is
 * valid or authentic.
 *
 * @warning It is the responsibility of the loader callback to check the final
 * image load addresses using hab_rvt.check_target() prior to copying any image
 * data.
 *
 * @pre The (possibly partial) image has been loaded in the initial load
 * address, and the Boot Data is within the initial image.
 *
 * @pre The @ref dcd has been run, if provided.
 *
 * @post The final image load addresses pass hab_rvt.check_target().
 *
 * @retval #HAB_SUCCESS if all operations completed successfully,
 *
 * @retval #HAB_FAILURE otherwise.
 */
typedef hab_status_t (*hab_loader_callback_f)(
    void** start, 
    size_t* bytes, 
    const void* boot_data);

/*---------------------------------------------------------------------------*/

/** Image entry function prototype
 *  @ingroup rvt
 *
 * This typedef serves as the return type for hab_rvt.authenticate_image().  It
 * specifies a void-void function pointer, but can be cast to another function
 * pointer type if required.
 */
typedef void (*hab_image_entry_f)(void);

/*---------------------------------------------------------------------------*/

/** @ref rvt structure
 *  @ingroup rvt
 *
 * @par Format
 *
 * The @ref rvt consists of a @ref hdr followed by a list of addresses as
 * described further below.
 */
struct hab_rvt {

    /** @ref hdr with tag #HAB_TAG_RVT, length and HAB version fields
     *  (see @ref data)
     */
    hab_hdr_t hdr;

    /** Enter and initialise HAB library.
     * @ingroup entry
     *
     * @par Purpose
     *
     * This function initialises the HAB library and @ref shw plugins.  It is
     * intended for use by post-ROM boot stage components, via the @ref rvt,
     * prior to calling any other HAB functions other than
     * hab_rvt.report_event() and hab_rvt.report_status().
     * 
     * @ifrom It is also intended for use by the boot ROM via hab_rvt.entry().
     * @endrom
     * 
     * @par Operation 
     *
     * This function performs the following operations every time it is called:
     *
     * - Initialise the HAB library internal state
     * - Run the entry sequence of each available @ref shw plugin
     *
     * If any failure occurs, an audit event is logged and all remaining
     * operations are abandoned.
     *
     * When first called from boot ROM, this function also performs the
     * following operations prior to those given above:
     *
     * - Initialise the internal key store
     * - Run the self-test sequence of each available @ref shw plugin
     * - If a state machine is present and enabled, change the security state
     * as follows:
     *   - If the IC is configured as #HAB_CFG_OPEN, move to
     *   #HAB_STATE_NONSECURE
     *   - If the IC is configured as #HAB_CFG_CLOSED, move to
     *   #HAB_STATE_TRUSTED
     *   - Otherwise, leave the security state unchanged
     *
     * If any failure occurs in the operations above, an audit event is
     * logged, all remaining operations are abandoned, and, if a state machine
     * is present and enabled, the security state is set as follows:
     *
     *  - If the IC is configured as #HAB_CFG_OPEN or #HAB_CFG_CLOSED, move to
     *  #HAB_STATE_NONSECURE.  Note that if a security violation has been
     *  detected by the HW, the final state will be #HAB_STATE_FAIL_SOFT or
     *  #HAB_STATE_FAIL_HARD depending on the HW configuration.
     *  - Otherwise, leave the security state unchanged
     *
     * @warning Boot sequences may comprise several images with each launching
     * the next as well as alternative images should one boot device or boot
     * image be unavailable or unusable.  The authentication of each image in
     * a boot sequence must be bracketed by its own hab_rvt.entry()
     * ... hab_rvt.exit() pair in order to ensure that security state
     * information gathered for one image cannot be misapplied to another
     * image.
     *
     * @ifrom 
     *
     * @warning This applies to each boot path in boot ROM as well, except for
     * the fabrication test path.
     *
     * @endrom
     *
     * @post HAB library internal state is initialised.
     *
     * @post Available @ref shw plugins are initialised.
     *
     * @post If a failure or warning occurs during @ref shw plugin
     * initialisation, an audit event is logged with the relevant @ref eng
     * tag.  The status and reason logged are described in the relevant @ref
     * shw plugin documentation.
     *
     * @post Security state is initialised, if a state machine is present and
     * enabled.
     *
     * @retval #HAB_SUCCESS on an IC not configured as #HAB_CFG_CLOSED,
     * although unsuccessful operations will still generate audit log events,
     *
     * @retval #HAB_SUCCESS on other ICs if all commands completed
     * without failure (even if warnings were generated),
     *
     * @retval #HAB_FAILURE otherwise.
     */
    hab_status_t (*entry)(void);

    /** Finalise and exit HAB library.
     * @ingroup exit
     *
     * @par Purpose
     *
     * This function finalises the HAB library and @ref shw plugins.  It is
     * intended for use by post-ROM boot stage components, via the @ref rvt,
     * after calling other HAB functions and prior to launching the next boot
     * stage or switching to another boot path.
     *
     * @ifrom It is also intended for use by the boot ROM via hab_rvt.exit().
     * @endrom
     * 
     * @par Operation 
     *
     * This function performs the following operations:
     *
     * - Finalise the HAB library internal state
     * - Run the finalisation sequence of each available @ref shw plugin
     *
     * If any failure occurs, an audit event is logged and all remaining
     * operations are abandoned.
     *
     * @warning See warnings for hab_rvt.entry().
     *
     * @post #HAB_ASSERT_BLOCK records are cleared from audit log.  Note that
     * other event records are not cleared.
     *
     * @post Any keys installed by @ref csf commands remain active.
     *
     * @post Available @ref shw plugins are in their final state as described
     * in the relevant sections.
     *
     * @post If a failure or warning occurs, an audit event is logged with the
     * @ref eng tag of the @ref shw plugin concerned.  The status and reason
     * logged are described in the relevant @ref shw plugin documentation.
     *
     * @retval #HAB_SUCCESS on an IC not configured as #HAB_CFG_CLOSED,
     * although unsuccessful operations will still generate audit log events,
     *
     * @retval #HAB_SUCCESS on other ICs if all commands completed
     * without failure (even if warnings were generated),
     *
     * @retval #HAB_FAILURE otherwise.
     */
    hab_status_t (*exit)(void);

    /** Check target address
     * @ingroup chk_tgt
     *
     * @par Purpose
     *
     * This function reports whether or not a given target region is allowed
     * for either peripheral configuration or image loading in memory.  It is
     * intended for use by post-ROM boot stage components, via the @ref rvt,
     * in order to avoid configuring security-sensitive peripherals, or
     * loading images over sensitive memory regions or outside recognised
     * memory devices in the address map.
     * 
     * @ifrom It is also available for use by the boot ROM, both directly via
     * hab_rvt.check_target() and indirectly via hab_rvt.authenticate_image(). 
     * @endrom
     *
     * @par Operation 
     *
     * The lists of allowed target regions vary by IC and core, and should be
     * taken from the @ref ref_rug.
     *
     * @ifrom The allowed register sets for peripheral configuration and memory
     * regions for image loading are defined in the @ref hal by
     * #hab_hal_peripheral and #hab_hal_memory respectively. @endrom
     * 
     * @param[in] type Type of target (memory or peripheral)
     *
     * @param[in] start Address of target region
     *
     * @param[in] bytes Size of target region
     *
     * @post if the given target region goes beyond the allowed regions, an
     * audit event is logged with status #HAB_FAILURE and reason
     * #HAB_INV_ADDRESS, together with the call parameters.  See the @ref evt
     * record documentation for details.
     *
     * @post For successful commands, no audit event is logged.
     *
     * @retval #HAB_SUCCESS on an IC not configured as #HAB_CFG_CLOSED,
     * although unsuccessful operations will still generate audit log events,
     *
     * @retval #HAB_SUCCESS if the given target region lies wholly within the
     * allowed regions for the requested type of target.
     *
     * @retval #HAB_FAILURE otherwise
     */
    hab_status_t (*check_target)(hab_target_t type, 
                                 const void* start,
                                 size_t bytes);

    /** Authenticate image.
     * @ingroup auth_img
     *
     * @par Purpose
     *
     * This function combines DCD, CSF and Assert functions in a standard
     * sequence in order to authenticate a loaded image.  It is intended for
     * use by post-ROM boot stage components, via the @ref rvt.  Support for
     * images partially loaded to an initial location is provided via a
     * callback function.
     * 
     * @ifrom It is also available for use by the boot ROM via
     * hab_rvt.authenticate_image(). @endrom
     * 
     * @par Operation 
     *
     * This function performs the following sequence of operations:
     * - Check that the initial image load addresses pass
     * hab_rvt.check_target().
     * - Check that the IVT offset lies within the initial image bounds.
     * - Check that the @ref ivt @a self and @a entry pointers are not NULL
     * - Check the @ref ivt header for consistency and compatability.
     * - If provided in the @ref ivt, calculate the @ref dcd initial location,
     * check that it lies within the initial image bounds, and run the @ref
     * dcd commands.
     * - If provided in the @ref ivt, calculate the Boot Data initial location
     * and check that it lies within the initial image bounds.
     * - If provided in the parameters, invoke the callback function with the
     * initial image bounds and initial location of the @ref ivt Boot Data.
     * 
     * From this point on, the full image is assumed to be in its final
     * location. The following operations will be performed on all IC 
     * configurations (#hab_config), but will be only enforced on an IC 
     * configured as #HAB_CFG_CLOSED:
     * - Check that the final image load addresses pass hab_rvt.check_target().
     * - Check that the CSF lies within the image bounds, and run the CSF
     * commands.
     * - Check that all of the following data have been authenticated (using
     * their final locations):
     *   - IVT;
     *   - DCD (if provided);
     *   - Boot Data (initial byte if provided);
     *   - Entry point (initial word).
     *
     * @param[in] cid Caller ID, used to identify which SW issued this call.
     *
     * @param[in] ivt_offset Offset in bytes of the IVT from the image start
     * address.
     *
     * @param[in,out] start Initial (possibly partial) image load address on
     * entry.  Final image load address on exit.
     *
     * @param[in,out] bytes Initial (possibly partial) image size on entry.
     * Final image size on exit.
     *
     * @param[in] loader Callback function to load the full image to its final
     * load address.  Set to NULL if not required.
     *
     * @remark Caller ID may be bound to signatures verified using keys
     * installed with #HAB_CMD_INS_KEY_CID flag. See @ref cmd_ins_key and @ref
     * bnd_obj for details.
     *
     * @remark A @a loader callback function may be supplied even if the image
     * is already loaded to its final location on entry.
     *
     * @remark Boot Data (boot_data in @ref ivt) will be ignored if the 
     * @a loader callback function point is set to Null.
     *
     * @warning The @a loader callback function should lie within existing
     * authenticated areas. @ifrom Or within the ROM. @endrom
     *
     * @warning It is the responsibility of the caller to check the initial
     * image load addresses using hab_rvt.check_target() prior to loading the
     * initial image and calling this function.
     *
     * @warning After completion of hab_rvt.authenticate_image(), the caller
     * should test using hab_rvt.assert() that the Boot Data was
     * authenticated.
     *
     * @post The post-conditions of the functions hab_rvt.check_target(),
     * hab_rvt.run_dcd(), hab_rvt.run_csf() and hab_rvt.assert() apply also to
     * this function.  In particular, any audit events logged within the given
     * functions have the context field appropriate to that function rather
     * than #HAB_CTX_AUTHENTICATE.  In addition, the side-effects and
     * post-conditions of any callback function supplied apply.
     *
     * @post If a failure or warning occurs outside these contexts, an audit
     * event is logged with status:
     *   - #HAB_FAILURE, with further reasons:
     *     - #HAB_INV_ADDRESS: initial or final image addresses outside allowed
     *     regions
     *     - #HAB_INV_ADDRESS: IVT, DCD, Boot Data or CSF outside image bounds
     *     - #HAB_INV_ADDRESS: IVT @a self or @a entry pointer is NULL 
     *     - #HAB_INV_CALL: hab_rvt.entry() not run successfully prior to call
     *     - #HAB_INV_IVT: IVT malformed
     *     - #HAB_INV_IVT: IVT version number is less than HAB library version
     *     - #HAB_INV_RETURN: Callback function failed
     *
     * @retval entry field from @ref ivt on an IC not configured as
     * #HAB_CFG_CLOSED provided that the following conditions are met
     * (other unsuccessful operations will generate audit log events):
     *  - the @a start pointer and the pointer it locates are not NULL
     *  - the initial @ref ivt location is not NULL
     *  - the final @ref ivt location (given by the @a self field) is not NULL 
     *  - any loader callback completed successfully,
     *
     * @retval entry field from @ref ivt on other ICs if all operations
     * completed without failure (even if warnings were generated),
     *
     * @retval NULL otherwise.
     */
    hab_image_entry_f (*authenticate_image)(uint8_t cid,
                                            ptrdiff_t ivt_offset, 
                                            void** start, 
                                            size_t* bytes, 
                                            hab_loader_callback_f loader);

    /** Execute a boot configuration script.
     * @ingroup run_dcd
     *
     * @par Purpose
     *
     * This function configures the IC based upon a @ref dcd table.  It is
     * intended for use by post-ROM boot stage components, via the @ref rvt.
     * This function may be invoked as often as required for each boot stage.
     *
     * @ifrom It is also intended for use by the boot ROM, both directly via
     * hab_rvt.run_dcd() and indirectly via hab_rvt.authenticate_image().
     * @endrom
     * 
     * The difference between the configuration functionality in this function
     * and hab_rvt.run_csf() arises because the @ref dcd table is not
     * authenticated prior to running the commands.  Hence, there is a more
     * limited range of commands allowed, and a limited range of parameters to
     * allowed commands.
     * 
     * @par Operation 
     *
     * This function performs the following operations:
     * - Checks the @ref hdr for compatibility and consistency
     * - Makes an internal copy of the @ref dcd table
     * - Executes the commands in sequence from the internal copy of the @ref
     * dcd
     *
     * If any failure occurs, an audit event is logged and all remaining
     * operations are abandoned.
     *
     * @param[in] dcd   Address of the @ref dcd.
     *
     * @warning It is the responsibility of the caller to ensure that the @a
     * dcd parameter points to a valid memory location.
     *
     * @warning The @ref dcd must be authenticated by a subsequent @ref csf
     * command prior to launching the next boot image, in order to avoid
     * unauthorised configurations which may subvert secure operation.
     * Although the content of the next boot stage's CSF may be out of scope
     * for the hab_rvt.run_dcd() caller, it is possible to enforce this
     * constraint by using hab_rvt.assert() to ensure that both the DCD and
     * any pointers used to locate it have been authenticated.
     *
     * @warning Each invocation of hab_rvt.run_dcd() must occur between a pair
     * of hab_rvt.entry() and hab_rvt.exit() calls, although multiple
     * hab_rvt.run_dcd() calls (and other HAB calls) may be made in one
     * bracket.  This constraint applies whether hab_rvt.run_dcd() is
     * successful or not: a subsequent call to hab_rvt.exit() is required
     * prior to launching the authenticated image or switching to another boot
     * target.
     *
     * @post Many commands may cause side-effects. See the @ref dcd
     * documentation.
     *
     * @post If a failure or warning occurs within a command handler, an audit
     * event is logged with the offending command, copied from the DCD.  The
     * status and reason logged are described in the relevant command
     * documentation.
     *
     * @post For other failures or warning, the status logged is:
     *   - #HAB_WARNING, with further reasons:
     *     - #HAB_UNS_COMMAND: unsupported command encountered, where DCD
     *     version and HAB library version differ
     *   - #HAB_FAILURE, with further reasons:
     *     - #HAB_INV_ADDRESS: NULL @a dcd parameter
     *     - #HAB_INV_CALL: hab_rvt.entry() not run successfully prior to call
     *     - #HAB_INV_COMMAND: command not allowed in DCD
     *     - #HAB_UNS_COMMAND: unrecognised command encountered, where DCD
     *     version and HAB library version match
     *     - #HAB_INV_DCD: DCD malformed or too large
     *     - #HAB_INV_DCD: DCD version number is less than HAB library version
     * @retval #HAB_SUCCESS on an IC not configured as #HAB_CFG_CLOSED,
     * although unsuccessful operations will still generate audit log events,
     *
     * @retval #HAB_SUCCESS on other ICs if all commands completed
     * without failure (even if warnings were generated),
     *
     * @retval #HAB_FAILURE otherwise.
     */
    hab_status_t (*run_dcd)(const uint8_t* dcd);

    /** Execute an authentication script.
     * @ingroup run_csf
     *
     * @par Purpose
     *
     * This function authenticates SW images and configures the IC based upon
     * a @ref csf.  It is intended for use by post-ROM boot stage components,
     * via the @ref rvt.  This function may be invoked as often as required
     * for each boot stage.
     * 
     * @ifrom It is also available for use by the boot ROM via hab_rvt.run_csf,
     * although it is anticipated that the boot ROM will mostly call this
     * function indirectly via hab_rvt.authenticate_image(). @endrom
     *
     * @par Operation 
     *
     * This function performs the following operations:
     * - Checks the @ref hdr for compatibility and consistency
     * - Makes an internal copy of the @ref csf
     * - Executes the commands in sequence from the internal copy of the @ref
     * csf
     *
     * The internal copy of the @ref csf is authenticated by an explicit
     * command in the sequence.  Prior to authentication, a limited set of
     * commands is available to:
     * - Install a Super-Root key (unless previously installed)
     * - Install a CSF key (unless previously installed)
     * - Specify any variable configuration items
     * - Authenticate the CSF
     *
     * Subsequent to CSF authentication, the full set of commands is available.
     *
     * If any failure occurs, an audit event is logged and all remaining
     * operations are abandoned.
     *
     * @param[in] csf   Address of the @ref csf.
     *
     * @param[in] cid Caller ID, used to identify which SW issued this call.
     *
     * @remark Caller ID may be bound to signatures verified using keys
     * installed with #HAB_CMD_INS_KEY_CID flag. See @ref cmd_ins_key and @ref
     * bnd_obj for details.
     *
     * @warning It is the responsibility of the caller to ensure that the @a
     * csf parameter points to a valid memory location.
     *
     * @warning Each invocation of hab_rvt.run_csf() must occur between a pair
     * of hab_rvt.entry() and hab_rvt.exit() calls, although multiple
     * hab_rvt.run_csf() calls (and other HAB calls) may be made in one
     * bracket.  This constraint applies whether hab_rvt.run_csf() is
     * successful or not: a subsequent call to hab_rvt.exit() is required
     * prior to launching the authenticated image or switching to another boot
     * target.
     *
     * @post Many commands may cause side-effects. See the @ref csf
     * documentation.  In particular, note that keys installed by the @ref csf
     * remain available for use in subsequent operations.
     *
     * @post If a failure or warning occurs within a command handler, an audit
     * event is logged with the offending command, copied from the CSF.  The
     * status and reason logged are described in the relevant command
     * documentation.
     *
     * @post For other failures or warning, the status logged is:
     *   - #HAB_WARNING, with further reasons:
     *     - #HAB_UNS_COMMAND: unsupported command encountered, where CSF
     *     version and HAB library version differ
     *   - #HAB_FAILURE, with further reasons:
     *     - #HAB_INV_ADDRESS: NULL @a csf parameter
     *     - #HAB_INV_CALL: hab_rvt.entry() not run successfully prior to call
     *     - #HAB_INV_COMMAND: command not allowed prior to CSF authentication
     *     - #HAB_UNS_COMMAND: unrecognised command encountered, where CSF
     *     version and HAB library version match
     *     - #HAB_INV_CSF: CSF not authenticated
     *     - #HAB_INV_CSF: CSF malformed or too large
     *     - #HAB_INV_CSF: CSF version number is less than HAB library version
     *
     * @retval #HAB_SUCCESS on an IC not configured as #HAB_CFG_CLOSED,
     * although unsuccessful operations will still generate audit log events,
     *
     * @retval #HAB_SUCCESS on other ICs if all commands completed
     * without failure (even if warnings were generated),
     *
     * @retval #HAB_FAILURE otherwise.
     */
    hab_status_t (*run_csf)(const uint8_t* csf, 
                            uint8_t cid);

    /** Test an assertion against the audit log.
     * @ingroup assert
     *
     * @par Purpose
     *
     * This function allows the audit log to be interrogated.  It is intended
     * for use by post-ROM boot stage components, via the @ref rvt, to
     * determine the state of authentication operations.  This function may be
     * invoked as often as required for each boot stage.
     *
     * @ifrom It is also available for use by the boot ROM, both directly via
     * hab_rvt.assert() and indirectly via hab_rvt.authenticate_image().
     * @endrom
     *
     * @par Operation 
     *
     * This function checks the required assertion as detailed below.
     *
     * @param[in] type Assertion type.
     *
     * @param[in] data Assertion data.
     *
     * @param[in] count Data size or count.
     *
     * @par Memory block authentication:
     * For #HAB_ASSERT_BLOCK assertion type, hab_rvt.assert() checks that the
     * given memory block has been authenticated after running a CSF. The
     * parameters are interpreted as follows:
     *
     * @par
     * - @a data: memory block starting address
     * - @a count: memory block size (in bytes)
     *
     * @par 
     *
     * A simple interpretation of "memory block has been authenticated" is
     * taken, such that the given block must lie wholly within a single
     * contiguous block authenticated while running a CSF.  A given memory
     * block covered by the union of several neighboring or overlapping
     * authenticated blocks could fail the test with this interpretation, but
     * it is assumed that such cases will not arise in practice.
     *
     * @post If the assertion fails, an audit event is logged with status
     * #HAB_FAILURE and reason #HAB_INV_ASSERTION, together with the call
     * parameters.  See the @ref evt record documentation for details.
     *
     * @post For successful commands, no audit event is logged.
     *
     * @retval #HAB_SUCCESS on an IC not configured as #HAB_CFG_CLOSED,
     * although unsuccessful operations will still generate audit log events,
     *
     * @retval #HAB_SUCCESS on other ICs if the assertion is confirmed
     *
     * @retval #HAB_FAILURE otherwise
     */
    hab_status_t (*assert)(hab_assertion_t type, 
                           const void* data, 
                           uint32_t count);

    /** Report an event from the audit log.
     * @ingroup event
     *
     * @par Purpose
     *
     * This function allows the audit log to be interrogated.  It is intended
     * for use by post-ROM boot stage components, via the @ref rvt, to
     * determine the state of authentication operations.  This function may
     * be called outside an hab_rvt.entry() / hab_rvt.exit() pair.
     * 
     * @ifrom It is also available for use by the boot ROM, where it may be
     * used to report boot failures as part of a tethered boot
     * protocol. @endrom
     *
     * @par Operation 
     *
     * This function performs the following operations:
     * - Scans the audit log for a matching event
     * - Copies the required details to the output parameters (if found)
     *
     * @param[in] status Status level of required event.
     *
     * @param[in] index Index of required event at given status level.
     *
     * @param[out] event @ref evt record.
     *
     * @param[in,out] bytes Size of @a event buffer on entry, size of event
     * record on exit.
     *
     * @remark Use @a status = #HAB_STS_ANY to match any logged event,
     * regardless of the status value logged.
     *
     * @remark Use @a index = 0 to return the first matching event, @a index =
     * 1 to return the second matching event, and so on.
     *
     * @remark The data logged with each event is context-dependent.  Refer to
     * @ref evt record documentation.
     *
     * @warning Parameter @a bytes may not be NULL.
     *
     * @warning If the @a event buffer is a NULL pointer or too small to fit
     * the event record, the required size is written to @a bytes, but no 
     * part of the event record is copied to the output buffer.
     *
     * @retval #HAB_SUCCESS if the required event is found, and the event
     * record is copied to the output buffer.
     * 
     * @retval #HAB_SUCCESS if the required event is found and @a event buffer 
     * passed is a NULL pointer.
     *
     * @retval #HAB_FAILURE otherwise
     */
    hab_status_t (*report_event)(hab_status_t status,
                                 uint32_t index,
                                 uint8_t* event,
                                 size_t* bytes);

    /** Report security status.
     * @ingroup status
     *
     * @par Purpose
     *
     * This function reports the security configuration and state of the IC as
     * well as searching the audit log to determine the status of the boot
     * process.  It is intended for use by post-ROM boot stage components, via
     * the @ref rvt.  This function may be called outside an
     * hab_rvt.entry() / hab_rvt.exit() pair.
     * 
     * @ifrom It is also available for use by the boot ROM, and should be used
     * rather than the HAL function hab_hal_read_sec_cfg(). @endrom
     *
     * @par Operation 
     *
     * This function reads the fuses which indicate the security
     * configuration.  The fusemap varies by IC, and should be taken from the
     * @ref ref_rug.  It also uses the @ref shw state machine, if present and
     * enabled, to report on the security state.
     *
     * @param[out] config Security configuration, NULL if not required
     *
     * @param[out] state Security state, NULL if not required
     *
     * @remark If no @ref shw state machine is present and enabled, the state
     * #HAB_STATE_NONE will be output.
     *
     * @retval #HAB_SUCCESS if no warning or failure audit events have been
     * logged.
     *
     * @retval #HAB_WARNING otherwise, if only warning events have been logged.
     *
     * @retval #HAB_FAILURE otherwise
     */
    hab_status_t (*report_status)(hab_config_t* config, hab_state_t* state);

    /** Enter failsafe boot mode.
     * @ingroup safe
     *
     * @par Purpose
     *
     * This function provides a safe path when image authentication has failed
     * and all possible boot paths have been exhausted.  It is intended for
     * use by post-ROM boot stage components, via the @ref rvt.
     * 
     * @ifrom It is also available for use by the boot ROM via
     * hab_rvt.failsafe(). @endrom
     *
     * @par Operation 
     *
     * The precise details of this function vary by IC and core, and should be
     * taken from @ref ref_rug.
     *
     * @warning This function does not return.
     *
     * @remark Since this function does not return, it implicitly performs the
     * functionality of hab_rvt.exit() in order to ensure an appropriate
     * configuration of the @ref shw plugins.
     *
     * @remark Two typical implementations are:
     * - a low-level provisioning protocol in which an image is downloaded to
     * RAM from an external host, authenticated and launched.  The downloaded
     * image may communicate with tools on the external host to report the
     * reasons for boot failure, and may re-provision the end-product with
     * authentic boot images.
     * - a failsafe boot mode which does not allow execution to leave the ROM
     * until the IC is reset.
     */
    void (*failsafe)(void);
};

/** @ref rvt type
 * @ingroup rvt
 */
typedef struct hab_rvt hab_rvt_t;

/*---------------------------------------------------------------------------*/

/** @ref ivt structure
 * @ingroup ivt
 *
 * @par Format
 *
 * An @ref ivt consists of a @ref hdr followed by a list of addresses as
 * described further below.
 *
 * @warning The @a entry address may not be NULL.
 *
 * @warning On an IC not configured as #HAB_CFG_CLOSED, the
 * @a csf address may be NULL.  If it is not NULL, the @ref csf will be
 * processed, but any failures should be non-fatal.
 *
 * @warning On an IC configured as #HAB_CFG_CLOSED, the @a
 * csf address may not be NULL, and @ref csf failures are typically fatal.
 *
 * @remark The Boot Data located using the @a boot_data field is interpreted
 * by the HAB caller in a boot-mode specific manner.  This may be used by the
 * boot ROM as to determine the load address and boot device configuration for
 * images loaded from block devices (see @ref ref_rug for details).
 *
 * @remark All addresses given in the IVT, including the Boot Data (if
 * present) are those for the final load location. 
 *
 * @anchor ila
 *
 * @par Initial load addresses
 *
 * The @a self field is used to calculate addresses in boot modes where an
 * initial portion of the image is loaded to an initial location.  In such
 * cases, the IVT, Boot Data (if present) and DCD (if present) are used in
 * configuring the IC and loading the full image to its final location.  Only
 * the IVT, Boot Data (if present) and DCD (if present) are required to be
 * within the initial image portion.
 *
 * The method for calculating an initial load address for the DCD is
 * illustrated in the following C fragment.  Similar calculations apply to
 * other fields.
 *
@verbatim
        hab_ivt_t* ivt_initial = <initial IVT load address>;
        const void* dcd_initial = ivt_initial->dcd;
        if (ivt_initial->dcd != NULL)
            dcd_initial = (const uint8_t*)ivt_initial 
                          + (ivt_initial->dcd - ivt_initial->self)
@endverbatim
 */
struct hab_ivt {
    /** @ref hdr with tag #HAB_TAG_IVT, length and HAB version fields
     *  (see @ref data)
     */
    hab_hdr_t hdr;
    /** Absolute address of the first instruction to execute from the
     *  image
     */
    hab_image_entry_f entry;
    /** Reserved in this version of HAB: should be NULL. */
    const void* reserved1;
    /** Absolute address of the image DCD: may be NULL. */
    const void* dcd;
    /** Absolute address of the Boot Data: may be NULL, but not interpreted
     *  any further by HAB
     */
    const void* boot_data;
    /** Absolute address of the IVT.*/
    const void* self;
    /** Absolute address of the image CSF.*/
    const void* csf;
    /** Reserved in this version of HAB: should be zero. */
    uint32_t reserved2;
};

/** @ref ivt type
 * @ingroup ivt
 */
typedef struct hab_ivt hab_ivt_t;

/*===========================================================================
                     GLOBAL VARIABLE DECLARATIONS
=============================================================================*/

/** @ref rvt instance
 * @ingroup rvt
 */
extern const hab_rvt_t hab_rvt;

/*===========================================================================
                         FUNCTION PROTOTYPES
=============================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* HAB_H */
