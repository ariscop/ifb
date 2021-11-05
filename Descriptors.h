
#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#include <avr/pgmspace.h>

#include <LUFA/Drivers/USB/USB.h>

#define BULK_IN_EPADDR               (ENDPOINT_DIR_IN  | 1)
#define BULK_OUT_EPADDR              (ENDPOINT_DIR_OUT | 2)
#define INTERRUPT_EPADDR             (ENDPOINT_DIR_IN  | 3)
#define BULK_EPSIZE                  64

typedef struct
{
    USB_StdDescriptor_Configuration_Header_t Config;
    USB_StdDescriptor_Interface_t            Interface;
    USB_StdDescriptor_Endpoint_t             BulkIn;
    USB_StdDescriptor_Endpoint_t             BulkOut;
    USB_StdDescriptor_Endpoint_t             InterruptIn;
} USB_Descriptor_Configuration_t;


/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif

