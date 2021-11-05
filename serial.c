#include "ifb.h"

_Atomic SERIAL_MODE state;

/* This horrible mess does work
 * Might be cleaned up at some point */

/* Next mode */
bool enable_sio32;

/* Current mode */
bool sio32_mode;

uint16_t packet_len;
uint16_t current;

uint8_t packet_buffer[512];

uint16_t checksum;

void Serial_task(void)
{
    switch(state)
    {
    case RX_DONE:
        if(usb_packet_IN(packet_buffer, packet_len + 4) != 0) {
            /* Some sort of error */
            state = IDLE;
            break;
        };
        current = 0;
        state = TX_WAIT;
        break;

    case TX_WAIT:
        memset(packet_buffer, 0, sizeof(packet_buffer));
        if(usb_packet_OUT(packet_buffer, sizeof(packet_buffer)) != 0) {
            /* Some sort of error */
            state = IDLE;
            break;
        };
        current = 0;
        state = TX_START;
        break;

    case MODE_SWITCH:
        serial_disable();
        serial_enable(enable_sio32);
        break;

    default:
        break;
    }
}

static uint8_t sio32_rx[4];
static uint8_t sio32_tx[4];

void serial_enable(bool mode_32bit)
{
    SPCR = _BV(SPE) | _BV(CPOL) | _BV(CPHA);
    SPSR = 0;
    SPDR = 0xD2;

    state = RX_WAIT;
    current = 0;
    packet_len = 0;
    sio32_mode = mode_32bit;

    if (mode_32bit) {
      memset(sio32_tx, 0xd2, sizeof(sio32_tx));

      PCIFR = _BV(PCIF0);
      PCMSK0 = _BV(PCINT1);
      PCICR = _BV(PCIE0);
    } else {
      SPCR |= _BV(SPIE);
    }
}

void serial_disable(void)
{
    /* Disable the active interupt first
     * This should make the action atomic */
    if (sio32_mode) {
        PCICR = 0;
        SPCR = 0;
    } else {
        SPCR = 0;
        PCICR = 0;
    }
    state = IDLE;
    SPSR = 0;
    /* for 32bit mode */
    PCMSK0 = 0;
    PCIFR = _BV(PCIF0);
}

/* SIO8:
 *
 *  0x99 0x66 cmd 0x00 len len (buffer[len]) csum csum ack ack
 *  | -------------- | | ----------------- | | ----- | | --- |
 *  Header             Data                  Checksum  Ack
 *            | -------------------------- |
 *            Checksum Coverage
 *
 * SIO32:
 *                     / Must be a multiple of 4 bytes         \
 *  0x99 0x66 cmd 0x00|len len (buffer[len]) pad[0-2] csum csum|ack ack 0x00 0x00
 *  | -------------- | | ---------------------------| | ----- | | ------------- |
 *  Header             Data                           Checksum  Ack
 *            | ----------------------------------- |
 *            Checksum Coverage
 *            Data is padded to a multiple of 4 bytes, so the checksum
 *            always occupies the second half of a word
 *
 */

uint8_t sio8_transfer(uint8_t c) {
    switch(state) {
    case RX_WAIT:
        if (current == 1 && c == 0x66) {
            state = RX_DATA;
            packet_buffer[current++] = c;
            packet_len = 6; // header length
            checksum = 0;
            break;
        } else {
            current = 0;
        }
        if (current == 0 && c == 0x99) {
            packet_buffer[current++] = c;
        }
        break;

    case RX_DATA:
        packet_buffer[current++] = c;
        if (current == 6)
            packet_len =
                 (packet_buffer[4] << 8)
                + packet_buffer[5] + 6;
        if (current <= packet_len)
            checksum += c;
        if (current == packet_len + 1)
            state = RX_ACK;
        break;

    case RX_ACK:
        packet_buffer[current++] = c;
        /* first ack byte */
        if (current == packet_len + 2)
            return 0x88;
        /* second ack byte */
        if (current == packet_len + 3) {
            if (checksum == (packet_buffer[packet_len] << 8)
                           + packet_buffer[packet_len + 1]) {

                if (packet_buffer[2] == 0x18)
                    enable_sio32 = packet_buffer[6] == 1;

                return packet_buffer[2] ^ 0x80;
            } else {
                /* Checksum error, retry */
                state = RX_WAIT;
                return 0xf1;
            }
        }

        current = 0;
        state = RX_DONE;
        break;

    case TX_START:
        packet_len = (packet_buffer[4] << 8) + packet_buffer[5];
        /* Include header */
        packet_len += 6;
        state = TX_DATA;

        /* Fallthrough */
    case TX_DATA:
        if (current < packet_len + 4)
            return packet_buffer[current++];

        if (c == 0xF0 || c == 0xF1 || c == 0xF2)
        {
            // retry
            current = 0;
            break;
        } else {
            if (packet_buffer[2] == 0x98) {
                state = MODE_SWITCH;
                break;
            } else if (packet_buffer[2] == 0x96 || packet_buffer[2] == 0x91) {
                enable_sio32 = false;
                state = MODE_SWITCH;
                break;
            }

            current = 0;
            state = RX_WAIT;
            break;
        }

    default:
        break;
    }

    return 0xD2;
}

