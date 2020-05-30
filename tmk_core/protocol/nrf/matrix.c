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
const uint32_t row_pins[THIS_DEVICE_ROWS] = MATRIX_ROW_PINS;
const uint32_t col_pins[THIS_DEVICE_COLS] = MATRIX_COL_PINS;
const bool isLeftHand = IS_LEFT_HAND;

#ifndef DEBOUNCE
#   define DEBOUNCE 2
#endif
static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_dummy[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static bool send_flag;
#define QUEUE_LEN 32
typedef struct {
  ble_switch_state_t* const buf;
  uint8_t ridx, widx, cnt;
  const uint8_t len;
} switch_queue;
ble_switch_state_t rcv_keys_buf[QUEUE_LEN], delay_keys_buf[QUEUE_LEN];
switch_queue rcv_keys_queue={rcv_keys_buf, 0, 0, 0, sizeof(rcv_keys_buf)/sizeof(rcv_keys_buf[0])};
switch_queue delay_keys_queue={delay_keys_buf, 0, 0, 0, sizeof(delay_keys_buf)/sizeof(delay_keys_buf[0])};
#ifndef BURST_TRESHOLD
  extern const uint8_t MAINTASK_INTERVAL;
#ifdef BLE_NUS_MAX_INTERVAL
  #define BURST_THRESHOLD (BLE_NUS_MAX_INTERVAL/MAINTASK_INTERVAL+1)
#else
  #define BURST_THRESHOLD 1
#endif // BLE_NUS_MAX_INTERVAL
#endif

static size_t push_queue(switch_queue *q, ble_switch_state_t dat) {
  if (q->cnt < q->len) {
    q->buf[q->widx++] = dat;
    q->widx %= q->len;
    q->cnt++;
    return 1;
  } else {
    return 0;
  }
}

static size_t pop_queue(switch_queue *q, ble_switch_state_t *dat) {
  if (q->cnt) {
    *dat = q->buf[q->ridx++];
    q->ridx %= q->len;
    q->cnt--;
    return 1;
  } else {
    return 0;
  }
}

static ble_switch_state_t front_queue(switch_queue *q){
  return q->buf[q->ridx];
}

static void init_rows(void);
static void init_cols(void);
#if DIODE_DIRECTION == ROW2COL
void scan_row2col_matrix(void);
matrix_row_t get_row2col_matrix(uint8_t row);
void unselect_cols(void);
void select_col(uint8_t col);
matrix_col_t read_rows(void);
matrix_col_t read_col(uint8_t col);
#else
void unselect_rows(void);
void select_row(uint8_t row);
matrix_row_t read_cols(void);
matrix_row_t read_row(uint8_t row);
#endif

__attribute__ ((weak))
void matrix_init_quantum(void) {
    matrix_init_kb();
}

__attribute__ ((weak))
void matrix_scan_quantum(void) {
    matrix_scan_kb();
}

__attribute__ ((weak))
void matrix_init_kb(void) {
    matrix_init_user();
}

__attribute__ ((weak))
void matrix_scan_kb(void) {
    matrix_scan_user();
}

__attribute__ ((weak))
void matrix_init_user(void) {
}

__attribute__ ((weak))
void matrix_scan_user(void) {
}

inline
uint8_t matrix_rows(void)
{
    return MATRIX_ROWS;
}

inline
uint8_t matrix_cols(void)
{
    return MATRIX_COLS;
}

#define LED_ON()    do { } while (0)
#define LED_OFF()   do { } while (0)
#define LED_TGL()   do { } while (0)

ret_code_t ret_radio = 0xFFFF;
static uint8_t timing;
static uint8_t sync;
void radio_event_callback(bool active){
  if(!active && send_flag){
#ifdef NRF_SEPARATE_KEYBOARD_SLAVE
    sync = timing % 0xFF;
    send_flag = false;
#endif
  }
}

void matrix_init(void) {
  // initialize row and col
  init_rows();
#if DIODE_DIRECTION == ROW2COL
  unselect_cols();
#else
  unselect_rows();
#endif
  init_cols();
//    NRF_LOG_INFO("matrix init\r\n")

// initialize matrix state: all keys off
  for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
    matrix[i] = 0;
    matrix_debouncing[i] = 0;
  }

#if defined(NRF_SEPARATE_KEYBOARD_MASTER) && defined(USE_I2C)
  i2c_init();
#endif
#if defined(NRF_SEPARATE_KEYBOARD_SLAVE) && defined(USE_I2C)
  i2cs_init();
#endif
#if defined(USE_AS_I2C_SLAVE)
  i2cs_init();
#endif
  matrix_init_quantum();
}

