/*
Copyright 2018 Sekigon

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
/*
 * scan matrix
 */
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "wait.h"
#include "quantum.h"

#include "progmem.h"

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "ble_radio_notification.h"

#include "app_ble_func.h"

#include "i2c.h"
#include "io_expander.h"

#ifndef MATRIX_ROW_PINS
 #include "pin_assign.h"
#endif

#include <stdbool.h>
const uint32_t rowPins[THIS_DEVICE_ROWS] = MATRIX_ROW_PINS;
const uint32_t colPins[THIS_DEVICE_COLS] = MATRIX_COL_PINS;

#ifndef DEBOUNCE
#   define DEBOUNCE 2
#endif
static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix_dummy[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static void init_rows(void);
static void init_cols(void);
void scan_row2col(void);
matrix_row_t get_row2col(uint8_t row);
void unselectCols(void);
void selectCol(uint8_t col);
matrix_col_t readRows(void);
matrix_col_t read_col(uint8_t col);
void unselectRows(void);
void selectRow(uint8_t row);
matrix_row_t readCols(void);
matrix_row_t read_row(uint8_t row);

__attribute__ ((weak))
void matrix_init_user_for_keymap(void) {
}

void matrix_init_user(){
  // initialize row and col
  init_rows();
  init_cols();
  unselectCols();
  unselectRows();

  set_usb_enabled(true);

  matrix_init_user_for_keymap();
}

uint8_t matrix_scan_impl(matrix_row_t* _matrix){
  uint8_t matrix_offset = 0;

  matrix_offset = 0;
  init_rows();
  for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
    matrix_row_t row = read_row(i);
    if (matrix_debouncing[i + matrix_offset] != row) {
      matrix_debouncing[i + matrix_offset] = row;
      debouncing = DEBOUNCE;
    }
  }


  if (debouncing) {
    if (--debouncing) {
//            wait_ms(1);
    } else {
      for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
        matrix_dummy[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
        _matrix[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
      }
    }
  }

  matrix_offset = MATRIX_ROWS / 2;
  init_cols();
  scan_row2col();
  for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
    matrix_row_t row = get_row2col(i);
    if (matrix_debouncing[i + matrix_offset] != row) {
      matrix_debouncing[i + matrix_offset] = row;
      debouncing = DEBOUNCE;
    }
  }

  if (debouncing) {
    if (--debouncing) {
//            wait_ms(1);
    } else {
      for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
        matrix_dummy[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
        _matrix[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
      }
    }
  }
  return 1;
}

char str[16];

static void init_rows() {
  for(int i=0; i<THIS_DEVICE_COLS; i++) {
    nrf_gpio_cfg_input(colPins[i], NRF_GPIO_PIN_PULLUP);
  }
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_cfg(rowPins[i],
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0D1,
        NRF_GPIO_PIN_NOSENSE);
  }
}
/* Column pin configuration
 */
static void  init_cols(void)
{
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_cfg_input(rowPins[i], NRF_GPIO_PIN_PULLUP);
  }
  for(int i=0; i<THIS_DEVICE_COLS; i++) {
    nrf_gpio_cfg(colPins[i],
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0D1,
        NRF_GPIO_PIN_NOSENSE);
  }
}
matrix_row_t matrix_row2col[MATRIX_ROWS];
void scan_row2col(void)
{
  for (uint8_t i = 0; i < THIS_DEVICE_COLS; i++) {
    matrix_col_t col = read_col(i);
    for (uint8_t j = 0; j < THIS_DEVICE_ROWS; j++) {
      uint8_t bit =  (col >> j) & 1;
      if ( bit == 1) {
        matrix_row2col[j] |= (1 << i);
      } else {
        matrix_row2col[j] &= ~(1 << i);
      }
    }
  }
}
matrix_row_t get_row2col(uint8_t row)
{
  return matrix_row2col[row];
}

/* Returns status of switches(1:on, 0:off) */
matrix_col_t readRows(void)
{
  matrix_col_t col = 0;
  for (int i=0; i<THIS_DEVICE_ROWS; i++) {
    col |= ((nrf_gpio_pin_read(rowPins[i]) ? 0 : 1) << i);
  }
  return col;
}

void unselectCols(void)
{
  for(int i=0; i<THIS_DEVICE_COLS; i++) {
    nrf_gpio_pin_set(colPins[i]);
  }
}

void selectCol(uint8_t col)
{
    nrf_gpio_pin_clear(colPins[col]);
}

/* Returns status of switches(1:on, 0:off) */
matrix_row_t readCols(void)
{
  matrix_row_t row = 0;
  for (int i=0; i<THIS_DEVICE_COLS; i++) {
    row |= ((nrf_gpio_pin_read(colPins[i]) ? 0 : 1) << i);
  }
  return row;
}

/* Row pin configuration
 */
void unselectRows(void)
{
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_pin_set(rowPins[i]);
  }
}

void selectRow(uint8_t row)
{
    nrf_gpio_pin_clear(rowPins[row]);
}

matrix_row_t read_row(uint8_t row)
{
#ifdef USE_I2C_IOEXPANDER
  return read_row_ioexpander(row);
#else
  selectRow(row);
  wait_us(0);
  matrix_row_t row_state = readCols();
  unselectRows();
  return row_state;
#endif
}

matrix_col_t read_col(uint8_t col)
{
  selectCol(col);
  wait_us(0);
  matrix_col_t col_state = readRows();
  unselectCols();
  return col_state;
}
