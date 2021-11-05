#include "ifb.h"


/* Transmit packet to host
 * Not re-entrant */
uint8_t
usb_packet_IN(uint8_t *buffer, uint16_t len)
{
    static uint16_t BytesProcessed;
    uint8_t ret;
    BytesProcessed = 0;

    Endpoint_SelectEndpoint(BULK_IN_EPADDR);

    do {
        ret = Endpoint_Write_Stream_LE(
                buffer,
                len,
                &BytesProcessed);
    } while (ret == ENDPOINT_RWSTREAM_IncompleteTransfer);

    if (ret == ENDPOINT_RWSTREAM_NoError)
        Endpoint_ClearIN();
    else
        Endpoint_StallTransaction();

    return ret;
}

/* Receive packet from host
 * Not re-entrant */
uint8_t
usb_packet_OUT(uint8_t *buffer, uint16_t len)
{
    //static uint16_t BytesProcessed;
    uint8_t ret;
    bool fullPacket;

    /* TODO: handle oversized transfers? */
    //BytesProcessed = 0;
    Endpoint_SelectEndpoint(BULK_OUT_EPADDR);

    do {
        ret = Endpoint_WaitUntilReady();
        if (ret != ENDPOINT_READYWAIT_NoError)
            break;
        /* Transfer ends with a ZLP
         * Has not been confirmed whether or not the driver
         * will produce zero-length packets */
        fullPacket = BULK_EPSIZE == Endpoint_BytesInEndpoint();

        while(Endpoint_IsReadWriteAllowed())
            *buffer++ = Endpoint_Read_8();

        Endpoint_ClearOUT();
    } while(fullPacket);

    if (ret != ENDPOINT_READYWAIT_NoError)
        Endpoint_StallTransaction();

    return ret;
}
