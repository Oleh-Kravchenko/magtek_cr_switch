/*
 * Mag-Tek Mini Swipe Reader mode switch tool
 *
 * Copyright (c) 2010 Oleg Kravchenko
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <libusb.h>

#define __MAGTEK_VENDOR  0x0801
#define __KEYBRD_PRODUCT 0x0001
#define __READER_PRODUCT 0x0002

/*-------------------------------------------------------------------------*/

#define __INTERFACE 0

/// See <http://www.usb.org> for more information
#define __HID_SET_REPORT 0x09

/*-------------------------------------------------------------------------*/

#define __RESULT(exp)					\
	if(exp)								\
		puts("successfully");			\
	else								\
		puts("failed");

/*-------------------------------------------------------------------------*/

unsigned char set_hid[0x18]   = {0x01, 0x02, 0x10, 0x00};
unsigned char set_keyb[0x18]  = {0x01, 0x02, 0x10, 0x01};
unsigned char set_reset[0x18] = {0x02};

/*-------------------------------------------------------------------------*/

const char help[] =
	"MagTek Card Reader mode switch tool\n\n"
	"Copyright (c) 2010 Oleg Kravchenko\n\n"
	"This program is free software; you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License version 2 as\n"
	"published by the Free Software Foundation.\n\n"
	"Usage: magtek_cr_switch [-h] [-s]\n"
	"-h - show this help\n"
	"-s - switch card reader mode";


/*-------------------------------------------------------------------------*/

int usb_set_feature_report(libusb_device_handle* dev_h, unsigned char* data, size_t len)
{
	return
	(
		libusb_control_transfer
		(
			dev_h,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
			__HID_SET_REPORT, (LIBUSB_REQUEST_SET_FEATURE << 8) | 0,
			0, data, len, 5000
		)
	);
}

/*-------------------------------------------------------------------------*/

int main(int argc, char** argv)
{
	libusb_device_handle* dev_h;
	int res, do_sw = 0, magtek_mode = 0;

	while((res = getopt(argc, argv, "sh")) != -1)
	{
		switch(res)
		{
			case 's':
				do_sw = 1;
				break;
			case 'h':
				puts(help);
				return(0);
			case '?':
				return(1);
        }
	}

	if(libusb_init(NULL))
	{
		puts("Can't initialize libusb!");

		return(1);
	}

	// try open as keyboard device
	dev_h = libusb_open_device_with_vid_pid(NULL, __MAGTEK_VENDOR, __KEYBRD_PRODUCT);

	if(!dev_h)
	{
		magtek_mode = 1;

		// try open as hid device
		dev_h = libusb_open_device_with_vid_pid(NULL, __MAGTEK_VENDOR, __READER_PRODUCT);

		if(!dev_h)
		{
			puts("Mag-Tek Mini Swipe Reader not found!");

			libusb_exit(NULL);

			return(1);
		}
		else
		puts("Mag-Tek Mini Swipe Reader in HID mode detected.");
	}
	else
		puts("Mag-Tek Mini Swipe Reader in Keyboard mode detected.");

	// only detect device
	if(!do_sw)
	{
		libusb_close(dev_h);
		libusb_exit(NULL);

		return(0);
	}

	// detaching a kernel driver if need
	switch(libusb_kernel_driver_active(dev_h, 0))
	{
		case 0:
			puts("No kernel driver is active.");
			break;

		case 1:
			printf
			(
				"Kernel driver is active. \n"
				"Detaching a kernel driver from an interface 0x%x ... ",
				__INTERFACE
			); fflush(stdout);

			if(libusb_detach_kernel_driver(dev_h, __INTERFACE))
			{
				puts("failed");

				libusb_close(dev_h);
				libusb_exit(NULL);

				return(1);
			}
			else
				puts("successfully");

			break;

		default:
			break;
	}

	if(libusb_claim_interface(dev_h, 0))
		puts("Claiming interface failed!");

	// switch mode of device
	if(magtek_mode)
	{
		printf("Switching to keyboard mode ... "); fflush(stdout);
		__RESULT(usb_set_feature_report(dev_h, set_keyb, sizeof(set_keyb)) > 0);
	}
	else
	{
		printf("Switching to HID mode ... "); fflush(stdout);
		__RESULT(usb_set_feature_report(dev_h, set_hid, sizeof(set_hid)) > 0);
	}

	// reseting device to apply changes
	printf("Reseting device ... "); fflush(stdout);
	__RESULT(usb_set_feature_report(dev_h, set_reset, sizeof(set_reset)) > 0);

	// waiting 4 seconds, MagTek recommened (?)
	usleep(4000);

	libusb_release_interface(dev_h, 0);

	libusb_reset_device(dev_h);

	libusb_close(dev_h);
	libusb_exit(NULL);

	return(0);
}
