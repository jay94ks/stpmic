#ifndef __STPMIC_H__
#define __STPMIC_H__

#include <stdint.h>
#include <stddef.h>

/**
 * STPMIC driver.
 * --
 * author: jay94ks@gmail.com
 * repository: https://github.com/jay94ks/stpmic
 * --
 * Copyright(C) 2025, jay94ks.
 * License: MIT.
 * 
 * --
 * OPTIONS.
 * 1. STPMIC_USE_HAL:
 *  uses STM32's HAL library for I2C.
 * 
 * 2. STPMIC_USE_CHAN:
 *  uses `stpmic_i2c_t` structure to provide I2C functions.
 *  a pointer that is below structure must be passed to `stpmic_init` function.
 * 
 *  typedef struct stpmic_i2c_t {
 *      uint8_t (*read_i2c)(
 *          struct stpmic_i2c_t*, uint8_t addr,
 *          uint8_t* buf, uint32_t len, uint32_t timeout);
 *      uint8_t (*write_i2c)(
 *          struct stpmic_i2c_t*, uint8_t addr,
 *          uint8_t* buf, uint32_t len, uint32_t timeout);
 *  } stpmic_i2c_t;
 * 
 * 3. STPMIC_USE_CUSTOM:
 *  customize entire I2C functions. 
 *  below functions must be implemented in somewhere.
 * 
 *    uint8_t stpmic_read_i2c(uint8_t addr, uint8_t* buf, uint32_t len, uint32_t timeout);
 *    uint8_t stpmic_write_i2c(uint8_t addr, uint8_t* buf, uint32_t len, uint32_t timeout);
 * 
 * --
 * common notes for STPMIC_USE_CHAN and STPMIC_USE_CUSTOM.
 * --
 * read and write functions for these,
 * functions must return the succeed length of bytes.
 * and if expected length and returned length mismatch,
 * it'll be handled as `failure` with error code: `STPMIC_RET_TIMEOUT`.
 */

#define STPMIC_USE_HAL      1   // --> use HAL to read/write registers.
#define STPMIC_USE_CHAN     0   // --> use custom I2C channel pointer.
#define STPMIC_USE_CUSTOM   0   // --> use custom I2C channel functions.
#define STPMIC_INIT_DELAY   100 // --> initial delay settings.

#if STPMIC_USE_HAL
#include "stpmic_hal.h"

/* I2C channel. */
typedef I2C_HandleTypeDef stpmic_i2c_t;
#elif STPMIC_USE_CUSTOM
#ifdef __cplusplus
extern "C" {
#endif
/* read bytes using I2C: this should be implemented in somewhere. */
uint8_t stpmic_read_i2c(uint8_t addr, uint8_t* buf, uint32_t len, uint32_t timeout);

/* write bytes using I2C: this should be implemented in somewhere. */
uint8_t stpmic_write_i2c(uint8_t addr, uint8_t* buf, uint32_t len, uint32_t timeout);
#ifdef __cplusplus
}
#endif
#elif STPMIC_USE_CHAN
#ifdef __cplusplus
extern "C" {
#endif

/* I2C channel. */
typedef struct stpmic_i2c_t {
    /* read bytes using I2C. */
    uint8_t (*read_i2c)(
        struct stpmic_i2c_t*, uint8_t addr,
        uint8_t* buf, uint32_t len, uint32_t timeout);

    /* write bytes using I2C. */
    uint8_t (*write_i2c)(
        struct stpmic_i2c_t*, uint8_t addr,
        uint8_t* buf, uint32_t len, uint32_t timeout);
} stpmic_i2c_t;
#ifdef __cplusplus
}
#endif
#else
#error "please set 1 to one of STPMIC_USE_HAL, STPMIC_USE_CHAN and STPMIC_USE_CUSTOM."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* make a bit mask. */
#define STPMIC_BIT_MASK(nth)    (1u << (nth))

/* make a bit mask. */
#define STPMIC_BIT_MASK_VAL(nth, val)    ((val) << (nth))

/* STPMIC driver return values. */
typedef enum {
    STPMIC_RET_OK = 0,
    STPMIC_RET_INVALID,
    STPMIC_RET_NOTIMPL,
    STPMIC_RET_NOTSUP,
    STPMIC_RET_NODEV,
    STPMIC_RET_TIMEOUT,
    STPMIC_RET_ALREADY,
    STPMIC_RET_RANGE,
    STPMIC_RET_DISABLED,
    STPMIC_RET_UNKNOWN,
} stpmic_ret_t;

/* pull up or pull down. */
typedef enum {
    STPMIC_PULL_OFF     = 0,
    STPMIC_PULL_UP      = 1,
    STPMIC_PULL_DOWN    = 2,
    STPMIC_PULL_MASK    = 3,
} stpmic_pull_t;

/* polarity type. */
typedef enum {
    STPMIC_POL_ACTIVE_LOW   = 0,
    STPMIC_POL_ACTIVE_HIGH  = 1,
} stpmic_pol_t;

/* register value type. */
typedef uint8_t stpmic_reg_t;

