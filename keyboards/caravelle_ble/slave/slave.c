
#include "matrix.h"

#include "wait.h"
#include "app_ble_func.h"
#include "bootloader.h"

#include "nrf_power.h"
#include "nrf_gpio.h"
#include "nrf.h"

extern const uint32_t row_pins[THIS_DEVICE_ROWS];

void unselect_rows(void);
void select_row(uint8_t row);
matrix_row_t read_cols(void);

static bool bootloader_flag = false;

void matrix_init_user() {

  // Set high-current drive mode
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_cfg(row_pins[i],
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_H0D1,
        NRF_GPIO_PIN_NOSENSE);
  }

  select_row(0);
  wait_us(50);
  matrix_row_t row = read_cols();
  unselect_rows();
  if (row == 0b11100) {
    delete_bonds();
  } else if (row == 0b10) {
    bootloader_flag = true;
  }
}

void matrix_scan_user() {
 static int cnt;
 if (bootloader_flag && cnt++==500) {
   bootloader_jump();
 }
}
