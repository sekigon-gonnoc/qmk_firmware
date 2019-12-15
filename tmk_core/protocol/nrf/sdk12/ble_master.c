/**
 * Copyright (c) 2012 - 2018, Nordic Semiconductor ASA
 * Copyright (c) 2018, Sekigon
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "nrf_assert.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advertising.h"
#include "ble_advdata.h"
#include "ble_hids.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "softdevice_handler_appsh.h"
#include "app_button.h"
#include "fds.h"
#include "fstorage.h"
#include "ble_conn_state.h"
#include "app_timer_appsh.h"

#include "ble_common.h"

#if (NRF_SD_BLE_API_VERSION == 3)
#define NRF_BLE_MAX_MTU_SIZE            GATT_MTU_SIZE_DEFAULT                       /**< MTU size used in the softdevice enabling and to reply to a BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST event. */
#endif

#define UART_TX_BUF_SIZE                 256                                        /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                 1                                          /**< UART RX buffer size. */

#define KEY_PRESS_BUTTON_ID              0                                          /**< Button used as Keyboard key press. */
#define SHIFT_BUTTON_ID                  1                                          /**< Button used as 'SHIFT' Key. */

#define DEVICE_NAME                      "QMK Keyboad"                          /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                "NordicSemiconductor"                      /**< Manufacturer. Will be passed to Device Information Service. */

#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          50                                          /**< Size of timer operation queues. */

#define BATTERY_LEVEL_MEAS_INTERVAL      APP_TIMER_TICKS(2000, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). */
#define MIN_BATTERY_LEVEL                81                                         /**< Minimum simulated battery level. */
#define MAX_BATTERY_LEVEL                100                                        /**< Maximum simulated battery level. */
#define BATTERY_LEVEL_INCREMENT          1                                          /**< Increment between each simulated battery level measurement. */

#define PNP_ID_VENDOR_ID_SOURCE          0x02                                       /**< Vendor ID Source. */
#define PNP_ID_VENDOR_ID                 0x1915                                     /**< Vendor ID. */
#define PNP_ID_PRODUCT_ID                0xEEEE                                     /**< Product ID. */
#define PNP_ID_PRODUCT_VERSION           0x0001                                     /**< Product Version. */

#define APP_ADV_FAST_INTERVAL            0x0028                                     /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_SLOW_INTERVAL            0x0C80                                     /**< Slow advertising interval (in units of 0.625 ms. This value corrsponds to 2 seconds). */
#define APP_ADV_FAST_TIMEOUT             30                                         /**< The duration of the fast advertising period (in seconds). */
#define APP_ADV_SLOW_TIMEOUT             180                                        /**< The duration of the slow advertising period (in seconds). */

/*lint -emacro(524, MIN_CONN_INTERVAL) // Loss of precision */
#ifndef BLE_HID_MAX_INTERVAL
  #define BLE_HID_MAX_INTERVAL 90
#endif
#ifndef BLE_HID_SLAVE_LATENCY
  #define BLE_HID_SLAVE_LATENCY 4
#endif
//#ifndef BLE_HID_TIMEOUT
//  #define BLE_HID_TIMEOUT 1000
//#endif
#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(30, UNIT_1_25_MS)           /**< Minimum connection interval (7.5 ms) */
#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(BLE_HID_MAX_INTERVAL, UNIT_1_25_MS)            /**< Maximum connection interval (30 ms). */
#define SLAVE_LATENCY                       BLE_HID_SLAVE_LATENCY                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS((SLAVE_LATENCY+2)*BLE_HID_MAX_INTERVAL*2, UNIT_10_MS)             /**< Connection supervisory timeout (430 ms). */
// TIMEOUT > (LATENCY+1)*MAX_INTERVAL*2

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                   1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                   0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS               0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                          /**< Maximum encryption key size. */

#define OUTPUT_REPORT_INDEX              0                                           /**< Index of Output Report. */
#define OUTPUT_REPORT_MAX_LEN            1                                           /**< Maximum length of Output Report. */
#define INPUT_REPORT_KEYS_INDEX          0                                           /**< Index of Input Report. */
#define OUTPUT_REPORT_BIT_MASK_CAPS_LOCK 0x02                                        /**< CAPS LOCK bit in Output Report (based on 'LED Page (0x08)' of the Universal Serial Bus HID Usage Tables). */
#define INPUT_REP_REF_ID                 0                                           /**< Id of reference to Keyboard Input Report. */
#define OUTPUT_REP_REF_ID                0                                           /**< Id of reference to Keyboard Output Report. */

#define APP_FEATURE_NOT_SUPPORTED        BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define MAX_BUFFER_ENTRIES               5                                           /**< Number of elements that can be enqueued */

#define BASE_USB_HID_SPEC_VERSION        0x0101                                      /**< Version number of base USB HID Specification implemented by this application. */

#define INPUT_REPORT_KEYS_MAX_LEN        8                                           /**< Maximum length of the Input Report characteristic. */

#define DEAD_BEEF                        0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


#define MODIFIER_KEY_POS                 0                                           /**< Position of the modifier byte in the Input Report. */
#define SCAN_CODE_POS                    2                                           /**< This macro indicates the start position of the key scan code in a HID Report. As per the document titled 'Device Class Definition for Human Interface Devices (HID) V1.11, each report shall have one modifier byte followed by a reserved constant byte and then the key scan code. */
#define SHIFT_KEY_CODE                   0x02                                        /**< Key code indicating the press of the Shift Key. */

