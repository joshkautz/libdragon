/**
 * @file interrupt.h
 * @brief Interrupt Controller
 * @ingroup interrupt
 */
#ifndef __LIBDRAGON_INTERRUPT_H
#define __LIBDRAGON_INTERRUPT_H

#include <stdint.h>

/**
 * @defgroup interrupt Interrupt Controller
 * @ingroup lowlevel
 * @brief N64 interrupt registering and servicing routines.
 *
 * The N64 interrupt controller provides a software interface to
 * register for interrupts from the various systems in the N64.
 * Most interrupts on the N64 coordinate through the MIPS interface
 * (MI) to allow interrupts to be handled at one spot.  A notable
 * exception is the timer interrupt which is generated by the MIPS
 * r4300 itself and not the N64 hardware.
 *
 * The interrupt controller is automatically initialized before
 * main is called. By default, all interrupts are enabled and any
 * registered callback can be called when an interrupt occurs.
 * Each of the N64-generated interrupts is maskable using the various
 * set accessors.
 *
 * Interrupts can be enabled or disabled as a whole on the N64 using
 * #enable_interrupts and #disable_interrupts.  It is assumed that
 * once the interrupt system is activated, these will always be called
 * in pairs.  Calling #enable_interrupts without first calling
 * #disable_interrupts is considered a violation of this assumption
 * and should be avoided.  Calling #disable_interrupts when interrupts
 * are already disabled will have no effect interrupts-wise
 * (but should be paired with a #enable_interrupts regardless),
 * and in that case the paired #enable_interrupts will not enable
 * interrupts either.
 * In this manner, it is safe to nest calls to disable and enable
 * interrupts.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief State of interrupts on the system
 */
typedef enum
{
    /** @brief Interrupt controller has not been initialized */
    INTERRUPTS_UNINITIALIZED,
    /** @brief Interrupts are currently disabled */
    INTERRUPTS_DISABLED,
    /** @brief Interrupts are currently enabled */
    INTERRUPTS_ENABLED
} interrupt_state_t;

/** @} */

/**
 * @brief Register an AI callback
 *
 * @param[in] callback
 *            Function to call when an AI interrupt occurs
 */
void register_AI_handler( void (*callback)() );

/**
 * @brief Register a VI callback
 *
 * @param[in] callback
 *            Function to call when a VI interrupt occurs
 */
void register_VI_handler( void (*callback)() );

/**
 * @brief Register a PI callback
 *
 * @param[in] callback
 *            Function to call when a PI interrupt occurs
 */
void register_PI_handler( void (*callback)() );

/**
 * @brief Register a DP callback
 *
 * @param[in] callback
 *            Function to call when a DP interrupt occurs
 */
void register_DP_handler( void (*callback)() );

/**
 * @brief Register a SI callback
 *
 * @param[in] callback
 *            Function to call when a SI interrupt occurs
 */
void register_SI_handler( void (*callback)() );

/**
 * @brief Register a SP callback
 *
 * @param[in] callback
 *            Function to call when a SP interrupt occurs
 */
void register_SP_handler( void (*callback)() );

/**
 * @brief Register a timer callback
 * 
 * The callback will be used when the timer interrupt is triggered by the CPU.
 * This happens when the COP0 COUNT register reaches the same value of the
 * COP0 COMPARE register.
 * 
 * This function is useful only if you want to do your own low level programming
 * of the internal CPU timer and handle the interrupt yourself. In this case,
 * also remember to activate the timer interrupt using #set_TI_interrupt.
 * 
 * @note If you use the timer library (#timer_init and #new_timer), you do not
 * need to call this function, as timer interrupt are already handled by the timer
 * library.
 *
 * @param[in] callback
 *            Function to call when a timer interrupt occurs
 */
void register_TI_handler( void (*callback)() );

/**
 * @brief Register a CART interrupt callback.
 * 
 * The callback will be called when a CART interrupt is triggered. CART interrupts
 * are interrupts triggered by devices attached to the PI bus (aka CART bus),
 * for instance the 64DD, or the modem cassette.
 * 
 * CART interrupts are disabled by default in libdragon. Use #set_CART_interrupt
 * to enable/disable them.
 * 
 * Notice that there is no generic way to acknowledge those interrupts, so if
 * you activate CART interrupts, make also sure to register an handler that
 * acknowledge them, otherwise the interrupt will deadlock the console.
 * 
 * @param[in] callback
 *            Function that should no longer be called on CART interrupts
 */
