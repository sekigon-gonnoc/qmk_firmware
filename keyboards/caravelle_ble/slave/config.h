#pragma once

#include "../board/custom_board.h"

#define THIS_DEVICE_ROWS 1
#define THIS_DEVICE_COLS 24

#define MATRIX_ROW_PINS { 27 }
#define MATRIX_COL_PINS { 31, 19, 13, 3,  6, 11, \
		                      30, 17,  0, 4, 7, 12, 16, \
		                      29, 15,  2, 5, 8, 14, 18, \
		                      20, 22, 23, 24 }
#define IS_LEFT_HAND  true
#define USE_AS_I2C_SLAVE