#define MAX_KEYS_IN_ONE_REPORT           (INPUT_REPORT_KEYS_MAX_LEN - SCAN_CODE_POS) /**< Maximum number of key presses that can be sent in one Input Report. */

typedef enum {
  BLE_NO_ADV, /**< No advertising running. */
  BLE_DIRECTED_ADV, /**< Direct advertising to the latest central. */
  BLE_FAST_ADV_WHITELIST, /**< Advertising with whitelist. */
  BLE_FAST_ADV, /**< Fast advertising running. */
  BLE_SLOW_ADV, /**< Slow advertising running. */
  BLE_SLEEP, /**< Go to system-off. */
} ble_advertising_mode_t;

#include "ble_master.h"
#include "ble_central.h"
#include "ble_report_def.h"

static ble_hids_t m_hids_composite; /**< Structure used to identify the HID service. */
static ble_bas_t m_bas; /**< Structure used to identify the battery service. */
static bool m_in_boot_mode = false; /**< Current protocol mode. */
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

APP_TIMER_DEF( m_battery_timer_id); /**< Battery timer. */
APP_TIMER_DEF( main_task_timer_id);

static pm_peer_id_t m_peer_id; /**< Device reference handle to the current bonded central. */
static bool m_caps_on = false; /**< Variable to indicate if Caps Lock is turned on. */

static pm_peer_id_t m_whitelist_peers[BLE_GAP_WHITELIST_ADDR_MAX_COUNT]; /**< List of peers currently in the whitelist. */
uint32_t m_whitelist_peer_cnt; /**< Number of peers currently in the whitelist. */
static bool m_is_wl_changed; /**< Indicates if the whitelist has been changed since last time it has been updated in the Peer Manager. */

static ble_uuid_t m_adv_uuids[] = { { BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE,
    BLE_UUID_TYPE_BLE } };

void ble_advertising_modes_config_set(ble_adv_modes_config_t const * const p_adv_modes_config);
void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name) {
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for starting advertising.
 */
void advertising_start(void) {
  ret_code_t ret;

  memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
  m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

  peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

  ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
  APP_ERROR_CHECK(ret);

  // Setup the device identies list.
  // Some SoftDevices do not support this feature.
  ret = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
  if (ret != NRF_ERROR_NOT_SUPPORTED) {
    APP_ERROR_CHECK(ret);
  }

  m_is_wl_changed = false;

  ret = ble_advertising_start(BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(ret);
}

void ble_disconnect() {
  sd_ble_gap_adv_stop();
  if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
    ret_code_t err_code = sd_ble_gap_disconnect(m_conn_handle,
    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_ERROR_INVALID_STATE) {
      APP_ERROR_CHECK(err_code);
    }
  }
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
void pm_evt_handler(pm_evt_t const * p_evt) {
  ret_code_t err_code;

  switch (p_evt->evt_id) {
  case PM_EVT_BONDED_PEER_CONNECTED: {
    NRF_LOG_INFO("Connected to a previously bonded device: ID%d.\r\n",p_evt->peer_id);
  }
    break;

  case PM_EVT_CONN_SEC_SUCCEEDED: {
    NRF_LOG_INFO(
        "Connection secured. Role: %d. conn_handle: %d, Procedure: %d\r\n",
        ble_conn_state_role(p_evt->conn_handle), p_evt->conn_handle,
        p_evt->params.conn_sec_succeeded.procedure);

    m_peer_id = p_evt->peer_id;

    // Note: You should check on what kind of white list policy your application should use.
    if (p_evt->params.conn_sec_succeeded.procedure
        == PM_LINK_SECURED_PROCEDURE_BONDING) {
      NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible\r\n");
      NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d\r\n",
          m_whitelist_peer_cnt + 1, BLE_GAP_WHITELIST_ADDR_MAX_COUNT);

      if (m_whitelist_peer_cnt < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) {
        // Bonded to a new peer, add it to the whitelist.
        m_whitelist_peers[m_whitelist_peer_cnt++] = m_peer_id;
        m_is_wl_changed = true;
      }
    }
  }
    break;

  case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED: {
    if (p_evt->params.peer_data_update_succeeded.flash_changed
        && (p_evt->params.peer_data_update_succeeded.data_id
            == PM_PEER_DATA_ID_BONDING)) {
      NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
//      advertising_start();
      NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
          m_whitelist_peer_cnt + 1, BLE_GAP_WHITELIST_ADDR_MAX_COUNT);
      // Note: You should check on what kind of white list policy your application should use.

      if (m_whitelist_peer_cnt < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) {
        // Bonded to a new peer, add it to the whitelist.
        m_whitelist_peers[m_whitelist_peer_cnt++] = m_peer_id;

        // The whitelist has been modified, update it in the Peer Manager.
        err_code = pm_device_identities_list_set(m_whitelist_peers,
            m_whitelist_peer_cnt);
//        if (err_code != NRF_ERROR_NOT_SUPPORTED) {
//          APP_ERROR_CHECK(err_code);
//        }

        err_code = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
        APP_ERROR_CHECK(err_code);
      }
    }
  }
    break;

  case PM_EVT_CONN_SEC_FAILED: {
    /* Often, when securing fails, it shouldn't be restarted, for security reasons.
     * Other times, it can be restarted directly.
     * Sometimes it can be restarted, but only after changing some Security Parameters.
     * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
     * Sometimes it is impossible, to secure the link, or the peer device does not support it.
     * How to handle this error is highly application dependent. */
  }
    break;

  case PM_EVT_CONN_SEC_CONFIG_REQ: {
    // Reject pairing request from an already bonded peer.
    pm_conn_sec_config_t conn_sec_config = { .allow_repairing = false };
    pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
  }
    break;

  case PM_EVT_STORAGE_FULL: {
    // Run garbage collection on the flash.
    err_code = fds_gc();
    if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES) {
      // Retry.
    } else {
      APP_ERROR_CHECK(err_code);
    }
  }
    break;

  case PM_EVT_PEERS_DELETE_SUCCEEDED: {
    advertising_start();
  }
    break;

  case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED: {
    // The local database has likely changed, send service changed indications.
    pm_local_database_has_changed();
  }
    break;

  case PM_EVT_PEER_DATA_UPDATE_FAILED: {
    // Assert.
    APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
  }
    break;

  case PM_EVT_PEER_DELETE_FAILED: {
    // Assert.
    APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
  }
    break;

  case PM_EVT_PEERS_DELETE_FAILED: {
    // Assert.
    APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
  }
    break;

  case PM_EVT_ERROR_UNEXPECTED: {
    // Assert.
    APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
  }
    break;

  case PM_EVT_CONN_SEC_START:
  case PM_EVT_PEER_DELETE_SUCCEEDED:
  case PM_EVT_LOCAL_DB_CACHE_APPLIED:
  case PM_EVT_SERVICE_CHANGED_IND_SENT:
  case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
  default:
    break;
  }
}