void register_CART_handler( void (*callback)() );

/**
 * @brief Register a handler that will be called when the user
 *        presses the RESET button. 
 * 
 * The N64 sends an interrupt when the RESET button is pressed,
 * and then actually resets the console after about ~500ms (but less
 * on some models, see #RESET_TIME_LENGTH).
 * 
 * Registering a handler can be used to perform a clean reset.
 * Technically, at the hardware level, it is important that the RCP
 * is completely idle when the reset happens, or it might freeze
 * and require a power-cycle to unfreeze. This means that any
 * I/O, audio, video activity must cease before #RESET_TIME_LENGTH
 * has elapsed.
 * 
 * This entry point can be used by the game code to basically
 * halts itself and stops issuing commands. Libdragon itself will
 * register handlers to halt internal modules so to provide a basic
 * good reset experience.
 * 
 * Handlers can use #exception_reset_time to read how much has passed
 * since the RESET button was pressed.
 * 
 * @param callback    Callback to invoke when the reset button is pressed.
 * 
 * @note  Reset handlers are called under interrupt.
 * 
 */
void register_RESET_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer FLASH callback
 *
 * @param[in] callback
 *            Function to call when a FLASH interrupt occurs
 */
void register_BB_FLASH_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer AES callback
 *
 * @param[in] callback
 *            Function to call when a AES interrupt occurs
 */
void register_BB_AES_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer IDE callback
 *
 * @param[in] callback
 *            Function to call when a IDE interrupt occurs
 */
void register_BB_IDE_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer PI Error callback
 *
 * @param[in] callback
 *            Function to call when a PI Error interrupt occurs
 */
void register_BB_PI_ERR_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer USB0 callback
 *
 * @param[in] callback
 *            Function to call when a USB0 interrupt occurs
 */
void register_BB_USB0_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer USB1 callback
 *
 * @param[in] callback
 *            Function to call when a USB1 interrupt occurs
 */
void register_BB_USB1_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer BTN callback
 *
 * @param[in] callback
 *            Function to call when a BTN interrupt occurs
 */
void register_BB_BTN_handler( void (*callback)() );

/**
 * @brief Register a BBPlayer MD callback
 *
 * @param[in] callback
 *            Function to call when a MD interrupt occurs
 */
void register_BB_MD_handler( void (*callback)() );

/**
 * @brief Unregister an AI callback
 *
 * @param[in] callback
 *            Function that should no longer be called on AI interrupts
 */
void unregister_AI_handler( void (*callback)() );

/**
 * @brief Unregister a VI callback
 *
 * @param[in] callback
 *            Function that should no longer be called on VI interrupts
 */
void unregister_VI_handler( void (*callback)() );

/**
 * @brief Unegister a PI callback
 *
 * @param[in] callback
 *            Function that should no longer be called on PI interrupts
 */
void unregister_PI_handler( void (*callback)() );

/**
 * @brief Unregister a DP callback
 *
 * @param[in] callback
 *            Function that should no longer be called on DP interrupts
 */
void unregister_DP_handler( void (*callback)() );

/**
 * @brief Unegister a SI callback
 *
 * @param[in] callback
 *            Function that should no longer be called on SI interrupts
 */
void unregister_SI_handler( void (*callback)() );

/**
 * @brief Unegister a SP callback
 *
 * @param[in] callback
 *            Function that should no longer be called on SP interrupts
 */
void unregister_SP_handler( void (*callback)() );

/**
 * @brief Unregister a timer callback
 *
 * @note If you use the timer library (#timer_init and #new_timer), you do not
 * need to call this function, as timer interrupt are already handled by the timer
 * library.
 *
 * @param[in] callback
 *            Function that should no longer be called on timer interrupts
 */
void unregister_TI_handler( void (*callback)() );

/**
 * @brief Unregister a CART interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on CART interrupts
 */
