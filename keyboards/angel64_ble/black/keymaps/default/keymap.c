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
#ifdef RGBLIGHT_ENABLE
#define USE_LED_RIPPLE_EFFECT
// #define RANDOM_RIPPLE_EFFECT
#endif

#include QMK_KEYBOARD_H
#include "app_ble_func.h"
#ifdef SSD1306OLED
  #include "ssd1306.h"
#endif


int RGB_current_mode;
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
  USE_RIPPLE,
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

#define KC_COMMAND LT(COMMAND, KC_F13)
#define KC_SETTING LT(SETTING, KC_F14)
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
        KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_BSLS,
        KC_LCTL, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, KC_ENT,
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT,
        KC_LCTL, KC_LALT, KC_LGUI, KC_COMMAND, KC_SPC, KC_SPC, KC_SPC, KC_SETTING, KC_RALT, MO(COMMAND),KC_DEL),
    [COMMAND] = LAYOUT(
        KC_ESC, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_DEL,
        KC_NO, KC_NO, KC_UP, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_CAPS, KC_LEFT, KC_DOWN, KC_RIGHT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_LSFT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_LCTL, KC_NO,  KC_NO, _______, KC_NO, KC_NO, KC_NO,  _______, KC_NO,  _______, KC_NO),
    [SETTING] = LAYOUT(
        AD_WO_L, ADV_ID0, ADV_ID1, ADV_ID2, ADV_ID3, ADV_ID4, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, ENT_DFU,
        KC_NO, DEL_ID0, DEL_ID1, DEL_ID2, DEL_ID3, DEL_ID4, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO,KC_LVAD, KC_LVAI, KC_LHUD, KC_LHUI, KC_LSAD, KC_LSAI, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        USE_RIPPLE, BLE_EN, BLE_DIS, USB_EN, USB_DIS, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
        RGB_TOG, KC_LMOD, RGBRST, _______, BATT_LV, BATT_LV, BATT_LV, _______, KC_NO, KC_NO,  DELBNDS),
};

#ifdef USE_LED_RIPPLE_EFFECT
struct keybuf {
  char col, row;
  char frame;
};
#define LED_KEY_BUFF 32
struct keybuf keybufs[LED_KEY_BUFF];
unsigned char keybuf_begin, keybuf_end;

unsigned char r = 0;
unsigned char g = 112;
unsigned char b = 255;
int col, row;
#endif

bool use_ripple = true;

void SetRGBMOD(void)
{
  rgblight_mode(RGB_current_mode);
  rgblight_step();
  RGB_current_mode = rgblight_config.mode;
  use_ripple = false;
}

void SetRGBRST(void)
{
  eeconfig_update_rgblight_default();
  rgblight_enable();
  RGB_current_mode = rgblight_config.mode;
  use_ripple = false;
}