/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
void service_error_handler(uint32_t nrf_error) {
  APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
void ble_advertising_error_handler(uint32_t nrf_error) {
  APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for performing a battery measurement, and update the Battery Level characteristic in the Battery Service.
 */
void battery_level_update(void) {
  uint32_t err_code;
  uint8_t battery_level;

  adc_start();
  battery_level = get_vcc() / 30;

  err_code = ble_bas_battery_level_update(&m_bas, battery_level);
  if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_INVALID_STATE)
      && (err_code != BLE_ERROR_NO_TX_PACKETS)
      && (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {
//    APP_ERROR_HANDLER(err_code);
  }
}

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
void battery_level_meas_timeout_handler(void * p_context) {
  UNUSED_PARAMETER(p_context);
  battery_level_update();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
void timers_init(void (*main_task)(void*)) {
  uint32_t err_code;

  // Initialize timer module, making it use the scheduler.
  APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, true);

  // Create battery timer.
  err_code = app_timer_create(&m_battery_timer_id, APP_TIMER_MODE_REPEATED,
      battery_level_meas_timeout_handler);
  APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&main_task_timer_id, APP_TIMER_MODE_REPEATED,
      main_task);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
void gap_params_init(void) {
  uint32_t err_code;
  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(&sec_mode,
      (const uint8_t*) STR(PRODUCT), strlen(STR(PRODUCT)));
  APP_ERROR_CHECK(err_code);

  err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_HID);
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing Device Information Service.
 */
void dis_init(void) {
  uint32_t err_code;
  ble_dis_init_t dis_init_obj;
  ble_dis_pnp_id_t pnp_id;

  pnp_id.vendor_id_source = PNP_ID_VENDOR_ID_SOURCE;
  pnp_id.vendor_id = VENDOR_ID;
  pnp_id.product_id = PRODUCT_ID;
  pnp_id.product_version = DEVICE_VER;

  memset(&dis_init_obj, 0, sizeof(dis_init_obj));

  ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, STR(MANUFACTURE));
  dis_init_obj.p_pnp_id = &pnp_id;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&dis_init_obj.dis_attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init_obj.dis_attr_md.write_perm);

  err_code = ble_dis_init(&dis_init_obj);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing Battery Service.
 */
void bas_init(void) {
  uint32_t err_code;
  ble_bas_init_t bas_init_obj;

  memset(&bas_init_obj, 0, sizeof(bas_init_obj));

  bas_init_obj.evt_handler = NULL;
  bas_init_obj.support_notification = true;
  bas_init_obj.p_report_ref = NULL;
  bas_init_obj.initial_batt_level = 100;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &bas_init_obj.battery_level_char_attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(
      &bas_init_obj.battery_level_char_attr_md.write_perm);

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &bas_init_obj.battery_level_report_read_perm);

  err_code = ble_bas_init(&m_bas, &bas_init_obj);
  APP_ERROR_CHECK(err_code);
}

