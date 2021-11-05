/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  USB Device Descriptors, for library use when in USB device mode. Descriptors are special
 *  computer-readable structures which the host requests upon device enumeration, to determine
 *  the device's capabilities and functions.
 */

#include "Descriptors.h"

enum StringDescriptors_t
{
	STRING_ID_Language     = 0, /**< Supported Languages string descriptor ID (must be zero) */
	STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
	STRING_ID_Product      = 2, /**< Product string ID */
};


/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
const USB_StdDescriptor_Device_t PROGMEM DeviceDescriptor =
{
	.bLength                = sizeof(USB_StdDescriptor_Device_t),
	.bDescriptorType        = DTYPE_Device,

	.bcdUSB                 = VERSION_BCD(1,0,0),
	.bDeviceClass           = 0,
	.bDeviceSubClass        = 0,
	.bDeviceProtocol        = 0,

	.bMaxPacketSize0        = 8,

	.idVendor               = 0x057E,
	.idProduct              = 0x0001,
	.bcdDevice              = VERSION_BCD(0,0,1),

	.iManufacturer          = NO_DESCRIPTOR,
	.iProduct               = STRING_ID_Product,
	.iSerialNumber          = NO_DESCRIPTOR,

	.bNumConfigurations     = 1
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
	.Config = {
		.bLength                = sizeof(USB_Descriptor_Configuration_Header_t),
		.bDescriptorType        = DTYPE_Configuration,

		.wTotalLength           = sizeof(USB_Descriptor_Configuration_t),
		.bNumInterfaces         = 1,

		.bConfigurationValue    = 1,
		.iConfiguration         = NO_DESCRIPTOR,

		.bmAttributes           = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

		.bMaxPower              = USB_CONFIG_POWER_MA(100)
	},

	.Interface = {
		.bLength                = sizeof(USB_Descriptor_Interface_t),
		.bDescriptorType        = DTYPE_Interface,

		.bInterfaceNumber       = 0,
		.bAlternateSetting      = 0,

		.bNumEndpoints          = 3,

		.bInterfaceClass        = 0xFF,
		.bInterfaceSubClass     = 0xFF,
		.bInterfaceProtocol     = 0xFF,

		.iInterface             = NO_DESCRIPTOR
	},

	.BulkIn = {
		.bLength                = sizeof(USB_Descriptor_Endpoint_t),
		.bDescriptorType        = DTYPE_Endpoint,

		.bEndpointAddress       = BULK_IN_EPADDR,
		.bmAttributes           = (EP_TYPE_BULK),
		.wMaxPacketSize         = 64,
		.bInterval              = 0
	},

	.BulkOut = {
		.bLength                = sizeof(USB_Descriptor_Endpoint_t),
		.bDescriptorType        = DTYPE_Endpoint,

		.bEndpointAddress       = BULK_OUT_EPADDR,
		.bmAttributes           = (EP_TYPE_BULK),
		.wMaxPacketSize         = 64,
		.bInterval              = 0
	},

	.InterruptIn = {
		.bLength                = sizeof(USB_Descriptor_Endpoint_t),
		.bDescriptorType        = DTYPE_Endpoint,

		.bEndpointAddress       = INTERRUPT_EPADDR,
		.bmAttributes           = (EP_TYPE_INTERRUPT),
		.wMaxPacketSize         = 8,
		.bInterval              = 5
	},

};

/* Not in the original firmware
 * Strings from the driver, added for convenience, and probably
 * best to not use the nintendo trademark
 */
const USB_Descriptor_String_t PROGMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
//const USB_Descriptor_String_t PROGMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Nintendo Co., Ltd");
//const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Nintendo Mobile Adapter Simulator USB");
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"Mobile Adapter Simulator USB");

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
			Address = &DeviceDescriptor;
			Size    = sizeof(DeviceDescriptor);
			break;
		case DTYPE_Configuration:
			Address = &ConfigurationDescriptor;
			Size    = sizeof(ConfigurationDescriptor);
			break;
		case DTYPE_String:
			switch (DescriptorNumber)
			{
				case STRING_ID_Language:
					Address = &LanguageString;
					Size    = pgm_read_byte(&LanguageString.Header.Size);
					break;
				//case STRING_ID_Manufacturer:
				//	Address = &ManufacturerString;
				//	Size    = pgm_read_byte(&ManufacturerString.Header.Size);
				//	break;
				case STRING_ID_Product:
					Address = &ProductString;
					Size    = pgm_read_byte(&ProductString.Header.Size);
					break;
			}

			break;
	}

	*DescriptorAddress = Address;
	return Size;
}

