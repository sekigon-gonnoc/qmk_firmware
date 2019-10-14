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

#include QMK_KEYBOARD_H
#include "app_ble_func.h"

#ifdef RGBLIGHT_ENABLE
//Following line allows macro to read current RGB settings
extern rgblight_config_t rgblight_config;
#endif

enum custom_keycodes
{
  AD_WO_L = SAFE_RANGE, /* Start advertising without whitelist  */
  BLE_DIS,              /* Disable BLE HID sending              */
  BLE_EN,               /* Enable BLE HID sending               */
  USB_DIS,              /* Disable USB HID sending              */
  USB_EN,               /* Enable USB HID sending               */
  DELBNDS,              /* Delete all bonding                   */
  ADV_ID0,              /* Start advertising to PeerID 0        */
  ADV_ID1,              /* Start advertising to PeerID 1        */
  ADV_ID2,              /* Start advertising to PeerID 2        */
  ADV_ID3,              /* Start advertising to PeerID 3        */
  ADV_ID4,              /* Start advertising to PeerID 4        */
  BATT_LV,              /* Display battery level in milli volts */
  DEL_ID0,              /* Delete bonding of PeerID 0           */
  DEL_ID1,              /* Delete bonding of PeerID 1           */
  DEL_ID2,              /* Delete bonding of PeerID 2           */
  DEL_ID3,              /* Delete bonding of PeerID 3           */
  DEL_ID4,              /* Delete bonding of PeerID 4           */
  ENT_DFU,              /* Start bootloader                     */
  ENT_SLP,              /* Deep sleep mode                      */
  RGBRST,
};

extern keymap_config_t keymap_config;
int RGB_current_mode;

// Fillers to make layering more clear
#define _______ KC_TRNS
#define XXXXXXX KC_NO

enum layers
{
  BASE = 0,
  COMMAND,
  SETTING
};

//#define KC_COMMAND    LT(COMMAND,KC_MHEN)
//#define KC_SETTING    LT(SETTING,KC_HENK)

#define KC_COMMAND LT(COMMAND, KC_MHEN)
#define KC_SETTING LT(SETTING, KC_MHEN)
#define KC_LTOG RGB_TOG
#define KC_LHUI RGB_HUI
#define KC_LHUD RGB_HUD
#define KC_LSAI RGB_SAI
#define KC_LSAD RGB_SAD
#define KC_LVAI RGB_VAI
#define KC_LVAD RGB_VAD
#define KC_LSMOD RGB_SMOD
#define KC_LMOD RGB_MOD

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [BASE] = LAYOUT(
        KC_GRV, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINS, KC_EQL, KC_BSPC,
        KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_BSLS,
        LCTL_T(KC_TAB), KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, KC_ENT,
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_UP, KC_RSFT,
        KC_TAB, KC_LALT, KC_LGUI, KC_COMMAND, KC_SPC, KC_SETTING, KC_APP, KC_LEFT, KC_DOWN, KC_RIGHT, KC_DEL),
    [COMMAND] = LAYOUT(
        KC_ESC, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_BSPC,
        KC_NO, KC_UP, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_CAPS, KC_LEFT, KC_DOWN, KC_RIGHT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, BATT_LV,
        KC_LSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, _______, KC_NO, _______, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO),
    [SETTING] = LAYOUT(
        AD_WO_L, ADV_ID0, ADV_ID1, ADV_ID2, ADV_ID3, ADV_ID4, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, ENT_DFU,
        KC_NO, DEL_ID0, DEL_ID1, DEL_ID2, DEL_ID3, DEL_ID4, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO,KC_LVAD, KC_LVAI, KC_LHUD, KC_LHUI, KC_LSAD, KC_LSAI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, BLE_EN, BLE_DIS, USB_EN, USB_DIS, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_LMOD, RGBRST, _______, BATT_LV, _______, KC_NO, KC_NO, KC_NO, KC_NO, DELBNDS),
};

void SetRGBMOD(void)
{

  rgblight_mode(RGB_current_mode);
  rgblight_step();
  RGB_current_mode = rgblight_config.mode;
}

void SetRGBRST(void)
{
  eeconfig_update_rgblight_default();
  rgblight_enable();
  RGB_current_mode = rgblight_config.mode;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
  char str[16];
  if (record->event.pressed)
  {
    switch (keycode)
    {
    case DELBNDS:
      delete_bonds();
      return false;
    case AD_WO_L:
      restart_advertising_wo_whitelist();
      return false;
    case USB_EN:
      set_usb_enabled(true);
      return false;
      break;
    case USB_DIS:
      set_usb_enabled(false);
      return false;
      break;
    case BLE_EN:
      set_ble_enabled(true);
      return false;
      break;
    case BLE_DIS:
      set_ble_enabled(false);
      return false;
      break;
    case ADV_ID0:
      restart_advertising_id(0);
      return false;
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
    case DEL_ID0:
      delete_bond_id(0);
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
    case BATT_LV:
      sprintf(str, "%4dmV", get_vcc());
      send_string(str);
      return false;
    case ENT_DFU:
      bootloader_jump();
      return false;
#ifdef RGBLIGHT_ENABLE
    case RGB_MOD:
      if (record->event.pressed)
      {
        SetRGBMOD();
      }
      return false;
    case RGBRST:
      if (record->event.pressed)
      {
        SetRGBRST();
      }
      return false;
#endif
    }
  }
  else if (!record->event.pressed)
  {
    switch (keycode)
    {
    case ENT_SLP:
      sleep_mode_enter();
      return false;
    }
  }
  return true;
};

void matrix_init_user_for_keymap(void)
{
#ifdef RGBLIGHT_ENABLE
  SetRGBRST();
#endif
}