/* register ID type. */
typedef enum {
    /* status registers, read-only. */
    STPMIC_REG_TURN_ON_SR = 0x01,
    STPMIC_REG_TURN_OFF_SR = 0x02,
    STPMIC_REG_OCP_LDOS_SR = 0x03,
    STPMIC_REG_OCP_BUCKS_BSW_SR = 0x04,
    STPMIC_REG_RESTART_SR = 0x05,
    STPMIC_REG_VERSION_SR = 0x06,

    /* control registers, writable. */
    STPMIC_REG_MAIN_CR = 0x10,
    STPMIC_REG_PADS_PULL_CR = 0x11,
    STPMIC_REG_BUCKS_PD_CR = 0x12,
    STPMIC_REG_LDO1234_PD_CR = 0x13,
    STPMIC_REG_LDO56_VREF_PD_CR = 0x14,
    STPMIC_REG_SW_VIN_CR = 0x15,
    STPMIC_REG_PKEY_TURNOFF_CR = 0x16,
    STPMIC_REG_BUCKS_MRST_CR = 0x18,
    STPMIC_REG_LDOS_MRST_CR = 0x1a,
    STPMIC_REG_WDG_CR = 0x1b,
    STPMIC_REG_WDG_TMR_CR = 0x1c,

    /* power supplies control registers, writable. */
    STPMIC_REG_BUCKx_MAIN_CR = 0x20, // --> x = 1, 2, 3, 4.
    STPMIC_REG_LDOx_MAIN_CR = 0x25,  // --> x = 1, 2, 5, 6.
    STPMIC_REG_LDO3_MAIN_CR = 0x27,
    STPMIC_REG_LDO4_MAIN_CR = 0x28,

    /* maximum registers. */
    STPMIC_REG_MAX = 0x29,

    /* aliases, branches... */
    STPMIC_REG_BUCK1_MAIN_CR = STPMIC_REG_BUCKx_MAIN_CR + 0,
    STPMIC_REG_BUCK2_MAIN_CR = STPMIC_REG_BUCKx_MAIN_CR + 1,
    STPMIC_REG_BUCK3_MAIN_CR = STPMIC_REG_BUCKx_MAIN_CR + 2,
    STPMIC_REG_BUCK4_MAIN_CR = STPMIC_REG_BUCKx_MAIN_CR + 3,
    STPMIC_REG_LDO1_MAIN_CR  = STPMIC_REG_LDOx_MAIN_CR + 0,
    STPMIC_REG_LDO2_MAIN_CR  = STPMIC_REG_LDOx_MAIN_CR + 1,
    STPMIC_REG_LDO5_MAIN_CR  = STPMIC_REG_LDOx_MAIN_CR + 4,
    STPMIC_REG_LDO6_MAIN_CR  = STPMIC_REG_LDOx_MAIN_CR + 5,
} stpmic_regid_t;

/* timeout. */
typedef struct {
    uint32_t write;
    uint32_t read;
} stpmic_timeout_t;

/* set timeout of STPMIC driver. */
void stpmic_set_timeout(stpmic_timeout_t* in);

/* get timeout of STPMIC driver. */
void stpmic_get_timeout(stpmic_timeout_t* out);

#if STPMIC_USE_CUSTOM
/**
 * initialize the STPMIC driver.
 * @param addr I2C device address, `-1` to use default value.
 * @return
 * `STPMIC_RET_ALREADY` if STPMIC driver is already initialized.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_NOTSUP` if chip's major version is not supported by driver.
 */
stpmic_ret_t stpmic_init(int16_t addr);
#else
/**
 * initialize the STPMIC driver.
 * @param dev I2C peripheral.
 * @param addr I2C device address, `-1` to use default value.
 * @return
 * `STPMIC_RET_INVALID` if `dev` is `NULL` or `addr` is bigger than `0x7fu`.
 * `STPMIC_RET_ALREADY` if STPMIC driver is already initialized.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_NOTSUP` if chip's major version is not supported by driver.
 */
stpmic_ret_t stpmic_init(stpmic_i2c_t* dev, int16_t addr);
#endif

/**
 * read a register of STPMIC without cache.
 * @param reg A register ID to read.
 * @param out A pointer to store value.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_INVALID` if register ID is bigger than `STPMIC_REG_MAX`.
 */
stpmic_ret_t stpmic_read_direct(stpmic_regid_t reg, stpmic_reg_t* out);

/**
 * write a register of STPMIC without cache.
 * @param reg A register ID to write.
 * @param val A value to write.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_INVALID` if register ID is bigger than `STPMIC_REG_MAX`.
 */
stpmic_ret_t stpmic_write_direct(stpmic_regid_t reg, stpmic_reg_t val);

/**
 * read a register of STPMIC with cache.
 * @param reg A register ID to read.
 * @param out A pointer to store value.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_INVALID` if register ID is bigger than `STPMIC_REG_MAX`.
 */
stpmic_ret_t stpmic_read(stpmic_regid_t reg, stpmic_reg_t* out);

/**
 * write a register of STPMIC with cache.
 * @param reg A register ID to write.
 * @param val A value to write.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_INVALID` if register ID is bigger than `STPMIC_REG_MAX`.
 */
stpmic_ret_t stpmic_write(stpmic_regid_t reg, stpmic_reg_t val);

/**
 * clear a register cache.
 * @param reg A register ID to clear.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_INVALID` if register ID is bigger than `STPMIC_REG_MAX`.
 */
stpmic_ret_t stpmic_clear_cache(stpmic_regid_t reg);

/**
 * reserve to write a register of STPMIC.
 * this does not write anything before `stpmic_batch_flush` is called.
 * @param reg A register ID to write.
 * @param val A value to write.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_INVALID` if register ID is bigger than `STPMIC_REG_MAX`.
 */
stpmic_ret_t stpmic_batch_write(stpmic_regid_t reg, stpmic_reg_t val);

/**
 * flush all pending batch writes.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 */
stpmic_ret_t stpmic_batch_flush();

/**
 * reload all cached registers.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 */
stpmic_ret_t stpmic_reload_cache();

/* STPMIC's version. */
typedef struct {
    uint8_t major;
    uint8_t minor;
} stpmic_version_t;

/**
 * get the version of STPMIC.
 * @param out A pointer to get the value.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 */
stpmic_ret_t stpmic_version(stpmic_version_t* out);

/* bits of TURN_ON_SR. */
enum {
    /* STPMIC1 has automatically turned ON on VIN rising. */
    STPMIC_TONSR_AUTOPWR = STPMIC_BIT_MASK(4),

    /* last Turn-ON condition was VBUS detection on SWOUT pin. */
    STPMIC_TONSR_SWOUT = STPMIC_BIT_MASK(3),
    
    /* last Turn-ON condition was VBUS detection on VBUSOTG pin. */
    STPMIC_TONSR_VBUS = STPMIC_BIT_MASK(2),
    
    /* last Turn-ON condition was WAKEUP pin detection. */
    STPMIC_TONSR_WKUP = STPMIC_BIT_MASK(1),

    /* last Turn-ON condition was PONKEYn detection. */
    STPMIC_TONSR_PONKEY = STPMIC_BIT_MASK(0),
};

/* get the TURN_ON_SR register value. */
static inline stpmic_ret_t stpmic_tonsr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_TURN_ON_SR, out);
}

/* bits of TURN_OFF_SR. */
enum {
    /* Last turn-OFF condition was due to PONKEYn long key. */
    STPMIC_TOFSR_PKEYLKP = STPMIC_BIT_MASK(5),

