#include "stpmic.h"

/* default address for STPMIC1. */
#define STPMIC1_DEF_ADDR    0x33u
#define STPMIC1_MAJOR_VER   2

enum {
    STPMIC_DRV_NOT_INIT = 0,
    STPMIC_DRV_INIT,
    STPMIC_DRV_READY,
};

/* register map, pairs of { start id, max id }. */
const uint8_t STPMIC_REGMAP[] = {
    STPMIC_REG_TURN_ON_SR,      STPMIC_REG_VERSION_SR + 1,
    STPMIC_REG_MAIN_CR,         STPMIC_REG_PKEY_TURNOFF_CR + 1,
    STPMIC_REG_BUCKS_MRST_CR,   STPMIC_REG_BUCKS_MRST_CR + 1,
    STPMIC_REG_LDOS_MRST_CR,    STPMIC_REG_WDG_TMR_CR + 1,
    STPMIC_REG_BUCKx_MAIN_CR,   STPMIC_REG_BUCKx_MAIN_CR + 4,
    STPMIC_REG_LDOx_MAIN_CR,    STPMIC_REG_LDOx_MAIN_CR + 6,
    0, 0
};

/* STPMIC driver status. */
static struct {
#if !STPMIC_USE_CUSTOM
    stpmic_i2c_t*       dev;
#endif
    int16_t             addr;
    uint8_t             state;

    /* read/write timeout. */
    uint32_t            timeout_r;
    uint32_t            timeout_w;
    
    /* cache: (MSB) xxxx xxxS VVVV VVVV (LSB). */
    uint16_t            cache[STPMIC_REG_CACHE_MAX];
} STPMIC1 = {
#if !STPMIC_USE_CUSTOM
    .dev = NULL,
#endif
    .addr = -1,
    .state = 0,
    .timeout_r = STPMIC_INIT_DELAY,
    .timeout_w = STPMIC_INIT_DELAY,
    .cache = { 0, }
};

/* cache mismatch bit, if set, the cached value should be ignored. */
#define STPMIC_CACHE_MISMATCH   (1u << 8)

/* set timeout of STPMIC driver. */
void stpmic_set_timeout(stpmic_timeout_t* in) {
    if (!in) {
        return;
    }

    STPMIC1.timeout_w = in->write;
    STPMIC1.timeout_r = in->read;
}

void stpmic_get_timeout(stpmic_timeout_t* out) {
    if (out) {
        out->write = STPMIC1.timeout_w;
        out->read = STPMIC1.timeout_r;
    }
}

