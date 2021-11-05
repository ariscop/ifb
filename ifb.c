#include "ifb.h"

int main(void) {
    SetupHardware();
    GlobalInterruptEnable();

    for(;;) {
        Serial_task();
    };
}

void SetupHardware(void)
{
    /* spi pins */
    DDRB &= ~_BV(DDB0); //SS
    DDRB &= ~_BV(DDB1); //SCK
    DDRB &= ~_BV(DDB2); //MOSI
    DDRB |= _BV(DDB3); //MISO

    /* Pullups on sck and mosi */
    PORTB |= _BV(PORTB1) | _BV(PORTB2);

    PORTD |= _BV(PORTD5); // connected indicator
    DDRD |= _BV(DDD5);

    USB_Init();
}

void EVENT_USB_Device_Reset(void)
{
    serial_disable();
}

void EVENT_USB_Device_Connect(void)
{
    PORTD &= ~_BV(PORTD5);
    serial_enable(false);
}

void EVENT_USB_Device_Disconnect(void)
{
    PORTD |= _BV(PORTD5);
    serial_disable();
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    if(USB_Device_ConfigurationNumber != 1)
        return;

    Endpoint_ConfigureEndpoint(BULK_IN_EPADDR, EP_TYPE_BULK, BULK_EPSIZE, 2);
    Endpoint_ConfigureEndpoint(BULK_OUT_EPADDR, EP_TYPE_BULK, BULK_EPSIZE, 2);

    // unused
    Endpoint_ConfigureEndpoint(INTERRUPT_EPADDR, EP_TYPE_INTERRUPT, 8, 1);
}

void EVENT_USB_Device_ControlRequest(void)
{
    /* Only implement vendor reqs */
    if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) != REQTYPE_VENDOR)
        return;

    switch (USB_ControlRequest.bRequest)
    {
        case 1:
            /* Version number */
            if (USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) {
                Endpoint_ClearSETUP();
                /* Known firmware is version 2.03 */
                Endpoint_Write_16_LE(0x0203);
                Endpoint_ClearIN();
                Endpoint_ClearStatusStage();
            }
            break;

        case 2:
            /* TODO */
            break;

        case 0x69:
            /* TODO */
            break;

        case 0x6A:
            /* port reset */
            Endpoint_ClearSETUP();
            serial_disable();
            serial_enable(false);
            Endpoint_ClearStatusStage();
            break;
    }
}