    /* Last turn-OFF condition was due to watchdog. */
    STPMIC_TOFSR_WDG = STPMIC_BIT_MASK(4),

    /* Last turn-ON condition was due to overcurrent protection. */
    STPMIC_TOFSR_OCP = STPMIC_BIT_MASK(3),

    /* Last turn-OFF condition was due to thermal shutdown. */
    STPMIC_TOFSR_THSD = STPMIC_BIT_MASK(2),

    /* Last turn-OFF condition was due to VIN below VINOK_Fall. */
    STPMIC_TOFSR_VINOK_FA = STPMIC_BIT_MASK(1),

    /*  Last turn-OFF condition was due to software switch OFF. */
    STPMIC_TOFSR_SWOFF = STPMIC_BIT_MASK(0),
};

/* get the TURN_OFF_SR register value. */
static inline stpmic_ret_t stpmic_tofsr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_TURN_OFF_SR, out);
}

/* bits of N'th LDO in OCP_LDOS_SR, nth: 1 ~ 6. */
#define STPMIC_OCPLDOSR(nth)    (1u << ((nth) - 1))

/* bits of OCP_LDOS_SR. */
enum {
    /* Last turn-OFF was due to overcurrent protection on N'th LDO. */
    STPMIC_OCPLDOSR_6 = STPMIC_OCPLDOSR(6),
    STPMIC_OCPLDOSR_5 = STPMIC_OCPLDOSR(5),
    STPMIC_OCPLDOSR_4 = STPMIC_OCPLDOSR(4),
    STPMIC_OCPLDOSR_3 = STPMIC_OCPLDOSR(3),
    STPMIC_OCPLDOSR_2 = STPMIC_OCPLDOSR(2),
    STPMIC_OCPLDOSR_1 = STPMIC_OCPLDOSR(1),
};

/* get the OCP_LDOS_SR register value. */
static inline stpmic_ret_t stpmic_ocpldosr(stpmic_reg_t* out) {
    return stpmic_read_direct(STPMIC_REG_OCP_LDOS_SR, out);
}

/* bits of N'th LDO in OCP_BUCKS_BSW_SR, nth: 1 ~ 7. */
#define STPMIC_OCPBUCKSBSW(nth)    (1u << ((nth) - 1))

/* bits of OCP_BUCKS_BSW_SR. */
enum {
    STPMIC_OCPBUCKSBSW_BOOST    = STPMIC_OCPBUCKSBSW(7),
    STPMIC_OCPBUCKSBSW_SWOUT    = STPMIC_OCPBUCKSBSW(6),
    STPMIC_OCPBUCKSBSW_VBUSOTG  = STPMIC_OCPBUCKSBSW(5),
    STPMIC_OCPBUCKSBSW_BUCK4    = STPMIC_OCPBUCKSBSW(4),
    STPMIC_OCPBUCKSBSW_BUCK3    = STPMIC_OCPBUCKSBSW(3),
    STPMIC_OCPBUCKSBSW_BUCK2    = STPMIC_OCPBUCKSBSW(2),
    STPMIC_OCPBUCKSBSW_BUCK1    = STPMIC_OCPBUCKSBSW(1),
};

/* get the OCP_BUCKS_BSW_SR register value. */
static inline stpmic_ret_t stpmic_ocpbucksbsw(stpmic_reg_t* out) {
    return stpmic_read_direct(STPMIC_REG_OCP_LDOS_SR, out);
}

/* bits of RESTART_SR. */
enum {
    /**
     * Operating mode.
     * Signal if the STPMIC1 is in MAIN mode or ALTERNATE mode.
     * 0: MAIN, 1: ALTERNATIVE.
     */
    STPMIC_RESTARTSR_OP_MODE_MASK       = STPMIC_BIT_MASK(7),
    STPMIC_RESTARTSR_OP_MODE_MAIN       = STPMIC_BIT_MASK_VAL(7, 0u),
    STPMIC_RESTARTSR_OP_MODE_ALTERNATIVE= STPMIC_BIT_MASK_VAL(7, 1u),

    /**
     * LDO4 input source mask.
     * Provides status of LDO4 input switch selection.
     * 00: LDO4 is off.
     * 01: VIN supply selected.
     * 10: VBUSOTG supply selected.
     * 11: BSTOUT supply selected.
     */
    STPMIC_RESTARTSR_LDO4_SRC_MASK      = STPMIC_BIT_MASK(6) | STPMIC_BIT_MASK(5),
    STPMIC_RESTARTSR_LDO4_SRC_OFF       = STPMIC_BIT_MASK_VAL(5, 0u),
    STPMIC_RESTARTSR_LDO4_SRC_VIN       = STPMIC_BIT_MASK_VAL(5, 1u),
    STPMIC_RESTARTSR_LDO4_SRC_VBUSOTG   = STPMIC_BIT_MASK_VAL(5, 2u),
    STPMIC_RESTARTSR_LDO4_SRC_BSTOUT    = STPMIC_BIT_MASK_VAL(5, 3u),

    /* Restart is due to VINOK_Fall turn-OFF condition while RREQ_EN bit is set. */
    STPMIC_RESTARTSR_VINOK_FA           = STPMIC_BIT_MASK(4),

    /* Restart is due to PONKEYn long key press turn- OFF condition while RREQ_EN bit is set. */
    STPMIC_RESTARTSR_PKEYLKP            = STPMIC_BIT_MASK(3),
    
    /* Restart is due to watchdog turn-OFF condition while RREQ_EN bit is set. */
    STPMIC_RESTARTSR_WDG                = STPMIC_BIT_MASK(2),
    
    /* Restart is due to SWOFF turn-OFF condition while RREQ_EN bit is set. */
    STPMIC_RESTARTSR_SWOUT              = STPMIC_BIT_MASK(1),

    /* Restart is due to RSTn signal asserted by application processor. */
    STPMIC_RESTARTSR_RSTn               = STPMIC_BIT_MASK(0),
};

/* get the RESTART_SR register value. */
static inline stpmic_ret_t stpmic_restartsr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_RESTART_SR, out);
}

/* STPMIC's operating mode.*/
enum {
    STPMIC_OPMODE_MAIN          = STPMIC_RESTARTSR_OP_MODE_MAIN,
    STPMIC_OPMODE_ALTERNATIVE   = STPMIC_RESTARTSR_OP_MODE_ALTERNATIVE,
    STPMIC_OPMODE_UNKNOWN       = 0xffu,
};

