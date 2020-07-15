#pragma once
#include <stdint.h>

#define I2C_STATUS_SUCCESS (0)
#define I2C_TIMEOUT 100

int i2c_init(void);
void i2c_uninit(void);
uint8_t i2c_transmit(uint8_t address, const uint8_t* data, uint16_t length, uint16_t timeout);
uint8_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length, uint16_t timeout);
uint8_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout);
uint8_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout);


int i2cs_init(void);
void i2cs_uninit(void);
void i2cs_prepare(uint8_t* const dat, uint8_t len);
