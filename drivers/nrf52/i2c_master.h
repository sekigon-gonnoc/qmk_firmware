#include "nrf/i2c.h"
#define i2c_stop() i2c_uninit()

// TODO: 本当にこれでいいのか？BMPの戻り値確認
#define I2C_STATUS_SUCCESS (0)
#define I2C_STATUS_ERROR (1)
#define I2C_STATUS_TIMEOUT (2)

#define I2C_TIMEOUT_IMMEDIATE (0)
#define I2C_TIMEOUT_INFINITE (0xFFFF)

#ifndef I2C_TIMEOUT
#  define I2C_TIMEOUT 100
#endif