/* get the STPMIC's operating mode. */
uint8_t stpmic_opmode_is_main();

/* LDO4's input source. */
typedef enum {
    STPMIC_LDO4SRC_OFF          = STPMIC_RESTARTSR_LDO4_SRC_OFF,
    STPMIC_LDO4SRC_VIN          = STPMIC_RESTARTSR_LDO4_SRC_VIN,
    STPMIC_LDO4SRC_VBUSOTG      = STPMIC_RESTARTSR_LDO4_SRC_VBUSOTG,
    STPMIC_LDO4SRC_BSTOUT       = STPMIC_RESTARTSR_LDO4_SRC_BSTOUT,
    STPMIC_LDO4SRC_UNKNOWN      = 0xffu,
} stpmic_ldo4src_t;

/* get the LDO4'th input source. */
uint8_t stpmic_ldo4_src(stpmic_reg_t in);

/* bits of MAIN_CR. */
enum {
    /**
     * OCP_OFF_DBG: Used as software debug bit to emulate OCP turn-OFF event generation.
     * OCP flags coming from any regulators are bypassed when this bit is set.
     * 0: OCP event is generated based on flags from regulators.
     * 1: OCP turn-OFF event is generated.
     */
    STPMIC_MAINCR_OCP_OFF_DBG   = STPMIC_BIT_MASK(4),

    /**
     * specifies PWRCTRL pin polarity.
     * 0: active low, 1: active high. 
     */
    STPMIC_MAINCR_PWRCTL_POL    = STPMIC_BIT_MASK(3),

    /**
     * enable PWRCTRL functionality.
     * 0: disabled, 1: enabled.
     */
    STPMIC_MAINCR_PWRCTL_EN     = STPMIC_BIT_MASK(2),

    /**
     * allows power cycling on turn-OFF condition.
     * 0: power cycling is performed only on RSTn assertion by the application processor.
     * 1: Power cycling is performed on turn-OFF condition and on RSTn assertion by the application processor.
     */
    STPMIC_MAINCR_RREQ_EN       = STPMIC_BIT_MASK(1),

    /**
     * Software switch OFF bit.
     * 0: no effect.
     * 1: switch-OFF requested (POWER_DOWN starts immediately).
     */
    STPMIC_MAINCR_SWOFF         = STPMIC_BIT_MASK(0),
};

/* get the MAIN_CR register value. */
static inline stpmic_ret_t stpmic_maincr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_MAIN_CR, out);
}

/* request `software switch` off. */
stpmic_ret_t stpmic_request_swoff();

/* bits of PADS_PULL_CR. */
enum {
    /**
     * Enable WAKEUP detector. 
     * 0: enabled, 1: disabled.
     */
    STPMIC_PADSPULLCR_WKUP_EN       = STPMIC_BIT_MASK(4),

    /**
     * PWRCTRL_PD: PWRCTRL pull-down control
     * 0: PD inactive, 1: PD active.
     * Note: this bit has higher priority than PWRCTRL_PU.
     */
    STPMIC_PADSPULLCR_PWRCTRL_PD    = STPMIC_BIT_MASK(3),

    /**
     * PWRCTRL_PU: PWRCTRL pull-up control
     * 0: PU inactive, 1: PU active.
     */
    STPMIC_PADSPULLCR_PWRCTRL_PU    = STPMIC_BIT_MASK(2),

    /**
     * WKUP_PD: WAKEUP pull-down control (reverse logic)
     * 0: PD active, 1: PD not active.
     */
    STPMIC_PADSPULLCR_WKUP_PD       = STPMIC_BIT_MASK(1),

    /**
     * PKEY_PU: PONKEY pull-up control (reverse logic)
     * 0: PU active, 1: PU not active.
     */
    STPMIC_PADSPULLCR_WKUP_PU       = STPMIC_BIT_MASK(0),
};

/* get the PADS_PULL_CR register value. */
static inline stpmic_ret_t stpmic_padspullcr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_PADS_PULL_CR, out);
}

/* structure for initializing PWRCTRL pin. */
typedef struct {
    stpmic_pol_t    pol;
    stpmic_pull_t   pull;
    uint8_t         en;
} stpmic_pwrctrl_t;

/* initialize the PWRCTRL pin's functionality. */
stpmic_ret_t stpmic_pwrctrl_init(stpmic_pwrctrl_t* pwrctrl);

/* de-initialize PWRCTRL functionality. */
stpmic_ret_t stpmic_pwrctrl_deinit();

/* structure for initializing WAKE-UP pin. */
typedef struct {
    stpmic_pull_t   pull;
    uint8_t         en;
} stpmic_wakeup_t;

/* initialize the WAKE-UP pin's functionality. */
stpmic_ret_t stpmic_wakeup_init(stpmic_wakeup_t* wakeup);

/* de-initialize the WAKE-UP pin's functionality. */
stpmic_ret_t stpmic_wakeup_deinit();

/**
 * BUCK's discharge pull-down mode. 
 * 0, 00: light PD active when ENA of BuckN = 0
 * 1, 01: high PD active when ENA of BuckN = 0
 * 2, 10: light and high PD forced inactive
 * 3, 11: light PD forced active
 */
typedef enum {
    STPMIC_BUCKSPD_AUTO_LIGHT = 0,
    STPMIC_BUCKSPD_AUTO_HIGH,
    STPMIC_BUCKSPD_FORCED_INACTIVE,
    STPMIC_BUCKSPD_FORCED_ACTIVE,
} stpmic_buckspd_t;

/* bits of BUCKS_PD_CR. */
enum {
    STPMIC_BUCKSPD_MASK_BUCK4 = STPMIC_BIT_MASK_VAL(6, 0x03u),
    STPMIC_BUCKSPD_MASK_BUCK3 = STPMIC_BIT_MASK_VAL(4, 0x03u),
    STPMIC_BUCKSPD_MASK_BUCK2 = STPMIC_BIT_MASK_VAL(2, 0x03u),
    STPMIC_BUCKSPD_MASK_BUCK1 = STPMIC_BIT_MASK_VAL(0, 0x03u),
};

/* get the BUCKS_PD_CR register value. */
static inline stpmic_ret_t stpmic_buckspd(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_BUCKS_PD_CR, out);
}