#if STPMIC_USE_CUSTOM
/* initialize the STPMIC driver. */
stpmic_ret_t stpmic_init(int16_t addr) {
    if (addr > 0x7fu) {
        return STPMIC_RET_INVALID;
    }
#else
/* initialize the STPMIC driver. */
stpmic_ret_t stpmic_init(stpmic_i2c_t* dev, int16_t addr) {
    if (!dev || addr > 0x7f) {
        return STPMIC_RET_INVALID;
    }
#endif
    if (STPMIC1.state != STPMIC_DRV_NOT_INIT) {
        return STPMIC_RET_ALREADY;
    }

#if !STPMIC_USE_HAL && !STPMIC_USE_CUSTOM
    if (!dev->read_i2c || !dev->write_i2c) {
        return STPMIC_RET_NOTSUP;
    }
#endif

    if (addr < 0) {
        addr = STPMIC1_DEF_ADDR;
    }

    uint8_t version_sr;
    stpmic_ret_t ret;

#if !STPMIC_USE_CUSTOM
    STPMIC1.dev = dev;
#endif
    STPMIC1.addr = addr;
    STPMIC1.state = STPMIC_DRV_INIT;
    
    // --> read VERSION_SR register.
    ret = stpmic_read_direct(STPMIC_REG_VERSION_SR, &version_sr);

    // --> failed to read.
    if (ret != STPMIC_RET_OK) {
        STPMIC1.state = STPMIC_DRV_NOT_INIT;
        return ret;
    }

    // --> not supported MAJOR version.
    if ((version_sr & 0xf0) != (STPMIC1_MAJOR_VER << 4)) {
        STPMIC1.state = STPMIC_DRV_NOT_INIT;
        return STPMIC_RET_NOTSUP;
    }

    // --> make register caches.
    if (stpmic_reload_cache() != STPMIC_RET_OK) {
        STPMIC1.state = STPMIC_DRV_NOT_INIT;
        return STPMIC_RET_UNKNOWN;
    }
    
    STPMIC1.state = STPMIC_DRV_READY;
    return STPMIC_RET_OK;
}

/* read a register of STPMIC without cache. */
stpmic_ret_t stpmic_read_direct(stpmic_regid_t _reg, stpmic_reg_t* out) {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    if (_reg >= STPMIC_REG_MAX) {
        return STPMIC_RET_INVALID;
    }

    uint8_t val = 0;
    uint8_t reg = _reg;

#if STPMIC_USE_HAL
    // --> transmit a `read` packet.
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        STPMIC1.dev, (STPMIC1.addr << 1) | 0, 
        &reg, sizeof(reg), STPMIC1.timeout_r);

    if (ret != HAL_OK) {
        return STPMIC_RET_TIMEOUT;
    }

    // --> then, receive register values.
    ret = HAL_I2C_Master_Receive(
        STPMIC1.dev, (STPMIC1.addr << 1) | 1, 
        &val, sizeof(val), STPMIC1.timeout_r);

    if (ret != HAL_OK) {
        return STPMIC_RET_TIMEOUT;
    }
#elif STPMIC_USE_CUSTOM
    uint8_t ret = stpmic_write_i2c((STPMIC1.addr << 1) | 0, 
        &reg, sizeof(reg), STPMIC1.timeout_r);

    if (ret != sizeof(reg)) {
        return STPMIC_RET_TIMEOUT;
    }

    // --> then, receive register values.
    ret = stpmic_read_i2c((STPMIC1.addr << 1) | 1, 
        &val, sizeof(val), STPMIC1.timeout_r);

    if (ret != sizeof(val)) {
        return STPMIC_RET_TIMEOUT;
    }
#else
    uint8_t ret = STPMIC1.dev->write_i2c(
        STPMIC1.dev, (STPMIC1.addr << 1) | 0, 
        &reg, sizeof(reg), STPMIC1.timeout_r);

    if (ret != sizeof(reg)) {
        return STPMIC_RET_TIMEOUT;
    }

    // --> then, receive register values.
    ret = STPMIC1.dev->read_i2c(
        STPMIC1.dev, (STPMIC1.addr << 1) | 1, 
        &val, sizeof(val), STPMIC1.timeout_r);

    if (ret != sizeof(val)) {
        return STPMIC_RET_TIMEOUT;
    }
#endif

    if (out) {
        *out = val;
    }

    if (reg < STPMIC_REG_CACHE_MAX) {
        STPMIC1.cache[reg] = val;
    }

    return STPMIC_RET_OK;
}

/* write a register of STPMIC without cache. */
stpmic_ret_t stpmic_write_direct(stpmic_regid_t reg, stpmic_reg_t val) {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    if (reg >= STPMIC_REG_MAX) {
        return STPMIC_RET_INVALID;
    }

    uint8_t buf[2] = { reg, val };
    
#if STPMIC_USE_HAL
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        STPMIC1.dev, (STPMIC1.addr << 1) | 0, 
        buf, sizeof(buf), STPMIC1.timeout_w);

    if (ret != HAL_OK) {
        return STPMIC_RET_TIMEOUT;
    }
#elif STPMIC_USE_CUSTOM
    uint8_t ret = stpmic_write_i2c((STPMIC1.addr << 1) | 0, 
        buf, sizeof(buf), STPMIC1.timeout_w);

    if (ret != sizeof(buf)) {
        return STPMIC_RET_TIMEOUT;
    }
#else
    uint8_t ret = STPMIC1.dev->write_i2c(
        STPMIC1.dev, (STPMIC1.addr << 1) | 0, 
        buf, sizeof(buf), STPMIC1.timeout_w);

    if (ret != sizeof(buf)) {
        return STPMIC_RET_TIMEOUT;
    }
#endif

    if (reg < STPMIC_REG_CACHE_MAX) {
        STPMIC1.cache[reg] = val;
    }
    
    return STPMIC_RET_OK;
}

