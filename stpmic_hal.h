#ifndef __STPMIC_HAL_H__
#define __STPMIC_HAL_H__

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
 * adds HAL includes in this file if missing.
 * and HALs in below are tested.
 */
#if defined(STM32H7)        // --> tested on H7B3VIT6, H723VIT6.
#include "stm32h7xx_hal.h"
#elif defined(STM32G0)      // --> tested on G030F6P6, G031F6P6 and G0B1CBT6.
#include "stm32g0xx_hal.h"
#elif defined(STM32F0)      // --> tested on F042F6P6.
#include "stm32f0xx_hal.h"
#elif defined(STM32F1)      // --> tested on F103CBT6.
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)      // --> tested on F411CBU6, F401RCT6.
#include "stm32f4xx_hal.h"
#else
#error "please add a preprocessor at your project properties."
#endif

#endif