/* encode BUCKS_PD_CR bits and set. */
static inline stpmic_reg_t stpmic_set_buckspd(stpmic_reg_t org, uint8_t nth, stpmic_buckspd_t val) {
    if (nth <= 0 || nth > 4) {
        return 0;
    }

    // --> erase old bits, and set new bits, then return it.
    org &= ~STPMIC_BIT_MASK_VAL((nth - 1) << 1, 0x03u);
    return org | STPMIC_BIT_MASK_VAL((nth - 1) << 1, val);
}

/**
 * LDO's discharge pull-down mode. 
 * 0, 00: PD active when ENA of LDOn = 0
 * 1, 01: PD forced inactive
 * 2, 10: PD forced inactive
 * 3, 11: PD forced active
 */
typedef enum {
    STPMIC_LDOSPD_ACTIVE = 0,
    STPMIC_LDOSPD_FORCED_INACTIVE_1 = 1,
    STPMIC_LDOSPD_FORCED_INACTIVE_2 = 2,
    STPMIC_LDOSPD_FORCED_ACTIVE = 3,
} stpmic_ldospd_t;

/* bits of LDO14_PD_CR. */
enum {
    STPMIC_LDO1234PD_LDO4 = STPMIC_BIT_MASK_VAL(6, 0x03u),
    STPMIC_LDO1234PD_LDO3 = STPMIC_BIT_MASK_VAL(4, 0x03u),
    STPMIC_LDO1234PD_LDO2 = STPMIC_BIT_MASK_VAL(2, 0x03u),
    STPMIC_LDO1234PD_LDO1 = STPMIC_BIT_MASK_VAL(0, 0x03u),
};

/* get the LDO14_PD_CR register value. */
static inline stpmic_ret_t stpmic_ldo1234pd(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_LDO1234_PD_CR, out);
}

/* encode LDO12344_PD_CR bits and set. */
static inline stpmic_reg_t stpmic_set_ldo1234pd(stpmic_reg_t org, uint8_t nth, stpmic_ldospd_t val) {
    if (nth <= 0 || nth > 4) {
        return 0;
    }

    // --> erase old bits, and set new bits, then return it.
    org &= ~STPMIC_BIT_MASK_VAL((nth - 1) << 1, 0x03u);
    return org | STPMIC_BIT_MASK_VAL((nth - 1) << 1, val);
}

/* bits of LDO56_VREF_PD_CR. */
enum {
    /**
     * 0: PD active when BST_ON = 0
     * 1: PD inactive when BST_ON = 0
     */
    STPMIC_LDO56PD_BST = STPMIC_BIT_MASK(6),

    /**
     * 00: PD active only when each LDO is disabled
     * 01: PD forced inactive
     * 10: PD forced inactive
     * 11: PD forced active
     */
    STPMIC_LDO56PD_REFDDR = STPMIC_BIT_MASK_VAL(4, 0x03u),
    STPMIC_LDO56PD_LDO6 = STPMIC_BIT_MASK_VAL(2, 0x03u),
    STPMIC_LDO56PD_LDO5 = STPMIC_BIT_MASK_VAL(0, 0x03u),
};

/* get the LDO56_VREF_PD_CR register value. */
static inline stpmic_ret_t stpmic_ldo56pd(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_LDO56_VREF_PD_CR, out);
}

/* encode LDO56_PD_CR bits and set, nth: 5 ~ 7, 7: REFDDR. */
static inline stpmic_reg_t stpmic_set_ldo56pd(stpmic_reg_t org, uint8_t nth, stpmic_ldospd_t val) {
    if (nth < 5 || nth > 7) {
        return 0;
    }

    // --> erase old bits, and set new bits, then return it.
    org &= ~STPMIC_BIT_MASK_VAL((nth - 5) << 1, 0x03u);
    return org | STPMIC_BIT_MASK_VAL((nth - 5) << 1, val);
}

/* encode LDO56_PD_CR bits for BST and set. */
static inline stpmic_reg_t stpmic_set_ldo56pd_bst(stpmic_reg_t org, uint8_t enable) {
    // --> erase old bits, and set new bits, then return it.
    org &= ~STPMIC_LDO56PD_BST;
    return org | (enable ? STPMIC_LDO56PD_BST : 0);
}

/* `VINLOW_HYST` bits of SW_VIN_CR. */
typedef enum {
    STPMIC_VINHYST_100mV = 0,
    STPMIC_VINHYST_200mV = 1,
    STPMIC_VINHYST_300mV = 2,
    STPMIC_VINHYST_400mV = 3,
} stpmic_vinhyst_t;

/* `VINLOW_TRESH` bits of SW_VIN_CR. */
typedef enum {
    STPMIC_VINTRESH_50mV = 0,
    STPMIC_VINTRESH_100mV = 1,
    STPMIC_VINTRESH_150mV = 2,
    STPMIC_VINTRESH_200mV = 3,
    STPMIC_VINTRESH_250mV = 4,
    STPMIC_VINTRESH_300mV = 5,
    STPMIC_VINTRESH_350mV = 6,
    STPMIC_VINTRESH_400mV = 7,
} stpmic_vintresh_t;

/* bits of SW_VIN_CR. */
enum {
    STPMIC_SWVINCR_SWIN_DET_EN = STPMIC_BIT_MASK(7),
    STPMIC_SWVINCR_SWOUT_DET_DIS = STPMIC_BIT_MASK(6),
    STPMIC_SWVINCR_HYST = STPMIC_BIT_MASK_VAL(4, 0x03u),
    STPMIC_SWVINCR_TRESH = STPMIC_BIT_MASK_VAL(1, 0x07u),
    STPMIC_SWVINCR_MON = STPMIC_BIT_MASK(0),
};

/* get the SW_VIN_CR register value. */
static inline stpmic_ret_t stpmic_swvincr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_SW_VIN_CR, out);
}

/* bits of PKEY_TURNOFF_CR. */
enum {
    /**
     * PKEY_LKP_OFF:
     * 0: Turn OFF on long key press inactive.
     * 1: Turn OFF on long key press active.
     */
    STPMIC_PKEYTOFCR_LKP_OFF = STPMIC_BIT_MASK(7),