static inline void set_received_key(ble_switch_state_t key, bool from_slave) {
  const uint8_t matrix_offset = (isLeftHand ^ from_slave) ?  0 : THIS_DEVICE_ROWS;

  uint8_t row = key.id / MATRIX_COLS;
  uint8_t col = key.id % MATRIX_COLS;

  if (key.state) {
    matrix[row + matrix_offset] |= (1 << col);
  } else {
    matrix[row + matrix_offset] &= ~(1 << col);
  }
}

__attribute__ ((weak))
uint8_t matrix_scan_impl(matrix_row_t* _matrix){
  uint8_t matrix_offset = isLeftHand ? 0 : MATRIX_ROWS-THIS_DEVICE_ROWS;
  volatile int matrix_changed = 0;
  ble_switch_state_t ble_switch_send[THIS_DEVICE_ROWS*THIS_DEVICE_COLS];

#if DIODE_DIRECTION == ROW2COL
  scan_row2col_matrix();
  for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
    matrix_row_t row = get_row2col_matrix(i);
    if (matrix_debouncing[i + matrix_offset] != row) {
      matrix_debouncing[i + matrix_offset] = row;
      debouncing = DEBOUNCE;
    }
  }
#else
  for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
    matrix_row_t row = read_row(i);
    if (matrix_debouncing[i + matrix_offset] != row) {
      matrix_debouncing[i + matrix_offset] = row;
      debouncing = DEBOUNCE;
    }
  }
