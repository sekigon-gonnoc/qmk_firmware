/*
Copyright 2019 Sekigon

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

#include QMK_KEYBOARD_H
#include "app_ble_func.h"
#include <stdio.h>

enum custom_keycodes {
    ADV_ID1 = SAFE_RANGE, /* Start advertising to PeerID 1                   */
    ADV_ID2,              /* Start advertising to PeerID 2                   */
    ADV_ID3,              /* Start advertising to PeerID 3                   */
    ADV_ID4,              /* Start advertising to PeerID 4                   */
    DEL_ID1,              /* Delete bonding of PeerID 1                      */
    DEL_ID2,              /* Delete bonding of PeerID 2                      */
    DEL_ID3,              /* Delete bonding of PeerID 3                      */
    DEL_ID4,              /* Delete bonding of PeerID 4                      */
    BATT_LV,              /* Display battery level in milli volts            */
    ENT_DFU,              /* Start bootloader                                */
    ENT_SLP,              /* Deep sleep mode                                 */
    SYMBOL,               /* Layer  keycode                                  */
    FN_MOVE,              /* Layer  keycode                                  */
    KC_BSES,              /* Back slash with non modifier, Escape with ctrl  */
};


extern keymap_config_t keymap_config;

enum {
  _QWERTY,
  _FN_MOVE,
  _SYMBOL,
  _SETTING,
};

// Layer related keycodes
#define SYMBOL   MO(_SYMBOL)
#define FN_MOVE  MO(_FN_MOVE)

// Fillers to make layering more clear
#define _______ KC_TRNS
#define XXXXXXX KC_NO

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_QWERTY] = LAYOUT(
 //+--------+--------+--------+--------+--------+--------+                        +--------+--------+--------+--------+--------+--------+
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                             KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSES, \
 //|--------+--------+--------+--------+--------+--------+--------+      +--------+--------+--------+--------+--------+--------+--------|
    KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_SPC,         XXXXXXX, KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_ENT,         KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
                               KC_LALT, SYMBOL,  FN_MOVE, KC_BSPC,        SYMBOL,  KC_SPC,  FN_MOVE, KC_RGUI  \
 //                           +--------+--------+--------+--------+      +--------+--------+--------+--------+
  ),

  [_SYMBOL] = LAYOUT(
 //+--------+--------+--------+--------+--------+--------+                        +--------+--------+--------+--------+--------+--------+
    _______, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                             KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, \
 //|--------+--------+--------+--------+--------+--------+--------+      +--------+--------+--------+--------+--------+--------+--------|
    _______, KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC, XXXXXXX,        XXXXXXX, KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_UNDS, \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
    _______, KC_GRV , KC_TILD, KC_EQL , KC_PLUS, XXXXXXX, XXXXXXX,        XXXXXXX, XXXXXXX, KC_LBRC, KC_RBRC, KC_LCBR, KC_RCBR, XXXXXXX, \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
                               _______, _______, _______, _______,        _______, _______, _______, _______ \
 //                           +--------+--------+--------+--------+      +--------+--------+--------+--------+
  ),

  [_FN_MOVE] = LAYOUT(
 //+--------+--------+--------+--------+--------+--------+                        +--------+--------+--------+--------+--------+--------+
    _______, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                          KC_HOME, KC_PGDN, KC_PGUP, KC_END,  XXXXXXX, XXXXXXX, \
 //|--------+--------+--------+--------+--------+--------+--------+      +--------+--------+--------+--------+--------+--------+--------|
    _______, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,        XXXXXXX, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, XXXXXXX, KC_F12,  \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
    _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   XXXXXXX,        XXXXXXX, KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
                               _______, _______, _______, KC_DEL,         _______, KC_ENT,  _______, _______ \
 //                           +--------+--------+--------+--------+      +--------+--------+--------+--------+
  ),

  [_SETTING] = LAYOUT(
 //+--------+--------+--------+--------+--------+--------+                        +--------+--------+--------+--------+--------+--------+
    BATT_LV, ADV_ID1, ADV_ID2, ADV_ID3, ADV_ID4, XXXXXXX,                          XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
 //|--------+--------+--------+--------+--------+--------+--------+      +--------+--------+--------+--------+--------+--------+--------|
    ENT_SLP, DEL_ID1, DEL_ID2, DEL_ID3, DEL_ID4, XXXXXXX, XXXXXXX,        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
    ENT_DFU, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, \
 //|--------+--------+--------+--------+--------+--------+--------|      |--------+--------+--------+--------+--------+--------+--------|
                               _______, _______, _______, _______,        _______, _______, _______, _______ \
 //                           +--------+--------+--------+--------+      +--------+--------+--------+--------+
  ),
};

static bool lctrl_pressed = false;
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  char str[16];
  switch (keycode) {
    // layers
    case SYMBOL:
      if (record->event.pressed) {
        layer_on(_SYMBOL);
        update_tri_layer(_SYMBOL, _FN_MOVE, _SETTING);
      } else {
        layer_off(_SYMBOL);
        update_tri_layer(_SYMBOL, _FN_MOVE, _SETTING);
      }
      return false;
      break;
    case FN_MOVE:
      if (record->event.pressed) {
        layer_on(_FN_MOVE);
        update_tri_layer(_SYMBOL, _FN_MOVE, _SETTING);
      } else {
        layer_off(_FN_MOVE);
        update_tri_layer(_SYMBOL, _FN_MOVE, _SETTING);
      }
      return false;
      break;

    // back slash and escape key
    case KC_BSES:
      if (record->event.pressed) {
        if (lctrl_pressed) {
          unregister_code(KC_LCTRL);
          register_code(KC_ESC);
        } else {
          register_code(KC_BSLS);
        }
      } else {
        unregister_code(KC_ESC);
        unregister_code(KC_BSLS);
        if (lctrl_pressed) {
          register_code(KC_LCTRL);
        } 
      }
      return false;
      break;
  }
  if (record->event.pressed) {
    switch (keycode) {
    // Bluetooth peering commands
    case ADV_ID1:
      restart_advertising_id(1);
      return false;
    case ADV_ID2:
      restart_advertising_id(2);
      return false;
    case ADV_ID3:
      restart_advertising_id(3);
      return false;
    case ADV_ID4:
      restart_advertising_id(4);
      return false;
    case DEL_ID1:
      delete_bond_id(1);
      return false;
    case DEL_ID2:
      delete_bond_id(2);
      return false;
    case DEL_ID3:
      delete_bond_id(3);
      return false;
    case DEL_ID4:
      delete_bond_id(4);
      return false;

    // Output battery level
    case BATT_LV:
      sprintf(str, "%4d", get_vcc());
      send_string(str);
      return false;

    case ENT_DFU:
      bootloader_jump();
      return false;

    case KC_LCTRL:
      lctrl_pressed = true;
      return true;
    }
  }
  else if (!record->event.pressed) {
    switch (keycode) {
    case ENT_SLP:
      sleep_mode_enter();
      return false;
    case KC_LCTRL:
      lctrl_pressed = false;
      return false;
    }

  }
  return true;
}
;