void SetRIPPLE(void)
{
#ifdef USE_LED_RIPPLE_EFFECT
  eeconfig_update_rgblight_default();
  rgblight_enable_noeeprom();
  rgblight_mode(0);
  use_ripple = true;
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
#ifdef USE_LED_RIPPLE_EFFECT
    col = record->event.key.col;
    row = record->event.key.row;
    if (record->event.pressed) {
      int end = keybuf_end;
      keybufs[end].col = col;
      keybufs[end].row = row;
      keybufs[end].frame = 0;
      keybuf_end = (keybuf_end + 1) % LED_KEY_BUFF;
    }
#endif
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
    case USE_RIPPLE:
      if (record->event.pressed)
      {
        SetRIPPLE();
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

#ifdef USE_LED_RIPPLE_EFFECT
int scan_count = -10;
int keys[] = { 14, 14, 13, 12, 11 };
int keys_sum[] = { 0, 14, 28, 41, 53 };
unsigned char rgb[14][5][3];
int row_max = 12;
int ToIndex(char _col, char _row) {
    return (_col * row_max) + _row;
}

void led_ripple_effect(void){
    if (scan_count == -1) {
      rgblight_enable_noeeprom();
      rgblight_mode(0);
    } else if (scan_count == 0) {
      // Create lipple effect
      memset(rgb, 0, sizeof(rgb));

      for (unsigned char c=keybuf_begin; c!=keybuf_end;  c=(c+1)%LED_KEY_BUFF ) {
        int i = c;
        // FIXME:

        int index = ToIndex(keybufs[i].col,keybufs[i].row);
        int coll = 0;
        for(int s = 4; s >= 0; --s){
            if( index >= keys_sum[s]){
                coll = s;
                break;
            }
        }
        int roww = MAX(0,index - keys_sum[coll]);


        for (int y = 0; y < 5; y++){
          int dist_y = abs(y - coll);
          for (int x=0; x<keys[y]; x++) {
            int dist = abs(x - roww) + dist_y;
            if (dist <= keybufs[i].frame) {
              int elevation = MAX(0, (8 + dist - keybufs[i].frame)) << 2;
              if (elevation) {
                if ((rgb[x][y][0] != 255) && r) { rgb[x][y][0] = MIN(255, elevation + rgb[x][y][0]); }
                if ((rgb[x][y][1] != 255) && g) { rgb[x][y][1] = MIN(255, elevation + rgb[x][y][1]); }
                if ((rgb[x][y][2] != 255) && b) { rgb[x][y][2] = MIN(255, elevation + rgb[x][y][2]); }
              }
            }
          }
        }
        if (keybufs[i].frame < 64) {
          keybufs[i].frame ++;
        } else {
          keybuf_begin = (keybuf_begin + 1) % LED_KEY_BUFF;
        }
      }
    } else if (scan_count == 1) {
      // Update LEDs.
      for (int y=0; y<5; y++) {
        for (int x=0; x<keys[y]; x++) {
          int at = keys_sum[y] + x ;
          led[at].r = rgb[x][y][0];
          led[at].g = rgb[x][y][1];
          led[at].b = rgb[x][y][2];
        }
      }
      rgblight_set();
    }
    scan_count++;
    if (scan_count >= 2) { scan_count = 0; }
}

#endif

void matrix_init_user_for_keymap() {
//  rgblight_mode_noeeprom(35);
//  set_usb_enabled(true);
  //SSD1306 OLED init, make sure to add #define SSD1306OLED in config.h
  #ifdef SSD1306OLED
      iota_gfx_init(!IS_LEFT_HAND);   // turns on the display
  #endif

#ifdef USE_LED_RIPPLE_EFFECT
    SetRIPPLE();
#endif
}

//runs every scan cycle (a lot)
void matrix_scan_user(void) {
  #ifdef SSD1306OLED
    iota_gfx_task();  // this is what updates the display continuously
  #endif

#ifdef USE_LED_RIPPLE_EFFECT
#ifdef RANDOM_RIPPLE_EFFECT
    static int timer = 0;
    static int timeout = 10;
    timer++;
    if(timer > timeout){
      int end = keybuf_end;
      col = rand() % 6;
      row = rand() % 12;
      keybufs[end].col = col;
      keybufs[end].row = row;
      keybufs[end].frame = 0;
      keybuf_end = (keybuf_end + 1) % LED_KEY_BUFF;
      timer = rand() % timeout;
    }
#endif
    if(use_ripple)
    {
      led_ripple_effect();
    }
#endif
}

//SSD1306 OLED update loop, make sure to add #define SSD1306OLED in config.h
#ifdef SSD1306OLED
void matrix_update(struct CharacterMatrix *dest,
                          const struct CharacterMatrix *source) {
  if (memcmp(dest->display, source->display, sizeof(dest->display))) {
    memcpy(dest->display, source->display, sizeof(dest->display));
    dest->dirty = true;
  }
}

/*
static void render_logo(struct CharacterMatrix *matrix) {

  static char logo[]={
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,
    0};
  matrix_write(matrix, logo);
  //matrix_write_P(&matrix, PSTR(" Split keyboard kit"));
}

*/
void render_status(struct CharacterMatrix *matrix) {

  // Render to mode icon
  static char logo[][2][3]={{{0x95,0x96,0},{0xb5,0xb6,0}},{{0x97,0x98,0},{0xb7,0xb8,0}}};
  if(keymap_config.swap_lalt_lgui==false){
    matrix_write(matrix, logo[0][0]);
    matrix_write_P(matrix, PSTR("\n"));
    matrix_write(matrix, logo[0][1]);
  }else{
    matrix_write(matrix, logo[1][0]);
    matrix_write_P(matrix, PSTR("\n"));
    matrix_write(matrix, logo[1][1]);
  }

  matrix_write_P(matrix, PSTR("\nLayer: "));
  matrix_write_P(matrix, PSTR("Default\n"));

#ifdef USE_LED_RIPPLE_EFFECT
  char buf[40];
  snprintf(buf,sizeof(buf), "col:%d row:%d", col, row);
  matrix_write(matrix, buf);
  matrix_write_P(matrix, PSTR("\n"));
#endif

  // Host Keyboard LED Status
  char led[40];
    snprintf(led, sizeof(led), "\n%s  %s  %s",
            (host_keyboard_leds() & (1<<USB_LED_NUM_LOCK)) ? "NUMLOCK" : "       ",
            (host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK)) ? "CAPS" : "    ",
            (host_keyboard_leds() & (1<<USB_LED_SCROLL_LOCK)) ? "SCLK" : "    ");
  matrix_write(matrix, led);
}


void iota_gfx_task_user(void) {
  struct CharacterMatrix matrix;

#if DEBUG_TO_SCREEN
  if (debug_enable) {
    return;
  }
#endif

  matrix_clear(&matrix);
  //if(is_master){
    render_status(&matrix);
  //}else{
  // render_logo(&matrix);
  //}
  matrix_update(&display, &matrix);
}
#endif