void composite_service_init(void) {
  uint32_t err_code;
  ble_hids_init_t composite_service_init_obj;
  ble_hids_inp_rep_init_t input_report_array[COMPOSITE_REPORT_COUNT];
  ble_hids_inp_rep_init_t * p_input_report;
  ble_hids_outp_rep_init_t output_report_array[1];
  ble_hids_outp_rep_init_t * p_output_report;
  uint8_t hid_info_flags;

  memset((void *) input_report_array, 0, sizeof(ble_hids_inp_rep_init_t));
  memset((void *) output_report_array, 0, sizeof(ble_hids_outp_rep_init_t));

  // Initialize HID Service
  p_input_report = &input_report_array[COMPOSITE_REPORT_INDEX_KEYBOARD];
#ifdef NKRO_ENABLE
  p_input_report->max_len = NKRO_BYTE_LEN;
#else
  p_input_report->max_len = INPUT_REPORT_KEYS_MAX_LEN;
#endif
  p_input_report->rep_ref.report_id = COMPOSITE_REPORT_ID_KEYBOARD;
  p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.write_perm);

  p_input_report = &input_report_array[COMPOSITE_REPORT_INDEX_MOUSE];
  p_input_report->max_len = 5;
  p_input_report->rep_ref.report_id = COMPOSITE_REPORT_ID_MOUSE;
  p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.write_perm);

  p_input_report = &input_report_array[COMPOSITE_REPORT_INDEX_SYSTEM];
  p_input_report->max_len = 2;
  p_input_report->rep_ref.report_id = COMPOSITE_REPORT_ID_SYSTEM;
  p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.write_perm);

  p_input_report = &input_report_array[COMPOSITE_REPORT_INDEX_CONSUMER];
  p_input_report->max_len = 2;
  p_input_report->rep_ref.report_id = COMPOSITE_REPORT_ID_CONSUMER;
  p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.write_perm);

  p_input_report = &input_report_array[COMPOSITE_REPORT_INDEX_ABS_MOUSE];
  p_input_report->max_len = 2;
  p_input_report->rep_ref.report_id = COMPOSITE_REPORT_ID_ABS_MOUSE;
  p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_input_report->security_mode.write_perm);

  p_output_report = &output_report_array[OUTPUT_REPORT_INDEX];
  p_output_report->max_len = OUTPUT_REPORT_MAX_LEN;
  p_output_report->rep_ref.report_id = OUTPUT_REP_REF_ID;
  p_output_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_OUTPUT;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_output_report->security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &p_output_report->security_mode.write_perm);

  hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK
      | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

  memset(&composite_service_init_obj, 0, sizeof(composite_service_init_obj));

  composite_service_init_obj.evt_handler = on_hids_evt;
  composite_service_init_obj.error_handler = service_error_handler;
  composite_service_init_obj.is_kb = true;
  composite_service_init_obj.is_mouse = true;
  composite_service_init_obj.inp_rep_count = COMPOSITE_REPORT_COUNT;
  composite_service_init_obj.p_inp_rep_array = input_report_array;
  composite_service_init_obj.outp_rep_count = 1;
  composite_service_init_obj.p_outp_rep_array = output_report_array;
  composite_service_init_obj.feature_rep_count = 0;
  composite_service_init_obj.p_feature_rep_array = NULL;
  composite_service_init_obj.rep_map.data_len = sizeof(CompositeReportMap);
  composite_service_init_obj.rep_map.p_data = (uint8_t*) CompositeReportMap;
  composite_service_init_obj.hid_information.bcd_hid =
      BASE_USB_HID_SPEC_VERSION;
  composite_service_init_obj.hid_information.b_country_code = 0;
  composite_service_init_obj.hid_information.flags = hid_info_flags;
  composite_service_init_obj.included_services_count = 0;
  composite_service_init_obj.p_included_services_array = NULL;

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.rep_map.security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(
      &composite_service_init_obj.rep_map.security_mode.write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.hid_information.security_mode.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(
      &composite_service_init_obj.hid_information.security_mode.write_perm);

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_boot_kb_inp_rep.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_boot_kb_inp_rep.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(
      &composite_service_init_obj.security_mode_boot_kb_inp_rep.write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_boot_kb_outp_rep.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_boot_kb_outp_rep.write_perm);

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_boot_mouse_inp_rep.cccd_write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_boot_mouse_inp_rep.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(
      &composite_service_init_obj.security_mode_boot_mouse_inp_rep.write_perm);

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_protocol.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_protocol.write_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(
      &composite_service_init_obj.security_mode_ctrl_point.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
      &composite_service_init_obj.security_mode_ctrl_point.write_perm);

  err_code = ble_hids_init(&m_hids_composite, &composite_service_init_obj);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
void services_init(void) {
  dis_init();
  bas_init();
  composite_service_init(); // Android can recognize only one servce
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
void conn_params_error_handler(uint32_t nrf_error) {
  APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void) {
  uint32_t err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail = false;
  cp_init.evt_handler = NULL;
  cp_init.error_handler = conn_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting timers.
 */
void timers_start(void) {
  uint32_t err_code;

  err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL,
      NULL);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the HID Report Characteristic Write event.
 *
 * @param[in]   p_evt   HID service event.
 */
void on_hid_rep_char_write(ble_hids_evt_t * p_evt) {
  if (p_evt->params.char_write.char_id.rep_type == BLE_HIDS_REP_TYPE_OUTPUT) {
    uint32_t err_code;
    uint8_t report_val;
    uint8_t report_index = p_evt->params.char_write.char_id.rep_index;

    if (report_index == OUTPUT_REPORT_INDEX) {
      // This code assumes that the outptu report is one byte long. Hence the following
      // static assert is made.
      STATIC_ASSERT(OUTPUT_REPORT_MAX_LEN == 1);

      err_code = ble_hids_outp_rep_get(&m_hids_composite, report_index,
      OUTPUT_REPORT_MAX_LEN, 0, &report_val);
      APP_ERROR_CHECK(err_code);

      if (!m_caps_on
          && ((report_val & OUTPUT_REPORT_BIT_MASK_CAPS_LOCK) != 0)) {
        // Caps Lock is turned On.
        NRF_LOG_INFO("Caps Lock is turned On!\r\n");
        m_caps_on = true;
      } else if (m_caps_on
          && ((report_val & OUTPUT_REPORT_BIT_MASK_CAPS_LOCK) == 0)) {
        // Caps Lock is turned Off .
        NRF_LOG_INFO("Caps Lock is turned Off!\r\n");
        m_caps_on = false;
      } else {
        // The report received is not supported by this application. Do nothing.
      }
    }
  }
}

/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt) {
  switch (p_evt->evt_type) {
  case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
    m_in_boot_mode = true;
    break;

  case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
    m_in_boot_mode = false;
    break;

  case BLE_HIDS_EVT_REP_CHAR_WRITE:
    on_hid_rep_char_write(p_evt);
    break;

  case BLE_HIDS_EVT_NOTIF_ENABLED:
    break;

  default:
    // No implementation needed.
    break;
  }
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
void on_adv_evt(ble_adv_evt_t ble_adv_evt) {
  uint32_t err_code;

  switch (ble_adv_evt) {
  case BLE_ADV_EVT_DIRECTED:
    NRF_LOG_INFO("BLE_ADV_EVT_DIRECTED\r\n");
    break; //BLE_ADV_EVT_DIRECTED

  case BLE_ADV_EVT_FAST:
    NRF_LOG_INFO("BLE_ADV_EVT_FAST\r\n");
    break; //BLE_ADV_EVT_FAST

  case BLE_ADV_EVT_SLOW:
    NRF_LOG_INFO("BLE_ADV_EVT_SLOW\r\n");
    break; //BLE_ADV_EVT_SLOW

  case BLE_ADV_EVT_FAST_WHITELIST:
    NRF_LOG_INFO("BLE_ADV_EVT_FAST_WHITELIST\r\n");
    break; //BLE_ADV_EVT_FAST_WHITELIST

  case BLE_ADV_EVT_SLOW_WHITELIST:
    NRF_LOG_INFO("BLE_ADV_EVT_SLOW_WHITELIST\r\n");
    break; //BLE_ADV_EVT_SLOW_WHITELIST

  case BLE_ADV_EVT_IDLE:
    sleep_mode_enter();
    break; //BLE_ADV_EVT_IDLE

  case BLE_ADV_EVT_WHITELIST_REQUEST: {
    ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    ble_gap_irk_t whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    uint32_t addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
    uint32_t irk_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt, whitelist_irks,
        &irk_cnt);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEBUG(
        "pm_whitelist_get returns %d addr in whitelist and %d irk whitelist\r\n",
        addr_cnt, irk_cnt);

    // Apply the whitelist.
    err_code = ble_advertising_whitelist_reply(whitelist_addrs, addr_cnt,
        whitelist_irks, irk_cnt);
    APP_ERROR_CHECK(err_code);
  }
    break; //BLE_ADV_EVT_WHITELIST_REQUEST

  case BLE_ADV_EVT_PEER_ADDR_REQUEST: {
    pm_peer_data_bonding_t peer_bonding_data;

    // Only Give peer address if we have a handle to the bonded peer.
    if (m_peer_id != PM_PEER_ID_INVALID) {
      err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
      if (err_code != NRF_ERROR_NOT_FOUND) {
        APP_ERROR_CHECK(err_code);

        ble_gap_addr_t * p_peer_addr =
            &(peer_bonding_data.peer_ble_id.id_addr_info);
        err_code = ble_advertising_peer_addr_reply(p_peer_addr);
        APP_ERROR_CHECK(err_code);
      }
    }
  }
    break; //BLE_ADV_EVT_PEER_ADDR_REQUEST

  default:
    break;
  }
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
void on_ble_evt(ble_evt_t * p_ble_evt) {
  uint32_t err_code;

  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_CONNECTED:
    NRF_LOG_INFO("Connected\r\n");
    m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_adv_modes_config_t options;
    options.ble_adv_whitelist_enabled = true;
    options.ble_adv_directed_enabled = true;
    options.ble_adv_directed_slow_enabled = false;
    options.ble_adv_directed_slow_interval = 0;
    options.ble_adv_directed_slow_timeout = 0;
    options.ble_adv_fast_enabled = true;
    options.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    options.ble_adv_fast_timeout = APP_ADV_FAST_TIMEOUT;
    options.ble_adv_slow_enabled = true;
    options.ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
    options.ble_adv_slow_timeout = APP_ADV_SLOW_TIMEOUT;
    ble_advertising_modes_config_set(&options);

    break; // BLE_GAP_EVT_CONNECTED

  case BLE_EVT_TX_COMPLETE:
    NRF_LOG_INFO("BLE_EVT_TX_COMPLETE\r\n");
    break; // BLE_EVT_TX_COMPLETE

  case BLE_GAP_EVT_DISCONNECTED:
    NRF_LOG_INFO("Disconnected\r\n");

    m_conn_handle = BLE_CONN_HANDLE_INVALID;

    // Reset m_caps_on variable. Upon reconnect, the HID host will re-send the Output
    // report containing the Caps lock state.
    m_caps_on = false;
    // disabling alert 3. signal - used for capslock ON

//    if (m_is_wl_changed) {
//      // The whitelist has been modified, update it in the Peer Manager.
//      err_code = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
//      APP_ERROR_CHECK(err_code);
//
//      err_code = pm_device_identities_list_set(m_whitelist_peers,
//          m_whitelist_peer_cnt);
//      if (err_code != NRF_ERROR_NOT_SUPPORTED) {
//        APP_ERROR_CHECK(err_code);
//      }
//
//      m_is_wl_changed = false;
//    }
    break; // BLE_GAP_EVT_DISCONNECTED

  case BLE_GATTC_EVT_TIMEOUT:
    // Disconnect on GATT Client timeout event.
    NRF_LOG_DEBUG("GATT Client Timeout.\r\n");
    err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    break; // BLE_GATTC_EVT_TIMEOUT

  case BLE_GATTS_EVT_TIMEOUT:
    // Disconnect on GATT Server timeout event.
    NRF_LOG_DEBUG("GATT Server Timeout.\r\n");
    err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    break; // BLE_GATTS_EVT_TIMEOUT

  case BLE_EVT_USER_MEM_REQUEST:
    err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
    APP_ERROR_CHECK(err_code);
    break; // BLE_EVT_USER_MEM_REQUEST

  case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: {
    ble_gatts_evt_rw_authorize_request_t req;
    ble_gatts_rw_authorize_reply_params_t auth_reply;

    req = p_ble_evt->evt.gatts_evt.params.authorize_request;

    if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID) {
      if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)
          || (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW)
          || (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)) {
        if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
          auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
        } else {
          auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
        }
        auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
        err_code = sd_ble_gatts_rw_authorize_reply(
            p_ble_evt->evt.gatts_evt.conn_handle, &auth_reply);
        APP_ERROR_CHECK(err_code);
      }
    }
  }
    break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

