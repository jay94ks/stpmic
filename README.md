
# STPMIC driver.
1. author: jay94ks@gmail.com
2. repository: https://github.com/jay94ks/stpmic
3. Copyright(C) 2025, jay94ks.
4. License: MIT.

## Usage.
add a preprocessor at your project properties, then copy below code.
```c
#include "stpmic.h"

int some_function() {
    stpmic_timeout_t tmo = {
        .write = HAL_MAX_DELAY,
        .read  = HAL_MAX_DELAY,
    };

    // --> set timeout of STPMIC.
    stpmic_set_timeout(&tmo);

    // --> initialize the STPMIC.
    if (stpmic_init(&i2c4, -1) != STPMIC_RET_OK) {
        // --> hang or some exception handling codes.
        while(1);
    }

    // --> disable PWRCTRL pin's functionality.
    stpmic_pwrctrl_t pwrctrl = {
        .pol = STPMIC_POL_ACTIVE_LOW,
        .pull = STPMIC_PULL_OFF,
        .en = 0,
    }

    stpmic_pwrctrl_init(&pwrctrl);

    stpmic_buck_t buck1 = {
        .pd = STPMIC_BUCKSPD_AUTO_LIGHT,
        .mode = STPMIC_PREGMODE_HIGH,   // --> high power mode (HP).
        .volts = STPMIC_BUCK1VOLTS_1V2, // --> 1.2v output.
        .enable = 0                     // --> disable on setup.
    };

    stpmic_buck_t buck2 = {
        .pd = STPMIC_BUCKSPD_AUTO_LIGHT,
        .mode = STPMIC_PREGMODE_HIGH,   // --> high power mode (HP).
        .volts = STPMIC_BUCK2VOLTS_1V5, // --> 1.5v output.
        .enable = 0                     // --> disable on setup.
    };

    stpmic_buck_t buck3 = {
        .pd = STPMIC_BUCKSPD_AUTO_LIGHT,
        .mode = STPMIC_PREGMODE_HIGH,   // --> high power mode (HP).
        .volts = STPMIC_BUCK3VOLTS_3V3, // --> 3.3v output.
        .enable = 0                     // --> disable on setup.
    };

    stpmic_buck_t buck4 = {
        .pd = STPMIC_BUCKSPD_AUTO_LIGHT,
        .mode = STPMIC_PREGMODE_HIGH,   // --> high power mode (HP).
        .volts = STPMIC_BUCK4VOLTS_3V3, // --> 3.3v output.
        .enable = 1                     // --> enable on setup.
    };

    // --> setup all bucks.
    stpmic_buck_setup(1, &buck1);
    stpmic_buck_setup(2, &buck2);
    stpmic_buck_setup(3, &buck3);
    stpmic_buck_setup(4, &buck4);

    // --> enable BUCK #3.
    stpmic_buck_enable(3);

    // --> LDO.
    stpmic_ldo_t ldo[6];

    for (uint8_t i = 0; i < 6; ++i) {
        ldo[i] = {
            .pd = STPMIC_LDOSPD_ACTIVE
            .enable = 0,
            .bypass = 0,
        };

        if (i < 3) {        // --> 0, 1, 2: #1, #2, #3.
            ldo[i].volts = STPMIC_LDO123VOLTS_3V3;

            if (i == 2) {   // --> #3.
                ldo[i].bypass = 0;
            }
        }

        else if (i == 3) {  // --> #4.
            ldo[i].src = STPMIC_LDO4SRC_UNKNOWN;
        }

        else if (i == 4) {  // --> #5.
            ldo[i].volts = STPMIC_LDO5VOLTS_1V7;
        }

        else if (i == 5) {  // --> #6.
            ldo[i].volts = STPMIC_LDO6VOLTS_2V7;
        }

        stpmic_ldo_setup(i + 1, &ldo[i]);
    }
    
    // --> enable LDO #1.
    stpmic_ldo_enable(1);

}
```

### Options.
1. STPMIC_USE_HAL: uses STM32's HAL library for I2C.
2. STPMIC_USE_CHAN:
    uses `stpmic_i2c_t` structure to provide I2C functions.
    a pointer that is below structure must be passed to `stpmic_init` function.
    
```c
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
```

3. STPMIC_USE_CUSTOM:
    customize entire I2C functions. 
    below functions must be implemented in somewhere.

```c
uint8_t stpmic_read_i2c(uint8_t addr, uint8_t* buf, uint32_t len, uint32_t timeout);
uint8_t stpmic_write_i2c(uint8_t addr, uint8_t* buf, uint32_t len, uint32_t timeout);
```

### common notes for STPMIC_USE_CHAN and STPMIC_USE_CUSTOM.
read and write functions for these, functions must return the succeed length of bytes.
and if expected length and returned length mismatch, it'll be handled as `failure` with error code: `STPMIC_RET_TIMEOUT`.

### `stpmic_hal.h`
If you want to use `STPMIC_USE_HAL` option, 
check supports in `stpmic_hal.h` file.
and you can add HAL includes in `stpmic_hal.h` if missing.