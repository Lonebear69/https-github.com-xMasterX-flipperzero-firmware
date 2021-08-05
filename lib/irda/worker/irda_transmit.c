#include "irda.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <furi.h>
#include <api-hal-irda.h>
#include <api-hal-delay.h>

static uint32_t irda_tx_number_of_transmissions = 0;
static uint32_t irda_tx_raw_timings_index = 0;
static uint32_t irda_tx_raw_timings_number = 0;
static uint32_t irda_tx_raw_start_from_mark = 0;
static bool irda_tx_raw_add_silence = false;

ApiHalIrdaTxGetDataState irda_get_raw_data_callback (void* context, uint32_t* duration, bool* level) {
    furi_assert(duration);
    furi_assert(level);
    furi_assert(context);

    ApiHalIrdaTxGetDataState state = ApiHalIrdaTxGetDataStateOk;
    const uint32_t* timings = context;

    if (irda_tx_raw_add_silence && (irda_tx_raw_timings_index == 0)) {
        irda_tx_raw_add_silence = false;
        *level = false;
        *duration = 180000;     // 180 ms delay between raw packets
    } else {
        *level = irda_tx_raw_start_from_mark ^ (irda_tx_raw_timings_index % 2);
        *duration = timings[irda_tx_raw_timings_index++];
    }

    if (irda_tx_raw_timings_number == irda_tx_raw_timings_index) {
        state = ApiHalIrdaTxGetDataStateLastDone;
    }

    return state;
}

void irda_send_raw_ext(const uint32_t timings[], uint32_t timings_cnt, bool start_from_mark, uint32_t frequency, float duty_cycle) {
    furi_assert(timings);
    furi_assert(timings_cnt > 1);

    irda_tx_raw_start_from_mark = start_from_mark;
    irda_tx_raw_timings_index = 0;
    irda_tx_raw_timings_number = timings_cnt;
    irda_tx_raw_add_silence = start_from_mark;
    api_hal_irda_async_tx_set_data_isr_callback(irda_get_raw_data_callback, (void*) timings);
    api_hal_irda_async_tx_start(frequency, duty_cycle);
    api_hal_irda_async_tx_wait_termination();

    furi_assert(!api_hal_irda_is_busy());
}

void irda_send_raw(const uint32_t timings[], uint32_t timings_cnt, bool start_from_mark) {
    irda_send_raw_ext(timings, timings_cnt, start_from_mark, IRDA_COMMON_CARRIER_FREQUENCY, IRDA_COMMON_DUTY_CYCLE);
}

ApiHalIrdaTxGetDataState irda_get_data_callback (void* context, uint32_t* duration, bool* level) {
    ApiHalIrdaTxGetDataState state = ApiHalIrdaTxGetDataStateError;
    IrdaEncoderHandler* handler = context;
    IrdaStatus status = IrdaStatusError;

    if (irda_tx_number_of_transmissions > 0) {
        status = irda_encode(handler, duration, level);
    }

    if (status == IrdaStatusError) {
        state = ApiHalIrdaTxGetDataStateError;
    } else if (status == IrdaStatusOk) {
        state = ApiHalIrdaTxGetDataStateOk;
    } else if (status == IrdaStatusDone) {
        state = ApiHalIrdaTxGetDataStateDone;
        if (--irda_tx_number_of_transmissions == 0) {
            state = ApiHalIrdaTxGetDataStateLastDone;
        }
    } else {
        furi_assert(0);
        state = ApiHalIrdaTxGetDataStateError;
    }

    return state;
}

void irda_send(const IrdaMessage* message, int times) {
    furi_assert(message);
    furi_assert(times);
    furi_assert(irda_is_protocol_valid(message->protocol));

    IrdaEncoderHandler* handler = irda_alloc_encoder();
    irda_reset_encoder(handler, message);
    irda_tx_number_of_transmissions = times;

    api_hal_irda_async_tx_set_data_isr_callback(irda_get_data_callback, handler);
    api_hal_irda_async_tx_start(IRDA_COMMON_CARRIER_FREQUENCY, IRDA_COMMON_DUTY_CYCLE);
    api_hal_irda_async_tx_wait_termination();

    irda_free_encoder(handler);

    furi_assert(!api_hal_irda_is_busy());
}