    /**
     * PKEY_CLEAR_OCP_FLAG:
     * 0: only VIN_POR_Fall can reset LOCK_OCP_FLAG internal signal.
     * 1: if PONKEYn pin is pressed for more than PKEY_LKP_TMR[3:0] 
     * then LOCK_OCP_FLAG is cleared. This also results as turn-ON condition.
     */
    STPMIC_PKEYTOFCR_CLR_OCP = STPMIC_BIT_MASK(6),

    /**
     * PONKEYn long key press duration.
     * 0x00: 16s, 0x0f: 1s.
     * formular: (16 - tmr). - seconds.
     */
    STPMIC_PKEYTOFCR_TMR_MASK = STPMIC_BIT_MASK_VAL(0, 0x0fu),
};

/* get the PKEY_TURNOFF_CR register value. */
static inline stpmic_ret_t stpmic_pkeytofcr(stpmic_reg_t* out) {
    return stpmic_read(STPMIC_REG_PKEY_TURNOFF_CR, out);
}

/**
 * bits of BUCKS_MRST_CR and LDOS_MRST_CR. 
 * used to mask `reset` of each registers on next NRST cycle.
 */
enum {
    /* BUCKS_MRST_CR, bits index: (n - 8). */
    STPMIC_MRST_BUCK4 = STPMIC_BIT_MASK(11),
    STPMIC_MRST_BUCK3 = STPMIC_BIT_MASK(10),
    STPMIC_MRST_BUCK2 = STPMIC_BIT_MASK(9),
    STPMIC_MRST_BUCK1 = STPMIC_BIT_MASK(8),
    
    /* LDOS_MRST_CR, bits index: as-is. */
    STPMIC_MRST_REFDDR = STPMIC_BIT_MASK(6),
    STPMIC_MRST_LDO6 = STPMIC_BIT_MASK(5),
    STPMIC_MRST_LDO5 = STPMIC_BIT_MASK(4),
    STPMIC_MRST_LDO4 = STPMIC_BIT_MASK(3),
    STPMIC_MRST_LDO3 = STPMIC_BIT_MASK(2),
    STPMIC_MRST_LDO2 = STPMIC_BIT_MASK(1),
    STPMIC_MRST_LDO1 = STPMIC_BIT_MASK(0),
};

/* get the MRST masks from BUCKS_MRST_CR and LDOS_MRST_CR. */
stpmic_ret_t stpmic_mrst(uint16_t* out);

/* set the MRST masks from BUCKS_MRST_CR and LDOS_MRST_CR. */
stpmic_ret_t stpmic_set_mrst(uint16_t val);

/* watchdog initialization parameters. */
typedef struct {
    uint8_t sec;    // --> (sec + 1) s.
    uint8_t enable;
} stpmic_watchdog_t;

/* deinitialize the watchdog timer. */
static inline stpmic_ret_t stpmic_watchdog_deinit() {
    /* (MSB) RESV | RST(1) | ENA(1) (LSB). */
    return stpmic_write_direct(STPMIC_REG_WDG_CR, (1u << 1) | (0u << 0));
}

/* initialize the watchdog timer, sec: 1 ~ 255, 0: disable. */
stpmic_ret_t stpmic_watchdog_init(uint8_t sec);

/* reset the watchdog counter to default counter. */
stpmic_ret_t stpmic_watchdog_reset();

/**
 * Power regulation mode.
 * STPMIC_PREGMODE_HIGH: High power mode, HP.
 * STPMIC_PREGMODE_LOW: Low power mode, LP.
 */
typedef enum {
    STPMIC_PREGMODE_HIGH = 0,
    STPMIC_PREGMODE_LOW,
} stpmic_pregmode_t;

/* voltage configuration for BUCK #1. */
typedef enum {
    STPMIC_BUCK1VOLTS_0V725 = 5,
    STPMIC_BUCK1VOLTS_0V75 = 6,
    STPMIC_BUCK1VOLTS_0V775 = 7,
    STPMIC_BUCK1VOLTS_0V8 = 8,
    STPMIC_BUCK1VOLTS_0V825 = 9,
    STPMIC_BUCK1VOLTS_0V85 = 10,
    STPMIC_BUCK1VOLTS_0V875 = 11,
    STPMIC_BUCK1VOLTS_0V9 = 12,
    STPMIC_BUCK1VOLTS_0V925 = 13,
    STPMIC_BUCK1VOLTS_0V95 = 14,
    STPMIC_BUCK1VOLTS_0V975 = 15,
    STPMIC_BUCK1VOLTS_1V0 = 16,
    STPMIC_BUCK1VOLTS_1V025 = 17,
    STPMIC_BUCK1VOLTS_1V05 = 18,
    STPMIC_BUCK1VOLTS_1V075 = 19,
    STPMIC_BUCK1VOLTS_1V1 = 20,
    STPMIC_BUCK1VOLTS_1V125 = 21,
    STPMIC_BUCK1VOLTS_1V15 = 22,
    STPMIC_BUCK1VOLTS_1V175 = 23,
    STPMIC_BUCK1VOLTS_1V2 = 24,
    STPMIC_BUCK1VOLTS_1V225 = 25,
    STPMIC_BUCK1VOLTS_1V25 = 26,
    STPMIC_BUCK1VOLTS_1V275 = 27,
    STPMIC_BUCK1VOLTS_1V3 = 28,
    STPMIC_BUCK1VOLTS_1V325 = 29,
    STPMIC_BUCK1VOLTS_1V35 = 30,
    STPMIC_BUCK1VOLTS_1V375 = 31,
    STPMIC_BUCK1VOLTS_1V4 = 32,
    STPMIC_BUCK1VOLTS_1V425 = 33,
    STPMIC_BUCK1VOLTS_1V45 = 34,
    STPMIC_BUCK1VOLTS_1V475 = 35,
    STPMIC_BUCK1VOLTS_1V5 = 36,
} stpmic_buck1volts_t;

/* voltage configuration for BUCK #2. */
typedef enum {
    STPMIC_BUCK2VOLTS_1V0   = 17,
    STPMIC_BUCK2VOLTS_1V05  = 19,
    STPMIC_BUCK2VOLTS_1V1   = 21,
    STPMIC_BUCK2VOLTS_1V15  = 23,
    STPMIC_BUCK2VOLTS_1V2   = 25,
    STPMIC_BUCK2VOLTS_1V25  = 27,
    STPMIC_BUCK2VOLTS_1V3   = 29,
    STPMIC_BUCK2VOLTS_1V35  = 31,
    STPMIC_BUCK2VOLTS_1V4   = 33,
    STPMIC_BUCK2VOLTS_1V45  = 35,
    STPMIC_BUCK2VOLTS_1V5   = 36,
} stpmic_buck2volts_t;

