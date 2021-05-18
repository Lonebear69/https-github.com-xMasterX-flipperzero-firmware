#pragma once

#include <rfal_nfc.h>
#include <st_errno.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init nfc
 */
ReturnCode api_hal_nfc_init();

/**
 * Check if nfc worker is busy
 */
bool api_hal_nfc_is_busy();

/**
 * NFC field on
 */
void api_hal_nfc_field_on();

/**
 * NFC field off
 */
void api_hal_nfc_field_off();

/**
 * NFC start sleep
 */
void api_hal_nfc_start_sleep();

/**
 * NFC stop sleep
 */
void api_hal_nfc_exit_sleep();

/**
 * NFC poll
 */
bool api_hal_nfc_detect(rfalNfcDevice** dev_list, uint8_t* dev_cnt, uint32_t cycles, bool deactivate);

/**
 * NFC data exchange
 */
ReturnCode api_hal_nfc_data_exchange(rfalNfcDevice* dev, uint8_t* tx_buff, uint16_t tx_len, uint8_t** rx_buff, uint16_t** rx_len, bool deactivate);

/**
 * NFC deactivate and start sleep
 */
void api_hal_nfc_deactivate();

#ifdef __cplusplus
}
#endif
