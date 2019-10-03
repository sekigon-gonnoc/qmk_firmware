/* Copyright 2019 _hatano_
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H
#include "bmp.h"
#include "keycode_str_converter.h"
#include <string.h>

// Defines the keycodes used by our macros in process_record_user
enum custom_keycodes {
    // kanji keys
    xEISU = BMP_SAFE_RANGE,
    xKANA,

    // BLE
    SEL_BLE,			// Select BLE HID
    SEL_USB,			// Select USB HID
    TOG_HID,			// Toggle BLE/USB HID

    // combination tap / layer
    m0EISU,
    m0KANA,
    m1EISU,
    m1KANA,
    m2EISU,
    m2KANA,
    m3EISU,
    m3KANA,

    // end
    END_KEYCODE,
};

extern keymap_config_t keymap_config;

#define LAYERSIZE 16

enum layers {
    _BL0 = 0,		// Basic Layer
    _BL1,		// Altanative Layer (e.g. 10Key pad)
    _ML0,		// Raise / Nums
    _ML1,		// Lower / Syms
    _ML2,		// Adjust = Raise + Lower / Func = Nums + Syms
    _ML3,		// Config
};

const key_string_map_t custom_keys_user =
{
    .start_kc = BMP_SAFE_RANGE,
    .end_kc= END_KEYCODE - 1,
    .key_strings =
    "xEISU\0xKANA\0"
    "SEL_BLE\0SEL_USB\0TOG_HID\0"
    "m0EISU\0m0KANA\0"
    "m1EISU\0m1KANA\0"
    "m2EISU\0m2KANA\0"
    "m3EISU\0m3KANA\0"
};

////////////////////////////////////////////////////////////////////////
// 
static struct {
  bool pressed;
  uint16_t time;
} layer_pressed[LAYERSIZE];

static void set_keyevent(int layer, keyrecord_t *record)
{
  layer_pressed[layer].time = record->event.time;
  layer_pressed[layer].pressed = record->event.pressed;
}

static void clear_keyevent()
{
  memset(layer_pressed, 0, sizeof(layer_pressed));
}

static int is_tapkey(int layer, int time, keyrecord_t *record)
{
  return layer_pressed[layer].pressed && 
    (TIMER_DIFF_16(record->event.time, layer_pressed[layer].time) < time );
}

static bool set_layer(int layer, keyrecord_t *record);
static void unset_layer(keyrecord_t *record);
static bool set_layer_and_key(uint16_t keycode, int layer, uint16_t kc, keyrecord_t *record);
static bool process_record_user_custom(uint16_t keycode, keyrecord_t *record);;
static void update_tri_layer_user();
////////////////////////////////////////////////////////////////////////
// 
static bool set_layer(int layer, keyrecord_t *record)
{
  if (record->event.pressed) {
    layer_on(layer);
  } else {
    layer_off(layer);
  }
  set_keyevent(layer, record);
  update_tri_layer_user();
  return false;
}

static void unset_layer(keyrecord_t *record)
{
  if (record->event.pressed) {
    clear_keyevent();
  }
}

extern uint16_t get_tapping_term(uint16_t keycode);
static bool set_layer_and_key(uint16_t keycode, int layer, uint16_t kc, keyrecord_t *record)
{
  if (record->event.pressed) {
    layer_on(layer);
  } else {
    layer_off(layer);
    if (is_tapkey(layer, get_tapping_term(keycode), record)) {
      record->event.pressed = true;
      if (process_record_user_custom(kc, record))
	register_code(kc);
      record->event.pressed = false;
      if (process_record_user_custom(kc, record))
	unregister_code(kc);
    }
  }
  set_keyevent(layer, record);
  update_tri_layer_user();
  return false;
}

#define LAYERx_CODE 0x5F00
#define LAYERy_CODE 0x5F01
#define LAYERz_CODE 0x5F02
static bool get_param(uint16_t code, int *value)
{
  int v_no = get_tapping_term(KC_NO);
  int v = get_tapping_term(code);

  *value = v;
  return v_no != v;
}

static void update_tri_layer_user()
{
  int x,y,z;

  if (get_param(LAYERx_CODE, &x) &&
      get_param(LAYERy_CODE, &y) &&
      get_param(LAYERz_CODE, &z))
    update_tri_layer(x, y, z);
}
////////////////////////////////////////////////////////////////////////
// custum key function
//
static bool process_record_user_custom(uint16_t keycode, keyrecord_t *record)
{
  bool continue_process = process_record_user_bmp(keycode, record);

  if (continue_process) {
    bool pressed = record->event.pressed;

    switch (keycode) {
      // eisu / kana
    case xEISU:
      if (pressed) {
	if (keymap_config.swap_lalt_lgui){
	  register_code(KC_LANG2);
	} else {
	  SEND_STRING(SS_LALT("`"));
	}
      } else {
	unregister_code(KC_LANG2);
      }
      break;
    case xKANA:
      if (pressed) {
	if (keymap_config.swap_lalt_lgui){
	  register_code(KC_LANG1);
	} else {
	  SEND_STRING(SS_LALT("`"));
	}
      } else {
	unregister_code(KC_LANG1);
      }
      break;

      // special BLE key
    case SEL_BLE:
      if (pressed) {
	set_ble_enabled(true);
	set_usb_enabled(false);
      }
      break;
    case SEL_USB:
      if (pressed) {
	set_ble_enabled(false);
	set_usb_enabled(true);
      }
      break;
    case TOG_HID:
      if (pressed) {
	bool ble = get_ble_enabled();

	set_ble_enabled(!ble);
	set_usb_enabled(ble);
      }
      break;

      // other unspecial keys
    default:
      return true;
    }
  }
  return false;
}
////////////////////////////////////////////////////////////////////////
// extern
void matrix_init_user(void)
{
  clear_keyevent();
  set_ble_enabled(true);
  set_usb_enabled(false);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
  switch (keycode) {
    // override MO() macro
  case QK_MOMENTARY ... QK_MOMENTARY_MAX:
    return set_layer(keycode & 0xF, record);

    // override LT() macro
  case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
    return set_layer_and_key(keycode, 
			     (keycode&0x0F00)>>8, keycode & 0xFF, record);

    // combine keycode and layer
  case m0EISU:
    return set_layer_and_key(keycode, _ML0, xEISU, record);
  case m0KANA:
    return set_layer_and_key(keycode, _ML0, xKANA, record);
  case m1EISU:
    return set_layer_and_key(keycode, _ML1, xEISU, record);
  case m1KANA:
    return set_layer_and_key(keycode, _ML1, xKANA, record);
  case m2EISU:
    return set_layer_and_key(keycode, _ML2, xEISU, record);
  case m2KANA:
    return set_layer_and_key(keycode, _ML2, xKANA, record);
  case m3EISU:
    return set_layer_and_key(keycode, _ML3, xEISU, record);
  case m3KANA:
    return set_layer_and_key(keycode, _ML3, xKANA, record);

    // special function key
  default:
    unset_layer(record);
    return process_record_user_custom(keycode, record);
  }
  return false;
}

////////////////////////////////////////////////////////////////////////

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    {{
    }}
};

#if 0
uint32_t keymaps_len()
{ 
 return sizeof(keymaps)/sizeof(uint16_t);
}

void matrix_scan_user(void)
{
}

void led_set_user(uint8_t usb_led)
{
}
#endif
