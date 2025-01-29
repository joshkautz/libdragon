/**
 * @file rtc.h
 * @brief Real-Time Clock Subsystem
 * @ingroup rtc
 */
#ifndef __LIBDRAGON_RTC_H
#define __LIBDRAGON_RTC_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * @defgroup rtc Real-Time Clock Subsystem
 * @ingroup peripherals
 * @brief Real-time clock interface.
 * @author Christopher Bonhage
 *
 * The Real-Time Clock (RTC) subsystem provides a high-level interface for
 * reading and writing the real-time clock on the N64 Joybus. The Joybus RTC
 * is currently the only real-time clock supported by LibDragon. There is
 * also a real-time clock included in the N64DD and iQue hardware, which use
 * different hardware interfaces and are not yet supported by LibDragon.
 *
 * To check if a hardware real-time clock is available, call #rtc_init.
 * If no hardware real-time clock is available, the subsystem will provide
 * a software-based real-time clock for the current play session. Note that
 * this software RTC does not persist across resets or power cycles!
 *
 * Once the RTC subsystem is initialized, you can use ISO C Time functions
 * to get the current time, for example: `time(NULL)` will return the number of
 * seconds elapsed since the UNIX epoch (January 1, 1970 at 00:00:00).
 * To check if the real-time clock supports writes, call #rtc_is_persistent.
 * To write a new time to the real-time clock, use the ISO C Time function
 * `settimeofday` or call #rtc_set_time.
 *
 * This subsystem handles decoding and encoding the date/time from its internal
 * format into a standard `struct tm` structure. You can use convert between
 * `struct tm` and `time_t` using the standard C library functions `gmtime`,
 * `mktime`, and `time`.
 *
 * In anticipation of support for the 64DD and iQue real-time clocks, the
 * subsystem has APIs that allow homebrew to specify the preferred RTC source.
 * Use #rtc_is_source_available to determine if a specific RTC source can be
 * used, and #rtc_set_source to switch between available sources. The subsystem
 * will automatically resynchronize the time with the new clock when the source
 * is changed.
 *
 * Internally, Joybus RTC cannot represent dates before 1990-01-01, although
 * some RTC implementations (like UltraPIF) only support dates after
 * 2000-01-01.
 *
 * 64DD RTC only stores two digits for the year, so conventionally 96-99 are
 * treated as 1996-1999 and 00-95 are treated as 2000-2095.
 *
 * For consistency, the RTC subsystem only supports dates between 1996-2095.
 * Attempting to set the clock beyond this range will fail. RTC subsystem
 * should only be set to the "actual" date and time, and not for
 * arbitrary time manipulation. If your game uses some form of
 * time travel or real-time clock that does not match the actual
 * date/time, you should use an offset from the actual time.
 *
 * @{
 */

/** @brief RTC minimum timestamp (1996-01-01 00:00:00) */
#define RTC_TIMESTAMP_MIN 820454400
/** @brief RTC maximum timestamp (2095-12-31 23:59:59) */
#define RTC_TIMESTAMP_MAX 3976214399

/** @brief RTC source values. */
typedef enum {
    /** @brief Software RTC source */
    RTC_SOURCE_NONE = 0,
    /** @brief Joybus RTC source */
    RTC_SOURCE_JOYBUS = 1,
    /** @brief 64DD RTC source (Not implemented yet) */
    RTC_SOURCE_DD = 2,
    /** @brief iQue RTC source (Not implemented yet) */
    RTC_SOURCE_BB = 3,
} rtc_source_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief High-level convenience helper to initialize the RTC subsystem.
 *
 * The RTC subsystem depends on the libdragon Timer subsystem, so make sure
 * to call #timer_init before calling #rtc_init!
 *
 * Some flash carts require the RTC to be explicitly enabled before loading
 * the ROM file. Some emulators and flash carts do not support RTC at all.
 *
 * This function will detect if the RTC is available and if so, will
 * prepare the RTC so that the current time can be read from it.
 *
 * This operation may take up to 50 milliseconds to complete.
 *
 * This will also hook the RTC into the newlib gettimeofday and settimeofday
 * functions, so you will be able to use the ISO C time functions.
 *
 * @return whether any supported hardware RTC source was initialized
 */
bool rtc_init( void );

/**
 * @brief Close the RTC subsystem, disabling system hooks.
 */
void rtc_close( void );

/**
 * @brief Check if the specified RTC source is usable by the subsystem.
 */
bool rtc_is_source_available( rtc_source_t source );

/**
 * @brief Get the current source clock for the subsystem.
 */
rtc_source_t rtc_get_source( void );

/**
 * @brief Switch the preferred source clock for the subsystem.
 *
 * By default, the subsytem will use to the first available source,
 * but some games may wish to specify the preferred RTC source.
 *
 * Make sure you call #rtc_resync_time after switching sources!
 *
 * @return whether the new source clock was successfully selected
 */
bool rtc_set_source( rtc_source_t source );

/**
 * @brief Determine whether the RTC supports persistent writing of the time.
 *
 * Some emulators and flash carts do not support writing to the RTC, so
 * this function makes an attempt to detect silent write failures and will
 * return `false` if it is unable to change the time on the RTC.
 *
 * This function is useful if your program wants to conditionally offer the
 * ability to set the time based on hardware/emulator support.
 *
 * Unfortunately this operation may introduce a slight drift in the clock,
 * but it is the only reliable way to determine if the RTC actually persists
 * the time.
 *
 * @return whether RTC write persistence appears to be supported
 */
bool rtc_is_persistent( void );

/**************************************
 *  DEPRECATED
 **************************************/

/// @cond

/**
 * @brief Structure for storing RTC time data.
 * @deprecated Use `struct tm` and `time_t` from <time.h> instead.
 */
typedef struct rtc_time_t
{
    /** @brief Year. [1900-20XX] */
    uint16_t year;
    /** @brief Month. [0-11] */
    uint8_t month;
    /** @brief Day of month. [1-31] */
    uint8_t day;
    /** @brief Hours. [0-23] */
    uint8_t hour;
    /** @brief Minutes. [0-59] */
    uint8_t min;
    /** @brief Seconds. [0-59] */
    uint8_t sec;
    /** @brief Day of week. [0-6] (Sun-Sat) */
    uint8_t week_day;
} rtc_time_t;

__attribute__((deprecated("use rtc_is_persistent instead")))
bool rtc_is_writable( void );

__attribute__((deprecated("use rtc_get_time instead")))
bool rtc_get( rtc_time_t *rtc_time );

__attribute__((deprecated("use rtc_set_time instead")))
bool rtc_set( rtc_time_t *rtc_time );

/// @endcond

#ifdef __cplusplus
}
#endif

/** @} */ /* rtc */

#endif
