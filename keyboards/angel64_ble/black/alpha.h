#pragma once

#ifndef KEYBOARDS_ANGEL64_BLE_ALPHA_H_
#define KEYBOARDS_ANGEL64_BLE_ALPHA_H_

#ifdef RGBLIGHT_ENABLE
//rgb led driver
//#include "ws2812.h"
#endif
#ifdef USE_I2C
#include <stddef.h>
#ifdef __AVR__
	#include <avr/io.h>
	#include <avr/interrupt.h>
#endif
#endif

#endif /* KEYBOARDS_ANGEL64_BLE_ALPHA_H_ */