/* voltage configuration for BUCK #3. */
typedef enum {
    STPMIC_BUCK3VOLTS_1V0   = 19,
    STPMIC_BUCK3VOLTS_1V1   = 23,
    STPMIC_BUCK3VOLTS_1V2   = 27,
    STPMIC_BUCK3VOLTS_1V3   = 31,
    STPMIC_BUCK3VOLTS_1V4   = 35,
    STPMIC_BUCK3VOLTS_1V5   = 36,
    STPMIC_BUCK3VOLTS_1V6   = 37,
    STPMIC_BUCK3VOLTS_1V7   = 38,
    STPMIC_BUCK3VOLTS_1V8   = 39,
    STPMIC_BUCK3VOLTS_1V9   = 40,
    STPMIC_BUCK3VOLTS_2V0   = 41,
    STPMIC_BUCK3VOLTS_2V1   = 42,
    STPMIC_BUCK3VOLTS_2V2   = 43,
    STPMIC_BUCK3VOLTS_2V3   = 44,
    STPMIC_BUCK3VOLTS_2V4   = 45,
    STPMIC_BUCK3VOLTS_2V5   = 46,
    STPMIC_BUCK3VOLTS_2V6   = 47,
    STPMIC_BUCK3VOLTS_2V7   = 48,
    STPMIC_BUCK3VOLTS_2V8   = 49,
    STPMIC_BUCK3VOLTS_2V9   = 50,
    STPMIC_BUCK3VOLTS_3V0   = 51,
    STPMIC_BUCK3VOLTS_3V1   = 52,
    STPMIC_BUCK3VOLTS_3V2   = 53,
    STPMIC_BUCK3VOLTS_3V3   = 54,
    STPMIC_BUCK3VOLTS_3V4   = 55,
} stpmic_buck3volts_t;

/* voltage configuration for BUCK #4. */
typedef enum {
    STPMIC_BUCK4VOLTS_0V6   = 0,
    STPMIC_BUCK4VOLTS_0V625 = 1,
    STPMIC_BUCK4VOLTS_0V65  = 2,
    STPMIC_BUCK4VOLTS_0V675 = 3,
    STPMIC_BUCK4VOLTS_0V7   = 4,
    STPMIC_BUCK4VOLTS_0V725 = 5,
    STPMIC_BUCK4VOLTS_0V75  = 6,
    STPMIC_BUCK4VOLTS_0V775 = 7,
    STPMIC_BUCK4VOLTS_0V8   = 8,
    STPMIC_BUCK4VOLTS_0V825 = 9,
    STPMIC_BUCK4VOLTS_0V85  = 10,
    STPMIC_BUCK4VOLTS_0V875 = 11,
    STPMIC_BUCK4VOLTS_0V9   = 12,
    STPMIC_BUCK4VOLTS_0V925 = 13,
    STPMIC_BUCK4VOLTS_0V95  = 14,
    STPMIC_BUCK4VOLTS_0V975 = 15,
    STPMIC_BUCK4VOLTS_1V0   = 16,
    STPMIC_BUCK4VOLTS_1V025 = 17,
    STPMIC_BUCK4VOLTS_1V05  = 18,
    STPMIC_BUCK4VOLTS_1V075 = 19,
    STPMIC_BUCK4VOLTS_1V1   = 20,
    STPMIC_BUCK4VOLTS_1V125 = 21,
    STPMIC_BUCK4VOLTS_1V15  = 22,
    STPMIC_BUCK4VOLTS_1V175 = 23,
    STPMIC_BUCK4VOLTS_1V2   = 24,
    STPMIC_BUCK4VOLTS_1V225 = 25,
    STPMIC_BUCK4VOLTS_1V25  = 26,
    STPMIC_BUCK4VOLTS_1V275 = 27,
    STPMIC_BUCK4VOLTS_1V3   = 29,
    STPMIC_BUCK4VOLTS_1V35  = 31,
    STPMIC_BUCK4VOLTS_1V4   = 33,
    STPMIC_BUCK4VOLTS_1V45  = 35,
    STPMIC_BUCK4VOLTS_1V5   = 36,
    STPMIC_BUCK4VOLTS_1V6   = 37,
    STPMIC_BUCK4VOLTS_1V7   = 38,
    STPMIC_BUCK4VOLTS_1V8   = 39,
    STPMIC_BUCK4VOLTS_1V9   = 40,
    STPMIC_BUCK4VOLTS_2V0   = 41,
    STPMIC_BUCK4VOLTS_2V1   = 42,
    STPMIC_BUCK4VOLTS_2V2   = 43,
    STPMIC_BUCK4VOLTS_2V3   = 44,
    STPMIC_BUCK4VOLTS_2V4   = 45,
    STPMIC_BUCK4VOLTS_2V5   = 46,
    STPMIC_BUCK4VOLTS_2V6   = 47,
    STPMIC_BUCK4VOLTS_2V7   = 48,
    STPMIC_BUCK4VOLTS_2V8   = 49,
    STPMIC_BUCK4VOLTS_2V9   = 50,
    STPMIC_BUCK4VOLTS_3V0   = 51,
    STPMIC_BUCK4VOLTS_3V1   = 52,
    STPMIC_BUCK4VOLTS_3V2   = 53,
    STPMIC_BUCK4VOLTS_3V3   = 54,
    STPMIC_BUCK4VOLTS_3V4   = 55,
    STPMIC_BUCK4VOLTS_3V5   = 56,
    STPMIC_BUCK4VOLTS_3V6   = 57,
    STPMIC_BUCK4VOLTS_3V7   = 58,
    STPMIC_BUCK4VOLTS_3V8   = 59,
    STPMIC_BUCK4VOLTS_3V9   = 60,
} stpmic_buck4volts_t;

/* buck parameters. */
typedef struct {
    stpmic_buckspd_t pd;    // --> pull-down configuration.
    stpmic_pregmode_t mode; // --> regulation mode.
    
    uint8_t volts;
    uint8_t enable;
} stpmic_buck_t;