/* read a register of STPMIC with cache. */
stpmic_ret_t stpmic_read(stpmic_regid_t reg, stpmic_reg_t* out) {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    if (reg >= STPMIC_REG_MAX) {
        return STPMIC_RET_INVALID;
    }

    if (reg >= STPMIC_REG_CACHE_MAX || (
        STPMIC1.cache[reg] & STPMIC_CACHE_MISMATCH)) 
    {
        return stpmic_read_direct(reg, out);
    }

    if (out) {
    	*out = (uint8_t)(STPMIC1.cache[reg] & 0xffu);
    }

    return STPMIC_RET_OK;
}

/* write a register of STPMIC with cache. */
stpmic_ret_t stpmic_write(stpmic_regid_t reg, stpmic_reg_t val) {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    if (reg >= STPMIC_REG_MAX) {
        return STPMIC_RET_INVALID;
    }

    if (reg >= STPMIC_REG_CACHE_MAX || (
        STPMIC1.cache[reg] & STPMIC_CACHE_MISMATCH)) 
    {
        return stpmic_write_direct(reg, val);
    }

    if ((uint8_t)(STPMIC1.cache[reg] & 0xffu) != val) {
        return stpmic_write_direct(reg, val);
    }

    return STPMIC_RET_OK;
}

/* clear a register cache. */
stpmic_ret_t stpmic_clear_cache(stpmic_regid_t reg) {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    if (reg >= STPMIC_REG_MAX) {
        return STPMIC_RET_INVALID;
    }

    STPMIC1.cache[reg] |= STPMIC_CACHE_MISMATCH;
    return STPMIC_RET_OK;
}

/* reserve to write a register of STPMIC. */
stpmic_ret_t stpmic_batch_write(stpmic_regid_t reg, stpmic_reg_t val) {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    if (reg >= STPMIC_REG_MAX) {
        return STPMIC_RET_INVALID;
    }

    STPMIC1.cache[reg] = ((uint16_t) val) | STPMIC_CACHE_MISMATCH;
    return STPMIC_RET_OK;
}

/* flush all pending batch writes. */
stpmic_ret_t stpmic_batch_flush() {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    stpmic_ret_t ret = STPMIC_RET_OK;
    uint32_t total = 0;
    uint32_t success = 0;
    
    for (size_t i = 0;; i += 2) {
        uint8_t s = STPMIC_REGMAP[i + 0];
        uint8_t e = STPMIC_REGMAP[i + 1];

        if (s == e && e == 0) {
            break;
        }

        while (s < e) {
            const stpmic_regid_t reg = (stpmic_regid_t) s++;
            total++;

            // --> write registers to make cache.
            ret = stpmic_write_direct(reg, STPMIC1.cache[reg]);

            if (ret != STPMIC_RET_OK) {
                break;
            }

            success++;
        }
    }

    if (success >= total) {
        return STPMIC_RET_OK;
    }

    // --> return the last error.
    return ret;
}

/* reload all cached registers. */
stpmic_ret_t stpmic_reload_cache() {
    if (STPMIC1.state < STPMIC_DRV_INIT) {
        return STPMIC_RET_NODEV;
    }
    
    stpmic_ret_t ret = STPMIC_RET_OK;
    uint32_t total = 0;
    uint32_t success = 0;
    
    for (size_t i = 0;; i += 2) {
        uint8_t s = STPMIC_REGMAP[i + 0];
        uint8_t e = STPMIC_REGMAP[i + 1];

        if (s == e && e == 0) {
            break;
        }

        while (s < e) {
            const stpmic_regid_t reg = (stpmic_regid_t) s++;
            total++;

            // --> read registers to make cache.
            ret = stpmic_read_direct(reg, NULL);

            if (ret != STPMIC_RET_OK) {
                break;
            }

            success++;
        }
    }

    if (success >= total) {
        return STPMIC_RET_OK;
    }

    // --> return the last error.
    return ret;
}

/* get the version of STPMIC. */
stpmic_ret_t stpmic_version(stpmic_version_t* out) {
    stpmic_reg_t version_sr;
    stpmic_ret_t ret = stpmic_read(STPMIC_REG_VERSION_SR, &version_sr);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if (out) {
        out->major = (version_sr >> 4) & 0x0fu;
        out->minor = (version_sr >> 0) & 0x0fu;
    }

    return STPMIC_RET_OK;
}