#if (NRF_SD_BLE_API_VERSION == 3)
    case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
    err_code = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle,
        NRF_BLE_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
    break; // BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST
#endif

  default:
    // No implementation needed.
    break;
  }
}

/**@brief   Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
void ble_evt_dispatch(ble_evt_t * p_ble_evt) {
  uint16_t conn_handle;
  uint16_t role;

  ble_conn_state_on_ble_evt(p_ble_evt);
  pm_on_ble_evt(p_ble_evt);

  // The connection handle should really be retrievable for any event type.
  conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
  role = ble_conn_state_role(conn_handle);

  /** The Connection state module has to be fed BLE events in order to function correctly
   * Remember to call ble_conn_state_on_ble_evt before calling any ble_conns_state_* functions. */
  if (role == BLE_GAP_ROLE_PERIPH) {
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_hids_on_ble_evt(&m_hids_composite, p_ble_evt);
    ble_bas_on_ble_evt(&m_bas, p_ble_evt);
  } else if ((role == BLE_GAP_ROLE_CENTRAL)
      || (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_REPORT)) {
    /** on_ble_central_evt will update the connection handles, so we want to execute it
     * after dispatching to the central applications upon disconnection. */
#ifdef NRF_SEPARATE_KEYBOARD_MASTER
    on_ble_central_evt(p_ble_evt, conn_handle);
#endif
  }
}

