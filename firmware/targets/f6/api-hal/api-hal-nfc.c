#include "api-hal-nfc.h"
#include <st25r3916.h>

static bool dev_is_found = false;

ReturnCode api_hal_nfc_init() {
    // Check if Nfc worker was started
    if(rfalNfcGetState() > RFAL_NFC_STATE_NOTINIT) {
        return ERR_NONE;
    }
    return rfalNfcInitialize();
}

bool api_hal_nfc_is_busy() {
    return rfalNfcGetState() > RFAL_NFC_STATE_IDLE;
}

void api_hal_nfc_field_on() {
    api_hal_nfc_exit_sleep();
    st25r3916TxRxOn();
}

void api_hal_nfc_field_off() {
    st25r3916TxRxOff();
    api_hal_nfc_start_sleep();
}

void api_hal_nfc_start_sleep() {
    rfalLowPowerModeStart();
}

void api_hal_nfc_exit_sleep() {
    rfalLowPowerModeStop();
}

static void api_hal_nfc_change_state_cb(rfalNfcState st) {
    FURI_LOG_D("HAL NFC", "NFC worker state: %d", st);
    if(st >= RFAL_NFC_STATE_POLL_SELECT) {
        dev_is_found = true;
    }
}

bool api_hal_nfc_detect(rfalNfcDevice **dev_list, uint8_t* dev_cnt, uint32_t cycles, bool deactivate) {
    furi_assert(dev_list);
    furi_assert(dev_cnt);

    rfalLowPowerModeStop();
    if(rfalNfcGetState() == RFAL_NFC_STATE_NOTINIT) {
        rfalNfcInitialize();
    }
    rfalNfcDiscoverParam params;
    params.compMode = RFAL_COMPLIANCE_MODE_EMV;
    params.techs2Find = RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B | RFAL_NFC_POLL_TECH_F |
                        RFAL_NFC_POLL_TECH_V | RFAL_NFC_POLL_TECH_AP2P | RFAL_NFC_POLL_TECH_ST25TB;
    params.totalDuration = 1000;
    params.devLimit = 3;
    params.wakeupEnabled = false;
    params.wakeupConfigDefault = true;
    params.nfcfBR = RFAL_BR_212;
    params.ap2pBR = RFAL_BR_424;
    params.maxBR = RFAL_BR_KEEP;
    params.GBLen = RFAL_NFCDEP_GB_MAX_LEN;
    params.notifyCb = api_hal_nfc_change_state_cb;

    dev_is_found = false;
    rfalNfcDiscover(&params);
    while(--cycles) {
        rfalNfcWorker();
        FURI_LOG_D("HAL NFC", "Current state %d", rfalNfcGetState());
        if(dev_is_found) {
            rfalNfcGetDevicesFound(dev_list, dev_cnt);
            FURI_LOG_D("HAL NFC", "Found %d devices", dev_cnt);
            break;
        }
        osDelay(5);
    }
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    if(!cycles) {
        FURI_LOG_D("HAL NFC", "Timeout");
        return false;
    }

    return true;
}

ReturnCode api_hal_nfc_data_exchange(rfalNfcDevice* dev, uint8_t* tx_buff, uint16_t tx_len, uint8_t** rx_buff, uint16_t** rx_len, bool deactivate) {
    furi_assert(dev);
    furi_assert(tx_buff);
    furi_assert(rx_buff);
    furi_assert(rx_len);

    ReturnCode ret;
    rfalNfcDevice* active_dev;
    rfalNfcState state = RFAL_NFC_STATE_ACTIVATED;

    ret = rfalNfcGetActiveDevice(&active_dev);
    if(ret != ERR_NONE) {
        return ret;
    }
    if (active_dev != dev) {
        return ERR_NOTFOUND;
    }
    ret = rfalNfcDataExchangeStart(tx_buff, tx_len, rx_buff, rx_len, 0);
    if(ret != ERR_NONE) {
        return ret;
    }
    FURI_LOG_D("HAL NFC", "Start data exchange");
    while(state != RFAL_NFC_STATE_DATAEXCHANGE_DONE) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        FURI_LOG_D("HAL NFC", "Data exchange status: %d", rfalNfcDataExchangeGetStatus());
        osDelay(10);
    }
    FURI_LOG_D("HAL NFC", "Data exchange complete");
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    return ERR_NONE;
}

void api_hal_nfc_deactivate() {
    rfalNfcDeactivate(false);
    rfalLowPowerModeStart();
}