void unregister_CART_handler( void (*callback)() );

/**
 * @brief Unregister a RESET interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on RESET interrupts
 */
void unregister_RESET_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer FLASH interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on FLASH interrupts
 */
void unregister_BB_FLASH_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer AES interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on AES interrupts
 */
void unregister_BB_AES_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer IDE interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on IDE interrupts
 */
void unregister_BB_IDE_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer PI Error interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on PI Error interrupts
 */
void unregister_BB_PI_ERR_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer USB0 interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on USB0 interrupts
 */
void unregister_BB_USB0_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer USB1 interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on USB1 interrupts
 */
void unregister_BB_USB1_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer BTN interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on BTN interrupts
 */
void unregister_BB_BTN_handler( void (*callback)() );

/**
 * @brief Unregister a BBPlayer MD interrupt callback
 *
 * @param[in] callback
 *            Function that should no longer be called on MD interrupts
 */
void unregister_BB_MD_handler( void (*callback)() );

/**
 * @brief Enable or disable the AI interrupt
 *
 * @param[in] active
 *            Flag to specify whether the AI interrupt should be active
 */
void set_AI_interrupt( int active );

/**
 * @brief Enable or disable the VI interrupt
 *
 * The VI interrupt is generated when the VI begins displaying a specific line
 * of the display output. The line number configured always refers to the
 * final TV output, so it should be either in the range 0..524 (NTSC) or
 * 0..624 (PAL).
 * The vblank happens at the beginning of the display period, in range
 * 0..33 (NTSC) or 0..43 (PAL). A common value used to trigger the interrupt
 * at the beginning of the vblank is 2.
 *
 * In non-interlaced modes, the VI only draws on even lines, so configuring
 * the interrupt on an odd line causes the interrupt to never trigger.
 * In interlace modes, instead, the VI alternates between even lines and odd
 * lines, so any specific line will trigger an interrupt only every other
 * frame. If you need an interrupt every frame in interlaced mode, you will
 * need to reconfigure the interrupt every frame, alternating between an odd
 * and an even number.
 *
 * @param[in] active
 *            Flag to specify whether the VI interrupt should be active
 * @param[in] line
 *            The vertical line that causes this interrupt to fire.  Ignored
 *            when setting the interrupt inactive.
 *            This line number refers to the lines in the TV output,
 *            and is unrelated to the current resolution.
 */
void set_VI_interrupt( int active, unsigned long line );

/**
 * @brief Enable or disable the PI interrupt
 *
 * @param[in] active
 *            Flag to specify whether the PI interrupt should be active
 */
void set_PI_interrupt( int active );

/**
 * @brief Enable or disable the DP interrupt
 *
 * @param[in] active
 *            Flag to specify whether the DP interrupt should be active
 */
void set_DP_interrupt( int active );

/**
 * @brief Enable or disable the SI interrupt
 *
 * @param[in] active
 *            Flag to specify whether the SI interrupt should be active
 */
void set_SI_interrupt( int active );

/**
 * @brief Enable or disable the SP interrupt
 *
 * @param[in] active
 *            Flag to specify whether the SP interrupt should be active
 */
void set_SP_interrupt( int active );

/**
 * @brief Enable or disable the timer interrupt
 * 
 * @note If you use the timer library (#timer_init and #new_timer), you do not
 * need to call this function, as timer interrupt is already handled by the timer
 * library.
 *
 * @param[in] active
 *            Flag to specify whether the timer interrupt should be active
 *
 * @see #register_TI_handler
 */
void set_TI_interrupt( int active );

/**
 * @brief Enable or disable the CART interrupt
 * 
 * @param[in] active
 *            Flag to specify whether the CART interrupt should be active
 * 
 * @see #register_CART_handler
 */
void set_CART_interrupt( int active );

/**
 * @brief Enable the RESET interrupt
 * 
 * @param[in] active
 *            Flag to specify whether the RESET interrupt should be active
 * 
 * @note RESET interrupt is active by default.
 * 
 * @see #register_RESET_handler
 */
void set_RESET_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer FLASH interrupt
 *
 * @param[in] active
 *            Flag to specify whether the FLASH interrupt should be active
 */