/* get the STPMIC's operating mode. */
uint8_t stpmic_opmode_is_main() {
    stpmic_reg_t reg;

    if (stpmic_restartsr(&reg) != STPMIC_RET_OK) {
        return 0xffu;
    }

    return reg & STPMIC_RESTARTSR_OP_MODE_MASK;
}

/* get the LDO4'th input source. */
uint8_t stpmic_ldo4_src(stpmic_reg_t in) {
    stpmic_reg_t reg;

    if (stpmic_restartsr(&reg) != STPMIC_RET_OK) {
        return 0xffu;
    }

    return reg & STPMIC_RESTARTSR_LDO4_SRC_MASK;
}

/* request `software switch` off. */
stpmic_ret_t stpmic_request_swoff() {
    stpmic_reg_t reg;
    stpmic_ret_t ret = stpmic_maincr(&reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    reg |= STPMIC_MAINCR_SWOFF;
    return stpmic_write_direct(STPMIC_REG_MAIN_CR, reg);
}

/* initialize the PWRCTRL pin's functionality. */
stpmic_ret_t stpmic_pwrctrl_init(stpmic_pwrctrl_t* pwrctrl) {
    stpmic_reg_t mcr, ppcr;
    stpmic_ret_t ret = stpmic_maincr(&mcr);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if ((ret = stpmic_padspullcr(&ppcr)) != STPMIC_RET_OK) {
        return ret;
    }

    mcr &= ~STPMIC_MAINCR_PWRCTL_EN;
    mcr &= ~STPMIC_MAINCR_PWRCTL_POL;
    ppcr &= ~(
        STPMIC_PADSPULLCR_PWRCTRL_PD |
        STPMIC_PADSPULLCR_PWRCTRL_PU
    );

    if (pwrctrl->pol) {
        mcr |= STPMIC_MAINCR_PWRCTL_POL;
    }

    if (pwrctrl->en) {
        mcr |= STPMIC_MAINCR_PWRCTL_EN;
    }

    ppcr |= (pwrctrl->pull << 2) & (
        STPMIC_PADSPULLCR_PWRCTRL_PD |
        STPMIC_PADSPULLCR_PWRCTRL_PU
    );
    
    if ((ret = stpmic_write(STPMIC_REG_PADS_PULL_CR, ppcr)) != STPMIC_RET_OK) {
        return ret;
    }

    return stpmic_write(STPMIC_REG_MAIN_CR, mcr);
}

/* enable the PWRCTRL pin's functionality. */
stpmic_ret_t stpmic_pwrctrl_enable() {
    stpmic_reg_t mcr;
    stpmic_ret_t ret = stpmic_maincr(&mcr);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    mcr |= STPMIC_MAINCR_PWRCTL_EN;
    return stpmic_write(STPMIC_REG_MAIN_CR, mcr);
}

/* enable the PWRCTRL pin's functionality. */
stpmic_ret_t stpmic_pwrctrl_disable() {
    stpmic_reg_t mcr;
    stpmic_ret_t ret = stpmic_maincr(&mcr);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    mcr &= ~STPMIC_MAINCR_PWRCTL_EN;
    return stpmic_write(STPMIC_REG_MAIN_CR, mcr);
}

/* de-initialize PWRCTRL functionality. */
stpmic_ret_t stpmic_pwrctrl_deinit() {
    stpmic_reg_t mcr, ppcr;
    stpmic_ret_t ret = stpmic_maincr(&mcr);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if ((ret = stpmic_padspullcr(&ppcr)) != STPMIC_RET_OK) {
        return ret;
    }

    mcr &= ~STPMIC_MAINCR_PWRCTL_EN;
    ppcr &= ~(
        STPMIC_PADSPULLCR_PWRCTRL_PD |
        STPMIC_PADSPULLCR_PWRCTRL_PU
    );
    
    if ((ret = stpmic_write(STPMIC_REG_PADS_PULL_CR, ppcr)) != STPMIC_RET_OK) {
        return ret;
    }

    return stpmic_write(STPMIC_REG_MAIN_CR, mcr);
}

/* initialize the WAKE-UP pin's functionality. */
stpmic_ret_t stpmic_wakeup_init(stpmic_wakeup_t* wakeup) {
    stpmic_reg_t reg;
    stpmic_ret_t ret = stpmic_padspullcr(&reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    reg &= ~(STPMIC_PADSPULLCR_WKUP_PD | STPMIC_PADSPULLCR_WKUP_PU);
    reg &= ~STPMIC_PADSPULLCR_WKUP_EN;

    reg |= (wakeup->pull << 0) & (
        STPMIC_PADSPULLCR_WKUP_PD | 
        STPMIC_PADSPULLCR_WKUP_PU
    );

    if (wakeup->en) {
        reg |= STPMIC_PADSPULLCR_WKUP_EN;
    }
    
    return stpmic_write(STPMIC_REG_PADS_PULL_CR, reg);
}

/* de-initialize the WAKE-UP pin's functionality. */
stpmic_ret_t stpmic_wakeup_deinit() {
    stpmic_reg_t reg;
    stpmic_ret_t ret = stpmic_padspullcr(&reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    reg &= ~(STPMIC_PADSPULLCR_WKUP_PD | STPMIC_PADSPULLCR_WKUP_PU);
    reg &= ~STPMIC_PADSPULLCR_WKUP_EN;
    
    return stpmic_write(STPMIC_REG_PADS_PULL_CR, reg);
}

/* get the MRST masks from BUCKS_MRST_CR and LDOS_MRST_CR. */
stpmic_ret_t stpmic_mrst(uint16_t* out) {
    uint8_t temp[2];
    stpmic_ret_t ret;

    if ((ret = stpmic_read(STPMIC_REG_BUCKS_MRST_CR, &temp[0])) != STPMIC_RET_OK) {
        return ret;
    }

    if ((ret = stpmic_read(STPMIC_REG_LDOS_MRST_CR, &temp[1])) != STPMIC_RET_OK) {
        return ret;
    }

    if (out) {
        *out = temp[1] | (((uint16_t)temp[0]) << 8);
    }

    return STPMIC_RET_OK;
}

/* set the MRST masks from BUCKS_MRST_CR and LDOS_MRST_CR. */
stpmic_ret_t stpmic_set_mrst(uint16_t val) {
    uint8_t temp[2];
    stpmic_ret_t ret;

    if ((ret = stpmic_read(STPMIC_REG_BUCKS_MRST_CR, &temp[0])) != STPMIC_RET_OK) {
        return ret;
    }

    if ((ret = stpmic_read(STPMIC_REG_LDOS_MRST_CR, &temp[1])) != STPMIC_RET_OK) {
        return ret;
    }

    temp[0] = (temp[0] & ~0x0f) | ((val >> 8) & 0x0f);
    temp[1] = (temp[1] & ~0x7f) | ((val >> 0) & 0x7f);
    
    if ((ret = stpmic_write(STPMIC_REG_BUCKS_MRST_CR, temp[0])) != STPMIC_RET_OK) {
        return ret;
    }
    
    return stpmic_write(STPMIC_REG_LDOS_MRST_CR, temp[1]);
}

/* initialize the watchdog timer, sec: 1 ~ 255, 0: disable. */
stpmic_ret_t stpmic_watchdog_init(uint8_t sec) {
    if (sec == 0) {
        return stpmic_watchdog_deinit();
    }

    /* 0x00 ~ 0xff = 1sec ~ 256sec. */
    stpmic_ret_t ret = stpmic_write(STPMIC_REG_WDG_TMR_CR, sec - 1);
    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    uint8_t cr = (1u << 1) | (sec > 0 ? 0x01 : 0);
    return stpmic_write_direct(STPMIC_REG_WDG_CR, cr);
}


/* reset the watchdog counter to default counter. */
stpmic_ret_t stpmic_watchdog_reset() {
    stpmic_reg_t reg = 0;
    stpmic_ret_t ret = stpmic_read(STPMIC_REG_WDG_TMR_CR, &reg);
    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    /* (MSB) RESV | RST(1) | ENA(1) (LSB). */
    if ((reg & (1u << 0)) == 0) {
        return STPMIC_RET_DISABLED;
    }

    return stpmic_write_direct(STPMIC_REG_WDG_CR, (1u << 1) | (1u << 0));
}

/* setup one of buck #1 ~ #4. */
stpmic_ret_t __stpmic_buck_setup(uint8_t nth, uint8_t alt, stpmic_buck_t* opts) {
    if (nth <= 0 || nth > 4) {
        return STPMIC_RET_RANGE;
    }

    stpmic_reg_t buckspd;
    stpmic_ret_t ret = stpmic_buckspd(&buckspd);
    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    uint8_t val = (opts->volts << 2) & 0xfc;
    if (!opts) {
        return STPMIC_RET_INVALID;
    }

    if (opts->mode) {
        val |= STPMIC_BIT_MASK(1);
    }

    if (opts->enable) {
        val |= STPMIC_BIT_MASK(0);
    }

    buckspd = stpmic_set_buckspd(buckspd, nth, opts->pd);
    if ((ret = stpmic_write(STPMIC_REG_BUCKS_PD_CR, buckspd)) != STPMIC_RET_OK) {
        return ret;
    }

    return stpmic_write(
        (stpmic_regid_t)((
            alt 
            ? STPMIC_REG_BUCKx_ALT_CR 
            : STPMIC_REG_BUCKx_MAIN_CR
        ) + (nth - 1)),
        val);
}

/**
 * enable the specified buck converter.
 * @param nth 1 ~ 4.
 * @return
 * `STPMIC_RET_NODEV` if STPMIC driver is not ready.
 * `STPMIC_RET_TIMEOUT` if timeout reached.
 * `STPMIC_RET_RANGE` if `nth` value is out of range.
 */
stpmic_ret_t __stpmic_buck_enable(uint8_t nth, uint8_t alt) {
    stpmic_reg_t reg;
    stpmic_ret_t ret = alt
        ? stpmic_buck_alt_cr(nth, &reg)
        : stpmic_buck_main_cr(nth, &reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if (reg & STPMIC_BIT_MASK(0)) {
        return STPMIC_RET_ALREADY;
    }

    reg |= STPMIC_BIT_MASK(0);
    return stpmic_write(
        (stpmic_regid_t)((
            alt 
            ? STPMIC_REG_BUCKx_ALT_CR 
            : STPMIC_REG_BUCKx_MAIN_CR
        ) + (nth - 1)),
        reg);
}

/* disable the specified buck converter. */
stpmic_ret_t __stpmic_buck_disable(uint8_t nth, uint8_t alt) {
    stpmic_reg_t reg;
    stpmic_ret_t ret = alt
        ? stpmic_buck_alt_cr(nth, &reg)
        : stpmic_buck_main_cr(nth, &reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if ((reg & STPMIC_BIT_MASK(0)) == 0) {
        return STPMIC_RET_ALREADY;
    }

    reg &= ~STPMIC_BIT_MASK(0);
    return stpmic_write(
        (stpmic_regid_t)((
            alt 
            ? STPMIC_REG_BUCKx_ALT_CR 
            : STPMIC_REG_BUCKx_MAIN_CR
        ) + (nth - 1)),
        reg);
}

/* setup the specified LDO. */
stpmic_ret_t __stpmic_ldo_setup(uint8_t nth, uint8_t alt, stpmic_ldo_t* opts) {
    stpmic_reg_t ldo;
    stpmic_reg_t reg;
    stpmic_ret_t ret;

    if (nth >= 1 && nth <= 4) {
        if ((ret = stpmic_ldo1234pd(&reg)) != STPMIC_RET_OK) {
            return ret;
        }

        reg = stpmic_set_ldo1234pd(reg, nth, opts->pd);

        if ((ret = stpmic_write(STPMIC_REG_LDO1234_PD_CR, reg)) != STPMIC_RET_OK) {
            return ret;
        }
    }

    else if (nth >= 5 && nth <= 6) {
        if ((ret = stpmic_ldo56pd(&reg)) != STPMIC_RET_OK) {
            return ret;
        }

        reg = stpmic_set_ldo56pd(reg, nth, opts->pd);

        if ((ret = stpmic_write(STPMIC_REG_LDO56_VREF_PD_CR, reg)) != STPMIC_RET_OK) {
            return ret;
        }
    }

    else {
        return STPMIC_RET_RANGE;
    }

    if (alt) {
        ldo = (stpmic_reg_t)(STPMIC_REG_LDOx_ALT_CR + (nth - 1));
    } else {
        ldo = (stpmic_reg_t)(STPMIC_REG_LDOx_MAIN_CR + (nth - 1));
    }

    uint8_t val = 0;
    switch (nth) {
        case 1: case 2: case 5: case 6: {
            val = (opts->volts & 0x1f) << 2;

            if (opts->enable) {
                val |= STPMIC_BIT_MASK(0);
            }
            break;
        }

        case 3: {
            val = (opts->volts & 0x1f) << 2;

            if (opts->bypass) {
                val |= STPMIC_BIT_MASK(7);
            }

            if (opts->enable) {
                val |= STPMIC_BIT_MASK(0);
            }
            break;
        }
        case 4: {
            switch (opts->src) {
                case STPMIC_LDO4SRC_VIN:
                    val |= STPMIC_BIT_MASK(2);
                    break;

                case STPMIC_LDO4SRC_BSTOUT:
                    val |= STPMIC_BIT_MASK(3);
                    break;

                case STPMIC_LDO4SRC_VBUSOTG:
                    val |= STPMIC_BIT_MASK(4);
                    break;
                
                case STPMIC_LDO4SRC_UNKNOWN: // --> automatic for all bits.
                default:
                    break;
            }

            if (opts->src != STPMIC_LDO4SRC_OFF &&
                opts->enable) 
            {
                val |= STPMIC_BIT_MASK(0);
            }
            break;
        }

        default:
            return STPMIC_RET_RANGE;
    }

    return stpmic_write(ldo, val);
}

/* enable the specified LDO. */
stpmic_ret_t __stpmic_ldo_enable(uint8_t nth, uint8_t alt) {
    if (nth <= 0 || nth > 6) {
        return STPMIC_RET_RANGE;
    }

    stpmic_reg_t reg;
    stpmic_regid_t ldo = (stpmic_reg_t)((
        alt ? STPMIC_REG_LDOx_ALT_CR : STPMIC_REG_LDOx_MAIN_CR
    ) + (nth - 1));

    stpmic_ret_t ret = stpmic_read(ldo, &reg);
    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if (reg & STPMIC_BIT_MASK(0)) {
        return STPMIC_RET_ALREADY;
    }

    reg |= STPMIC_BIT_MASK(0);
    return stpmic_write(ldo, reg);
}

/* disable the specified LDO. */
stpmic_ret_t __stpmic_ldo_disable(uint8_t nth, uint8_t alt) {
    if (nth <= 0 || nth > 6) {
        return STPMIC_RET_RANGE;
    }

    stpmic_reg_t reg;
    stpmic_regid_t ldo = (stpmic_reg_t)((
        alt ? STPMIC_REG_LDOx_ALT_CR : STPMIC_REG_LDOx_MAIN_CR
    ) + (nth - 1));

    stpmic_ret_t ret = stpmic_read(ldo, &reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if ((reg & STPMIC_BIT_MASK(0)) == 0) {
        return STPMIC_RET_ALREADY;
    }

    reg &= ~STPMIC_BIT_MASK(0);
    return stpmic_write(ldo, reg);
}

/* enable the REFDDR. */
stpmic_ret_t __stpmic_refddr_enable(uint8_t alt) {
    stpmic_reg_t reg;
    stpmic_ret_t ret = stpmic_read((
        alt ? STPMIC_REG_REFDDR_ALT_CR : STPMIC_REG_REFDDR_MAIN_CR
    ), &reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    reg |= STPMIC_BIT_MASK(0);
    return stpmic_write((
        alt ? STPMIC_REG_REFDDR_ALT_CR : STPMIC_REG_REFDDR_MAIN_CR
    ), reg);
}

/* disable the REFDDR. */
stpmic_ret_t __stpmic_refddr_disable(uint8_t alt) {
    stpmic_reg_t reg;
    stpmic_ret_t ret = stpmic_read((
        alt ? STPMIC_REG_REFDDR_ALT_CR : STPMIC_REG_REFDDR_MAIN_CR
    ), &reg);

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    reg &= ~STPMIC_BIT_MASK(0);
    return stpmic_write((
        alt ? STPMIC_REG_REFDDR_ALT_CR : STPMIC_REG_REFDDR_MAIN_CR
    ), reg);
}

/* read `INT_PENDING_Rx` register. */
stpmic_ret_t stpmic_interrupt_pending(uint32_t* out) {
    uint32_t val = 0;

    for (
        uint8_t i = STPMIC_REG_INT_PENDING_R1;
         i < STPMIC_REG_INT_PENDING_R4; ++i) 
    {
        stpmic_reg_t reg;
        stpmic_ret_t ret = stpmic_read((stpmic_regid_t)i, &reg);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }

        val |= ((uint32_t)reg) << (i << 3);
    }

    if (out) {
        *out = val;
    }

    return STPMIC_RET_OK;
}

/* clear interrupts. */
stpmic_ret_t stpmic_interrupt_clear(uint32_t bitmap) {
    for (
        uint8_t i = STPMIC_REG_INT_CLEAR_R1;
         i < STPMIC_REG_INT_CLEAR_R4; ++i) 
    {
        const uint8_t u8 = bitmap >> (i << 3);
        if (u8 == 0) {
            continue;
        }

        stpmic_ret_t ret = stpmic_write((stpmic_regid_t)i, u8);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }
    }

    return STPMIC_RET_OK;
}

/* mask an interrupt. */
stpmic_ret_t stpmic_interrupt_read_mask(uint32_t* out) {
    uint32_t val = 0;

    for (
        uint8_t i = STPMIC_REG_INT_MASK_R1;
         i < STPMIC_REG_INT_MASK_R4; ++i) 
    {
        stpmic_reg_t reg;
        stpmic_ret_t ret = stpmic_read((stpmic_regid_t)i, &reg);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }

        val |= ((uint32_t)reg) << (i << 3);
    }

    if (out) {
        *out = val;
    }

    return STPMIC_RET_OK;
}

/* set interrupt masks. */
stpmic_ret_t stpmic_interrupt_mask_set(uint32_t bitmap) {
    for (
        uint8_t i = STPMIC_REG_INT_MASK_SET_R1;
         i < STPMIC_REG_INT_MASK_SET_R4; ++i) 
    {
        const uint8_t u8 = bitmap >> (i << 3);
        if (u8 == 0) {
            continue;
        }

        stpmic_ret_t ret = stpmic_write((stpmic_regid_t)i, u8);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }
    }

    return STPMIC_RET_OK;
}

/* clear interrupt masks. */
stpmic_ret_t stpmic_interrupt_mask_clear(uint32_t bitmap) {
    for (
        uint8_t i = STPMIC_REG_INT_MASK_CLEAR_R1;
         i < STPMIC_REG_INT_MASK_CLEAR_R4; ++i) 
    {
        const uint8_t u8 = bitmap >> (i << 3);
        if (u8 == 0) {
            continue;
        }

        stpmic_ret_t ret = stpmic_write((stpmic_regid_t)i, u8);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }
    }

    return STPMIC_RET_OK;
}

/* read interrupt sources. */
stpmic_ret_t stpmic_interrupt_read_source(uint32_t* out) {
    uint32_t val = 0;

    for (
        uint8_t i = STPMIC_REG_INT_SRC_R1;
         i < STPMIC_REG_INT_SRC_R4; ++i) 
    {
        stpmic_reg_t reg;
        stpmic_ret_t ret = stpmic_read((stpmic_regid_t)i, &reg);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }

        val |= ((uint32_t)reg) << (i << 3);
    }

    if (out) {
        *out = val;
    }

    return STPMIC_RET_OK;
}

/* write interrupt sources. */
stpmic_ret_t stpmic_interrupt_write_source(uint32_t bitmap) {
    for (
        uint8_t i = STPMIC_REG_INT_SRC_R1;
         i < STPMIC_REG_INT_SRC_R4; ++i) 
    {
        const uint8_t u8 = bitmap >> (i << 3);
        if (u8 == 0) {
            continue;
        }

        stpmic_ret_t ret = stpmic_write((stpmic_regid_t)i, u8);

        if (ret != STPMIC_RET_OK) {
            return ret;
        }
    }

    return STPMIC_RET_OK;
}

/* test whether the NVM controller is busy or not. */
stpmic_ret_t stpmic_nvm_is_busy() {
    stpmic_reg_t reg;
    stpmic_ret_t ret = stpmic_read_direct(
        STPMIC_REG_NVM_SR, &reg
    );

    if (ret != STPMIC_RET_OK) {
        return ret;
    }

    if (reg & STPMIC_BIT_MASK(0)) {
        return STPMIC_RET_BUSY;
    }

    return STPMIC_RET_OK;
}