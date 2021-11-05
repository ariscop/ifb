#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>

#include "Descriptors.h"

#include <LUFA/Drivers/USB/USB.h>

void SetupHardware(void);

typedef enum {
    IDLE,
    RX_WAIT,
    RX_DATA,
    RX_ACK,
    RX_DONE,
    TX_WAIT,
    TX_START,
    TX_DATA,
    MODE_SWITCH,
} SERIAL_MODE;

extern _Atomic SERIAL_MODE state;

void serial_enable(bool mode_32bit);
void serial_disable(void);

void Serial_task(void);

uint8_t usb_packet_IN(uint8_t *buffer, uint16_t len);
uint8_t usb_packet_OUT(uint8_t *buffer, uint16_t len);