/**@brief   Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
void sys_evt_dispatch(uint32_t sys_evt) {
  // Dispatch the system event to the fstorage module, where it will be
  // dispatched to the Flash Data Storage (FDS) module.
  fs_sys_event_handler(sys_evt);

  // Dispatch to the Advertising module last, since it will check if there are any
  // pending flash operations in fstorage. Let fstorage process system events first,
  // so that it can report correctly to the Advertising module.
  ble_advertising_on_sys_evt(sys_evt);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void) {
  uint32_t err_code;

  nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

  // Initialize the SoftDevice handler module.
  SOFTDEVICE_HANDLER_APPSH_INIT(&clock_lf_cfg, true);

  ble_enable_params_t ble_enable_params;
  err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
  PERIPHERAL_LINK_COUNT, &ble_enable_params);
  APP_ERROR_CHECK(err_code);

  // Check the ram settings against the used number of links
  CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

  // Enable BLE stack.
#if (NRF_SD_BLE_API_VERSION == 3)
  ble_enable_params.gatt_enable_params.att_mtu = NRF_BLE_MAX_MTU_SIZE;
#endif
  err_code = softdevice_enable(&ble_enable_params);
  APP_ERROR_CHECK(err_code);

  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  APP_ERROR_CHECK(err_code);

  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Peer Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Peer Manager.
 */