#endif

  if (debouncing) {
    if (--debouncing) {
//            wait_ms(1);
    } else {
      for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
        if (matrix_dummy[i + matrix_offset] != matrix_debouncing[i + matrix_offset]) {
          for (uint8_t j = 0; j < THIS_DEVICE_COLS; j++) {
            if ((matrix_dummy[i + matrix_offset]
                ^ matrix_debouncing[i + matrix_offset]) & (1 << j)) {
              ble_switch_send[0].dat[0]=0xff;
              ble_switch_send[0].dat[1]=((int)sync) % 0xff; // synchronizing packet
              ble_switch_send[matrix_changed+1].timing = timing;
              ble_switch_send[matrix_changed+1].state = (matrix_debouncing[i
                  + matrix_offset] >> j) & 1;
              ble_switch_send[matrix_changed+1].id = i * MATRIX_COLS + j;
              matrix_changed++;
            }
          }
        }
#if defined(NRF_SEPARATE_KEYBOARD_MASTER) || defined(NRF_SEPARATE_KEYBOARD_SLAVE)
        matrix_dummy[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
//        matrix[i + matrix_offset] = matrix_debouncing[i + matrix_offset]; Do not set matrix directory
#else
        matrix_dummy[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
        matrix[i + matrix_offset] = matrix_debouncing[i + matrix_offset];
#endif
      }
    }
  }

#if defined(NRF_SEPARATE_KEYBOARD_MASTER)
  for (int i=0; i<matrix_changed; i++) {
    push_queue(&delay_keys_queue, ble_switch_send[i+1]);
  }

#if defined(NRF_SEPARATE_KEYBOARD_MASTER) && defined(USE_I2C)
#if MATRIX_COLS>8
#error "MATRIX_COLS should be less than eight for I2C "
#endif
  uint8_t slave_offset = isLeftHand ? THIS_DEVICE_ROWS : 0;
  uint8_t slave_matrix_changed = 0;
  uint8_t i2c_dat[MATRIX_ROWS];
  memset(i2c_dat, 0xFF, sizeof(i2c_dat));
//  i2c_readReg(SLAVE_I2C_ADDRESS, 0, i2c_dat, MATRIX_ROWS - THIS_DEVICE_ROWS, 0);
  i2c_init();
  i2c_receive(SLAVE_I2C_ADDRESS, i2c_dat, MATRIX_ROWS - THIS_DEVICE_ROWS);
  i2c_uninit();
  uint8_t comm_error=0;
  for (uint8_t i = 0; i < MATRIX_ROWS - THIS_DEVICE_ROWS; i++) {
    if (i2c_dat[i]==0xFF) {
      comm_error=1;
    }
  }
  if (!comm_error) {
    for (uint8_t i = 0; i < MATRIX_ROWS - THIS_DEVICE_ROWS; i++) {
      if (matrix_dummy[i + slave_offset] != i2c_dat[i]) {
        for (uint8_t j = 0; j < MATRIX_COLS; j++) {
          if ((matrix_dummy[i + slave_offset] ^ i2c_dat[i]) & (1 << j)) {
            ble_switch_send[0].dat[0] = 0xff;
            ble_switch_send[0].dat[1] = ((int) sync) % 0xff; // synchronizing packet
            ble_switch_send[slave_matrix_changed + 1].timing = timing;
            ble_switch_send[slave_matrix_changed + 1].state = (i2c_dat[i] >> j) & 1;
            ble_switch_send[slave_matrix_changed + 1].id = i * MATRIX_COLS + j;
            slave_matrix_changed++;
          }
        }
      }
      matrix_dummy[i + slave_offset] = i2c_dat[i];
    }
    for (int i=0; i<slave_matrix_changed; i++) {
      push_queue(&rcv_keys_queue, ble_switch_send[i+1]);
    }
  }
  else {
#ifdef USE_LP_MAT_EXPANDER
    // initialize LP_MAT_EXPANDER
    i2c_init();
    lp_mat_config_t lpconf = LP_MAT_CONFIG;
    uint8_t* plpconf = (uint8_t*)&lpconf;
    lpconf.checksum = 0;
    for(uint8_t i=0; i<sizeof(lpconf)-1; i++) {
      lpconf.checksum ^= plpconf[i];
    }
    i2c_transmit(SLAVE_I2C_ADDRESS, plpconf, sizeof(lpconf));
    i2c_uninit();
#endif
  }
#endif

#endif

/* Power consumption test*/
//  static int cnt1, cnt2;
//  if(cnt1 == 0){
////    matrix[0] |= 0x1;
//    matrix_changed = 1;
//    ble_switch_send[0].timing = 0;
//    ble_switch_send[0].row = 3;
//    ble_switch_send[0].col = 0;
//    ble_switch_send[0].state = 1;
//  } else if(cnt1==3){
////    matrix[0] &= ~0x1;
//    matrix_changed = 1;
//    ble_switch_send[0].timing = 0;
//    ble_switch_send[0].row = 3;
//    ble_switch_send[0].col = 0;
//    ble_switch_send[0].state = 0;
//  }
//  cnt1++;
//  cnt1%=6;
//
//  if(cnt2 == 0){
//      sprintf(str, "%4dmV\n", get_vcc());
////      send_string(str);
//  }
//  cnt2++;
//  cnt2%=10000;

#if defined(USE_AS_I2C_SLAVE)
  i2cs_prepare((uint8_t*)&matrix_dummy[matrix_offset], sizeof(matrix_row_t)*THIS_DEVICE_ROWS);
  UNUSED_VARIABLE(ble_switch_send);
#endif

#ifdef NRF_SEPARATE_KEYBOARD_SLAVE
#ifdef USE_I2C
  i2cs_prepare((uint8_t*)&matrix_dummy[matrix_offset], sizeof(matrix_row_t)*THIS_DEVICE_ROWS);
  UNUSED_VARIABLE(ble_switch_send);
#endif
  if (matrix_changed) {
    NRF_LOG_DEBUG("NUS send");
    ble_nus_send_bytes((uint8_t*) ble_switch_send, (matrix_changed+1)*sizeof(ble_switch_state_t));
    send_flag = true;
  }
#else
  UNUSED_VARIABLE(ble_switch_send);
#endif

  timing++;
  if(timing==0xFF) timing=0;

  // Process received keys
  ble_switch_state_t rcv_key;
  uint8_t slave_time_stamp = 0;
  uint8_t master_time_stamp = 0;
  static uint8_t dowel_count;

  slave_time_stamp = rcv_keys_queue.cnt ? front_queue(&rcv_keys_queue).timing : 0xFF;
  master_time_stamp = delay_keys_queue.cnt ? front_queue(&delay_keys_queue).timing : 0xFF;

  // count delay of master inputs
  dowel_count = timing - front_queue(&delay_keys_queue).timing;

  // master key inputs are proceeded after constant delay or newer slave inputs come.
  if ((master_time_stamp != 0xFF && dowel_count >= BURST_THRESHOLD) || (rcv_keys_queue.cnt &&
      ((master_time_stamp < slave_time_stamp) ))) {
    while (delay_keys_queue.cnt) {
      rcv_key = front_queue(&delay_keys_queue);
      if (master_time_stamp == rcv_key.timing) {
        set_received_key(rcv_key, false);
        pop_queue(&delay_keys_queue, &rcv_key);
      } else {
        break;
      }
    }
  }
  // slave key inputs are proceeded if they are older
  if (master_time_stamp >= slave_time_stamp) {
    while (rcv_keys_queue.cnt) {
      rcv_key = front_queue(&rcv_keys_queue);
      if (slave_time_stamp == rcv_key.timing) {
        set_received_key(rcv_key, true);
        pop_queue(&rcv_keys_queue, &rcv_key);
      } else {
        break;
      }
    }
  }
  return 1;
}