void set_BB_FLASH_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer AES interrupt
 *
 * @param[in] active
 *            Flag to specify whether the AES interrupt should be active
 */
void set_BB_AES_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer IDE interrupt
 *
 * @param[in] active
 *            Flag to specify whether the IDE interrupt should be active
 */
void set_BB_IDE_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer PI Error interrupt
 *
 * @param[in] active
 *            Flag to specify whether the PI Error interrupt should be active
 */
void set_BB_PI_ERR_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer USB0 interrupt
 *
 * @param[in] active
 *            Flag to specify whether the USB0 interrupt should be active
 */
void set_BB_USB0_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer USB1 interrupt
 *
 * @param[in] active
 *            Flag to specify whether the USB1 interrupt should be active
 */
void set_BB_USB1_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer BTN interrupt
 *
 * @param[in] active
 *            Flag to specify whether the BTN interrupt should be active
 */
void set_BB_BTN_interrupt( int active );

/**
 * @brief Enable or disable the BBPlayer MD interrupt
 *
 * @param[in] active
 *            Flag to specify whether the MD interrupt should be active
 */
void set_BB_MD_interrupt( int active );

/** 
 * @brief Guaranteed length of the reset time.
 * 
 * This is the guaranteed length of the reset time, that is the time
 * that goes between the user pressing the reset button, and the CPU actually
 * resetting. See #exception_reset_time for more details.
 * 
 * @note The general knowledge about this is that the reset time should be
 *       500 ms. Testing on different consoles show that, while most seem to
 *       reset after 500 ms, a few EU models reset after 200ms. So we define
 *       the timer shorter for greater compatibility.
 */
#define RESET_TIME_LENGTH      TICKS_FROM_MS(200)


/** 
 * @brief Check whether the RESET button was pressed and how long we are into
 *        the reset process.
 * 
 * This function returns how many ticks have elapsed since the user has pressed
 * the RESET button, or 0 if the user has not pressed it.
 * 
 * It can be used by user code to perform actions during the RESET
 * process (see #register_RESET_handler). It is also possible to simply
 * poll this value to check at any time if the button has been pressed or not.
 * 
 * The reset process takes about 500ms between the user pressing the
 * RESET button and the CPU being actually reset, though on some consoles
 * it seems to be much less. See #RESET_TIME_LENGTH for more information.
 * For the broadest compatibility, please use #RESET_TIME_LENGTH to implement
 * the reset logic.
 * 
 * Notice also that the reset process is initiated when the user presses the
 * button, but the reset will not happen until the user releases the button.
 * So keeping the button pressed is a good way to check if the application
 * actually winds down correctly.
 * 
 * @return Ticks elapsed since RESET button was pressed, or 0 if the RESET button
 *         was not pressed.
 * 
 * @see register_RESET_handler
 * @see #RESET_TIME_LENGTH
 */
uint32_t exception_reset_time( void );

static inline __attribute__((deprecated("calling init_interrupts no longer required")))
void init_interrupts() {}

static inline __attribute__((deprecated("use register_RESET_handler instead")))
void register_reset_handler( void (*callback)() )
{
    register_RESET_handler(callback);
}


/**
 * @brief Enable interrupts systemwide
 *
 * @note If this is called inside a nested disable call, it will have no effect on the
 *       system.  Therefore it is safe to nest disable/enable calls.  After the least
 *       nested enable call, systemwide interrupts will be reenabled.
 */
void enable_interrupts();

/**
 * @brief Disable interrupts systemwide
 *
 * @note If interrupts are already disabled on the system or interrupts have not
 *       been initialized, this function will not modify the system state.
 */
void disable_interrupts();


/**
 * @brief Return the current state of interrupts
 *
 * @retval INTERRUPTS_UNINITIALIZED if the interrupt system has not been initialized yet.
 * @retval INTERRUPTS_DISABLED if interrupts have been disabled.
 * @retval INTERRUPTS_ENABLED if interrupts are currently enabled.
 */
interrupt_state_t get_interrupts_state(); 

#ifdef __cplusplus
}
#endif

#endif