void peer_manager_init() {
  ble_gap_sec_params_t sec_param;
  ret_code_t err_code;

  err_code = pm_init();
  APP_ERROR_CHECK(err_code);

//  if (erase_bonds) {
//    err_code = pm_peers_delete();
//    APP_ERROR_CHECK(err_code);
//  }

  memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

  // Security parameters to be used for all security procedures.
  sec_param.bond = SEC_PARAM_BOND;
  sec_param.mitm = SEC_PARAM_MITM;
  sec_param.lesc = SEC_PARAM_LESC;
  sec_param.keypress = SEC_PARAM_KEYPRESS;
  sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;
  sec_param.oob = SEC_PARAM_OOB;
  sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
  sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
  sec_param.kdist_own.enc = 1;
  sec_param.kdist_own.id = 1;
  sec_param.kdist_peer.enc = 1;
  sec_param.kdist_peer.id = 1;

  err_code = pm_sec_params_set(&sec_param);
  APP_ERROR_CHECK(err_code);

  err_code = pm_register(pm_evt_handler);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
void advertising_init(void) {
  uint32_t err_code;
  uint8_t adv_flags;
  ble_advdata_t advdata;
  ble_adv_modes_config_t options;

  // Build and set advertising data
  memset(&advdata, 0, sizeof(advdata));

  adv_flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  advdata.name_type = BLE_ADVDATA_FULL_NAME;
  advdata.include_appearance = true;
  advdata.flags = adv_flags;
  advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids)
      / sizeof(m_adv_uuids[0]);
  advdata.uuids_complete.p_uuids = m_adv_uuids;

  memset(&options, 0, sizeof(options));
  options.ble_adv_whitelist_enabled = true;
  options.ble_adv_directed_enabled = true;
  options.ble_adv_directed_slow_enabled = false;
  options.ble_adv_directed_slow_interval = 0;
  options.ble_adv_directed_slow_timeout = 0;
  options.ble_adv_fast_enabled = true;
  options.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
  options.ble_adv_fast_timeout = APP_ADV_FAST_TIMEOUT;
  options.ble_adv_slow_enabled = true;
  options.ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
  options.ble_adv_slow_timeout = APP_ADV_SLOW_TIMEOUT;

  err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt,
      ble_advertising_error_handler);
  APP_ERROR_CHECK(err_code);
}


/* LED status */
uint8_t keyboard_leds(void) {
  return (uint8_t)(keyboard_led_stats & 0xFF);
}

/* prepare and start sending a report IN
 * not callable from ISR or locked state */
void ble_send_keyboard(report_keyboard_t *report) {
  if (m_conn_handle == BLE_CONN_HANDLE_INVALID) {
    return;
  }
  uint32_t err_code;
  if (!m_in_boot_mode) {
#ifdef NKRO_ENABLE
    err_code = ble_hids_inp_rep_send(&m_hids_composite,
        COMPOSITE_REPORT_INDEX_KEYBOARD, NKRO_BYTE_LEN, report->raw);
#else
    err_code = ble_hids_inp_rep_send(&m_hids_composite,
        COMPOSITE_REPORT_INDEX_KEYBOARD, INPUT_REPORT_KEYS_MAX_LEN,
        report->raw);
#endif
    NRF_LOG_INFO("key normal report send\r\n");
  } else {
    err_code = ble_hids_boot_kb_inp_rep_send(&m_hids_composite,
    INPUT_REPORT_KEYS_MAX_LEN, report->raw);
    NRF_LOG_INFO("key boot report send\r\n");
  }
//    APP_ERROR_CHECK(err_code);
  if (err_code != NRF_SUCCESS) {
    NRF_LOG_INFO("send key error\r\n");
  } else {
    keyboard_report_sent = *report;
  }
}

/* ---------------------------------------------------------
 *                     Mouse functions
 * ---------------------------------------------------------
 */

void ble_send_abs_mouse(uint8_t x, uint8_t y) {
  uint8_t data[2];
  data[0] = (uint8_t) x;
  data[1] = (uint8_t) y;
  if (m_conn_handle == BLE_CONN_HANDLE_INVALID) {
    return;
  }
  uint32_t err_code = ble_hids_inp_rep_send(&m_hids_composite,
      COMPOSITE_REPORT_INDEX_ABS_MOUSE, 2, (uint8_t*) data);
  NRF_LOG_INFO("abs mouse report send\r\n");
  if (err_code != NRF_SUCCESS) {
    NRF_LOG_INFO("abs mouse send error\r\n");
  }
}

#ifdef MOUSE_ENABLE

void ble_send_mouse(report_mouse_t *report) {
  if (m_conn_handle == BLE_CONN_HANDLE_INVALID) {
    return;
  }

  uint32_t err_code;
  if (!m_in_boot_mode) {
    err_code = ble_hids_inp_rep_send(&m_hids_composite,
        COMPOSITE_REPORT_INDEX_MOUSE, 5, (uint8_t*)report);
    NRF_LOG_INFO("mouse normal report send\r\n");
  } else {
    err_code = ble_hids_boot_mouse_inp_rep_send(&m_hids_composite,
        report->buttons, report->x, report->y, 0, NULL);
    NRF_LOG_INFO("mouse boot report send\r\n");
  }
//    APP_ERROR_CHECK(err_code);
  if (err_code != NRF_SUCCESS) {
    NRF_LOG_INFO("mouse send error\r\n");
  }
}

#else /* MOUSE_ENABLE */
void ble_send_mouse(report_mouse_t *report) {
  (void) report;
}
#endif /* MOUSE_ENABLE */

/* ---------------------------------------------------------
 *                   Extrakey functions
 * ---------------------------------------------------------
 */

#ifdef EXTRAKEY_ENABLE
static void ble_send_extra_report(uint8_t report_id, uint16_t data) {
  if (m_conn_handle == BLE_CONN_HANDLE_INVALID) {
    return;
  }
  uint32_t err_code = ble_hids_inp_rep_send(&m_hids_composite,
      report_id, 2, (uint8_t*)&data);
  if (err_code != NRF_SUCCESS) {
    NRF_LOG_INFO("extra send error\r\n");
  }
}

void ble_send_system(uint16_t data) {
  ble_send_extra_report(COMPOSITE_REPORT_INDEX_SYSTEM, data);
}