void sio32_transfer(uint8_t *out, uint8_t *in)
{
    switch(state) {
    case RX_WAIT:
        if (in[0] == 0x99 && in[1] == 0x66) {
            for (int b = 0; b < 4; b++)
                packet_buffer[b] = in[b];
            current = 4;

            checksum = 0;
            checksum += in[2];
            checksum += in[3];
            state = RX_DATA;
        }
        break;

    case RX_DATA:
        if (current == 4) {
            packet_len = (in[0] << 8) + in[1];
            /* pad to 4 bytes */
            packet_len = (packet_len + 3) & ~0x3;
            /* Include header */
            packet_len += 6;
        }

        for (int b = 0; b < 4; b++)
            packet_buffer[current + b] = in[b];

        current += 4;
        checksum += in[0];
        checksum += in[1];

        if (current <= packet_len) {
            checksum += in[2];
            checksum += in[3];
        } else {
            out[0] = 0x88;
            if (checksum == (in[2] << 8) + in[3]) {
                state = RX_ACK;
                out[1] = packet_buffer[2] | 0x80;
            } else {
                state = RX_WAIT;
                out[1] = 0xf1;
            }
            //out[2] = out[3] = 0;
            out[2] = (checksum >> 8) & 0xFF;
            out[3] = (checksum >> 0) & 0xFF;
            return;
        }
        break;

    case RX_ACK:
        for (int b = 0; b < 4; b++)
            packet_buffer[current + b] = in[b];

        current = 0;
        packet_len += 2;
        state = RX_DONE;
        break;

    case TX_START:
        packet_len = (packet_buffer[4] << 8) + packet_buffer[5];
        /* pad to 4 bytes */
        packet_len = (packet_len + 3) & ~0x3;
        /* Include header */
        packet_len += 6;
        state = TX_DATA;

        /* Fallthrough */
   case TX_DATA:
        if (current < packet_len + 4) {
            memcpy(out, &packet_buffer[current], 4);
            current += 4;
            return;
        } else if (in[1] == 0xF0 || in[1] == 0xF1 || in[1] == 0xF2) {
            /* retry */
            current = 0;
            break;
        } else {
            if (packet_buffer[2] == 0x98) {
                state = MODE_SWITCH;
                break;
            } else if (packet_buffer[2] == 0x96 || packet_buffer[2] == 0x91) {
                /* reset */
                enable_sio32 = false;
                state = MODE_SWITCH;
                break;
            }

            state = RX_WAIT;
        }
        break;

    default:
        break;
    }

    memset(out, 0xd2, 4);
}


ISR (PCINT0_vect)
{

    loop_until_bit_is_set(SPSR, SPIF);
    sio32_rx[0] = SPDR;
    SPDR = sio32_tx[1];

    loop_until_bit_is_set(SPSR, SPIF);
    sio32_rx[1] = SPDR;
    SPDR = sio32_tx[2];

    loop_until_bit_is_set(SPSR, SPIF);
    sio32_rx[2] = SPDR;
    SPDR = sio32_tx[3];

    loop_until_bit_is_set(SPSR, SPIF);
    sio32_rx[3] = SPDR;

    sio32_transfer(sio32_tx, sio32_rx);

    SPDR = sio32_tx[0];

    PCIFR = _BV(PCIF0);
}

ISR (SPI_STC_vect)
{
    SPDR = sio8_transfer(SPDR);
}