/* get the PKEY_TURNOFF_CR register value. */
static inline stpmic_ret_t stpmic_buck_main_cr(uint8_t nth, stpmic_reg_t* out) {
    if (nth <= 0 || nth > 4) {
        return STPMIC_RET_RANGE;
    }

    return stpmic_read(
        (stpmic_regid_t)(STPMIC_REG_BUCKx_MAIN_CR + (nth - 1)),
        out);
}

/**
 * setup the specified buck converter.
 * @param nth 1 ~ 4.
 * @param opts options to set.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t stpmic_buck_setup(uint8_t nth, stpmic_buck_t* opts);

/**
 * enable the specified buck converter.
 * @param nth 1 ~ 4.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t stpmic_buck_enable(uint8_t nth);

/**
 * disable the specified buck converter.
 * @param nth 1 ~ 4.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t stpmic_buck_disable(uint8_t nth);

/* LDO parameters. */
typedef struct {
    stpmic_ldospd_t pd;     // --> pull-down configuration.
    uint8_t enable;
    
    /* LDO #1, #2, #3, #5, #6. */
    uint8_t volts;

    /* LDO #3. */
    uint8_t bypass;

    /* LDO #4, input source. */
    stpmic_ldo4src_t src;
} stpmic_ldo_t;


/* LDO voltages. */
typedef enum {
    STPMIC_LDO123VOLTS_1V7 = 8,
    STPMIC_LDO123VOLTS_1V8 = 9,
    STPMIC_LDO123VOLTS_1V9 = 10,
    STPMIC_LDO123VOLTS_2V0 = 11,
    STPMIC_LDO123VOLTS_2V1 = 12,
    STPMIC_LDO123VOLTS_2V2 = 13,
    STPMIC_LDO123VOLTS_2V3 = 14,
    STPMIC_LDO123VOLTS_2V4 = 15,
    STPMIC_LDO123VOLTS_2V5 = 16,
    STPMIC_LDO123VOLTS_2V6 = 17,
    STPMIC_LDO123VOLTS_2V7 = 18,
    STPMIC_LDO123VOLTS_2V8 = 19,
    STPMIC_LDO123VOLTS_2V9 = 20,
    STPMIC_LDO123VOLTS_3V0 = 21,
    STPMIC_LDO123VOLTS_3V1 = 22,
    STPMIC_LDO123VOLTS_3V2 = 23,
    STPMIC_LDO123VOLTS_3V3 = 24,

    /* special value for LDO3: VOUT 2/2 (sink/source). */
    STPMIC_LDO3VOLTS_VOUT_22 = 31,
} stpmic_ldo123volts_t;

/* LDO voltages. */
typedef enum {
    STPMIC_LDO5VOLTS_1V7 = 8,
    STPMIC_LDO5VOLTS_1V8 = 9,
    STPMIC_LDO5VOLTS_1V9 = 10,
    STPMIC_LDO5VOLTS_2V0 = 11,
    STPMIC_LDO5VOLTS_2V1 = 12,
    STPMIC_LDO5VOLTS_2V2 = 13,
    STPMIC_LDO5VOLTS_2V3 = 14,
    STPMIC_LDO5VOLTS_2V4 = 15,
    STPMIC_LDO5VOLTS_2V5 = 16,
    STPMIC_LDO5VOLTS_2V6 = 17,
    STPMIC_LDO5VOLTS_2V7 = 18,
    STPMIC_LDO5VOLTS_2V8 = 19,
    STPMIC_LDO5VOLTS_2V9 = 20,
    STPMIC_LDO5VOLTS_3V0 = 21,
    STPMIC_LDO5VOLTS_3V1 = 22,
    STPMIC_LDO5VOLTS_3V2 = 23,
    STPMIC_LDO5VOLTS_3V3 = 24,
    STPMIC_LDO5VOLTS_3V4 = 25,
    STPMIC_LDO5VOLTS_3V5 = 26,
    STPMIC_LDO5VOLTS_3V6 = 27,
    STPMIC_LDO5VOLTS_3V7 = 28,
    STPMIC_LDO5VOLTS_3V8 = 29,
    STPMIC_LDO5VOLTS_3V9 = 30,
} stpmic_ldo5volts_t;

/* LDO voltages. */
typedef enum {
    STPMIC_LDO6VOLTS_0V9 = 0,
    STPMIC_LDO6VOLTS_1V0 = 1,
    STPMIC_LDO6VOLTS_1V1 = 2,
    STPMIC_LDO6VOLTS_1V2 = 3,
    STPMIC_LDO6VOLTS_1V3 = 4,
    STPMIC_LDO6VOLTS_1V4 = 5,
    STPMIC_LDO6VOLTS_1V5 = 6,
    STPMIC_LDO6VOLTS_1V6 = 7,
    STPMIC_LDO6VOLTS_1V7 = 8,
    STPMIC_LDO6VOLTS_1V8 = 9,
    STPMIC_LDO6VOLTS_1V9 = 10,
    STPMIC_LDO6VOLTS_2V0 = 11,
    STPMIC_LDO6VOLTS_2V1 = 12,
    STPMIC_LDO6VOLTS_2V2 = 13,
    STPMIC_LDO6VOLTS_2V3 = 14,
    STPMIC_LDO6VOLTS_2V4 = 15,
    STPMIC_LDO6VOLTS_2V5 = 16,
    STPMIC_LDO6VOLTS_2V6 = 17,
    STPMIC_LDO6VOLTS_2V7 = 18,
    STPMIC_LDO6VOLTS_2V8 = 19,
    STPMIC_LDO6VOLTS_2V9 = 20,
    STPMIC_LDO6VOLTS_3V0 = 21,
    STPMIC_LDO6VOLTS_3V1 = 22,
    STPMIC_LDO6VOLTS_3V2 = 23,
    STPMIC_LDO6VOLTS_3V3 = 24,
} stpmic_ldo6volts_t;

/**
 * setup the specified LDO.
 * @param nth 1 ~ 6.
 * @param opts options to set.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t stpmic_ldo_setup(uint8_t nth, stpmic_ldo_t* opts);

/**
 * enable the specified LDO.
 * @param nth 1 ~ 6.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t stpmic_ldo_enable(uint8_t nth);

/**
 * disable the specified LDO.
 * @param nth 1 ~ 6.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t stpmic_ldo_disable(uint8_t nth);

#ifdef __cplusplus
}
#endif
#endif