void ble_send_consumer(uint16_t data) {
  ble_send_extra_report(COMPOSITE_REPORT_INDEX_CONSUMER, data);
}

#else /* EXTRAKEY_ENABLE */
void ble_send_system(uint16_t data) {
  (void) data;
}
void ble_send_consumer(uint16_t data) {
  (void) data;
}
#endif /* EXTRAKEY_ENABLE */

/* ---------------------------------------------------------
 *                   Console functions
 * ---------------------------------------------------------
 */

#ifdef CONSOLE_ENABLE

int8_t sendchar(uint8_t c) {
  // The previous implmentation had timeouts, but I think it's better to just slow down
  // and make sure that everything is transferred, rather than dropping stuff
//  return chnWrite(&drivers.console_driver.driver, &c, 1);
  return 0;
}

// Just a dummy function for now, this could be exposed as a weak function
// Or connected to the actual QMK console
//static void console_receive( uint8_t *data, uint8_t length ) {
//  (void)data;
//  (void)length;
//}

void console_task(void) {
//  uint8_t buffer[CONSOLE_EPSIZE];
//  size_t size = 0;
//  do {
//    size_t size = chnReadTimeout(&drivers.console_driver.driver, buffer, sizeof(buffer), TIME_IMMEDIATE);
//    if (size > 0) {
//        console_receive(buffer, size);
//    }
//  } while(size > 0);
}

#else /* CONSOLE_ENABLE */
int8_t sendchar(uint8_t c) {
  (void) c;
  return 0;
}
#endif /* CONSOLE_ENABLE */

void sendchar_pf(void *p, char c) {
  (void) p;
  sendchar((uint8_t) c);
}

void restart_advertising_wo_whitelist() {
  uint32_t err_code;

  sd_ble_gap_adv_stop();

#ifdef NRF_SEPARATE_KEYBOARD_MASTER
  scan_start();
#endif

    ble_adv_modes_config_t options;
    options.ble_adv_whitelist_enabled = false;
    options.ble_adv_directed_enabled = false;
    options.ble_adv_directed_slow_enabled = false;
    options.ble_adv_directed_slow_interval = 0;
    options.ble_adv_directed_slow_timeout = 0;
    options.ble_adv_fast_enabled = true;
    options.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    options.ble_adv_fast_timeout = APP_ADV_FAST_TIMEOUT;
    options.ble_adv_slow_enabled = true;
    options.ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
    options.ble_adv_slow_timeout = APP_ADV_SLOW_TIMEOUT;
    ble_advertising_modes_config_set(&options);

    memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
    m_whitelist_peer_cnt = 0;
    m_is_wl_changed = true;

  if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
    err_code = sd_ble_gap_disconnect(m_conn_handle,
    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_ERROR_INVALID_STATE) {
      APP_ERROR_CHECK(err_code);
    }
  } else {
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
  }
  if (err_code != NRF_ERROR_INVALID_STATE) {
    APP_ERROR_CHECK(err_code);
  }
}

void restart_advertising_id(uint8_t id) {
  ret_code_t ret;

  if (m_conn_handle != BLE_CONN_HANDLE_INVALID) {
    ble_adv_modes_config_t options;
    options.ble_adv_whitelist_enabled = true;
    options.ble_adv_directed_enabled = false;
    options.ble_adv_directed_slow_enabled = false;
    options.ble_adv_directed_slow_interval = 0;
    options.ble_adv_directed_slow_timeout = 0;
    options.ble_adv_fast_enabled = true;
    options.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    options.ble_adv_fast_timeout = APP_ADV_FAST_TIMEOUT;
    options.ble_adv_slow_enabled = true;
    options.ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
    options.ble_adv_slow_timeout = APP_ADV_SLOW_TIMEOUT;
    ble_advertising_modes_config_set(&options);

    ret = sd_ble_gap_disconnect(m_conn_handle,
        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (ret != NRF_ERROR_INVALID_STATE) {
      APP_ERROR_CHECK(ret);
    }
  }
  sd_ble_gap_adv_stop();

//#ifdef NRF_SEPARATE_KEYBOARD_MASTER
//  sd_ble_gap_scan_stop();
//#endif

  memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
  m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

  peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);
  if (id >= m_whitelist_peer_cnt) {
    return;
  }
  m_whitelist_peer_cnt = 1;
  m_whitelist_peers[0] = m_whitelist_peers[id];

  ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
  APP_ERROR_CHECK(ret);

  ret = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
  if (ret != NRF_ERROR_NOT_SUPPORTED) {
    APP_ERROR_CHECK(ret);
  }

//#ifdef NRF_SEPARATE_KEYBOARD_MASTER
//  sd_ble_gap_scan_start();
//#endif

  ret = ble_advertising_start(BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(ret);
}

void main_task_start(uint8_t interval) {
  uint32_t err_code = app_timer_start(main_task_timer_id,
      APP_TIMER_TICKS(interval, 0), NULL);
  APP_ERROR_CHECK(err_code);
}

static bool enable_ble_send = true;
static bool enable_usb_send = false;
bool get_ble_enabled () { return enable_ble_send; }
void set_ble_enabled (bool enabled) { enable_ble_send = enabled; }
bool get_usb_enabled () { return enable_usb_send; }
void set_usb_enabled (bool enabled) { enable_usb_send = false; }