char str[16];

uint8_t matrix_scan(void)
{
  uint8_t res = matrix_scan_impl(matrix);
  matrix_scan_quantum();
  return res;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & ((matrix_row_t)1<<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
}

static void init_rows() {
#if DIODE_DIRECTION == ROW2COL
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_cfg_input(row_pins[i], NRF_GPIO_PIN_PULLUP);
  }
#else
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_cfg(row_pins[i],
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0D1,
        NRF_GPIO_PIN_NOSENSE);
  }
#endif
}
/* Column pin configuration
 */
static void  init_cols(void)
{
#if DIODE_DIRECTION == ROW2COL
  for(int i=0; i<THIS_DEVICE_COLS; i++) {
    nrf_gpio_cfg(col_pins[i],
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0D1,
        NRF_GPIO_PIN_NOSENSE);
  }
#else
  for(int i=0; i<THIS_DEVICE_COLS; i++) {
    nrf_gpio_cfg_input(col_pins[i], NRF_GPIO_PIN_PULLUP);
  }
#endif
}
#if DIODE_DIRECTION == ROW2COL
matrix_row_t matrix_row2col[MATRIX_ROWS];
void scan_row2col_matrix(void)
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
matrix_row_t get_row2col_matrix(uint8_t row)
{
  return matrix_row2col[row];
}

/* Returns status of switches(1:on, 0:off) */
matrix_col_t read_rows(void)
{
  matrix_col_t col = 0;
  for (int i=0; i<THIS_DEVICE_ROWS; i++) {
    col |= ((nrf_gpio_pin_read(row_pins[i]) ? 0 : 1) << i);
  }
  return col;
}

__attribute__ ((weak))
matrix_col_t read_col(uint8_t col)
{
  select_col(col);
  wait_us(0);
  matrix_col_t col_state = read_rows();
  unselect_cols();
  return col_state;
}

void unselect_cols(void)
{
  for(int i=0; i<THIS_DEVICE_COLS; i++) {
    nrf_gpio_pin_set(col_pins[i]);
  }
}

void select_col(uint8_t col)
{
    nrf_gpio_pin_clear(col_pins[col]);
}
#else

/* Returns status of switches(1:on, 0:off) */
matrix_row_t read_cols(void)
{
  matrix_row_t row = 0;
  for (int i=0; i<THIS_DEVICE_COLS; i++) {
    row |= ((nrf_gpio_pin_read(col_pins[i]) ? 0 : 1) << i);
  }
  return row;
}

__attribute__ ((weak))
matrix_row_t read_row(uint8_t row)
{
#ifdef USE_I2C_IOEXPANDER
  return read_row_ioexpander(row);
#else
  select_row(row);
  wait_us(0);
  matrix_row_t row_state = read_cols();
  unselect_rows();
  return row_state;
#endif
}

/* Row pin configuration
 */
void unselect_rows(void)
{
  for(int i=0; i<THIS_DEVICE_ROWS; i++) {
    nrf_gpio_pin_set(row_pins[i]);
  }
}

void select_row(uint8_t row)
{
    nrf_gpio_pin_clear(row_pins[row]);
}
#endif

void ble_nus_on_disconnect() {
#if defined(NRF_SEPARATE_KEYBOARD_MASTER) || defined(NRF_SEPARATE_KEYBOARD_SLAVE)
  uint8_t matrix_offset = isLeftHand ? THIS_DEVICE_ROWS : 0;
  for (uint8_t i = 0; i < THIS_DEVICE_ROWS; i++) {
    matrix[i + matrix_offset] = 0;
  }
  rcv_keys_queue.cnt = 0;
#endif
}

void ble_nus_packetrcv_handler(ble_switch_state_t* buf, uint8_t len) {
  static uint8_t prev_recv_timing;
  int i=0;
  int32_t slave_time_est;
  if (buf[0].dat[0]==0xFF) {
    // master and slave synchronizing
    NRF_LOG_DEBUG("%d %d %d %d",timing, buf[0].dat[1], prev_recv_timing, ((int32_t)timing+buf[0].dat[1]-prev_recv_timing) % 0xFF);
    slave_time_est = (int32_t)buf[0].dat[1];
    timing=((int32_t)timing+slave_time_est-prev_recv_timing) % 0xFF;
    prev_recv_timing = timing;
    i=1;
  }
  for (; i<len; i++) {
    push_queue(&rcv_keys_queue, buf[i]);
  }
}
