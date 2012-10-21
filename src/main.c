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
#include <unistd.h>

#include "magtek_cr_switch.h"

/*-------------------------------------------------------------------------*/

static struct magtek_device_info devices[2] =
{
	{
		.name = "MagTek Card Reader HID mode",
		.vendor = 0x0801,
		.product = 0x0001,
		.cmd =
		{
			.desc = "Switching to keyboard mode",
			.len = 0x18,
			.data = {0x01, 0x02, 0x10, 0x01},
		},
		.reset =
		{
			.desc = "Reset",
			.len = 0x18,
			.data = {0x02},
		},
	},
	{
		.name = "Switching keyboard mode",
		.vendor = 0x0801,
		.product = 0x0002,
		.cmd =
		{
			.desc = "Switching to HID mode",
			.len = 0x18,
			.data = {0x01, 0x02, 0x10, 0x00},
		},
		.reset =
		{
			.desc = "Reset",
			.len = 0x18,
			.data = {0x02},
		},
	},
};

/*-------------------------------------------------------------------------*/

const char copyright[] =
	"Copyright (c) 2010 Oleg Kravchenko\n"
	"MagTek Card Reader mode switch tool\n"
	"\n"
	"This program is free software; you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License version 2 as\n"
	"published by the Free Software Foundation.\n";

const char help[] =
	"Usage: magtek_cr_switch [-h] [-s]\n"
	"-h - show this help\n"
	"-s - switch card reader mode";

/*-------------------------------------------------------------------------*/

int usb_set_feature_report(libusb_device_handle* dev_h, unsigned char* data, size_t len)
{
	int res = libusb_control_transfer
	(
		dev_h,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
		__HID_SET_REPORT, (LIBUSB_REQUEST_SET_FEATURE << 8) | 0,
		0, data, len, 5000
	);

	/* waiting 4 seconds, MagTek recommened (?) */
	usleep(4000);

	return(res);
}

/*-------------------------------------------------------------------------*/

void set_feature_report(libusb_device_handle* dev_h, struct command_info *cmd)
{
	/* set feature report to device */
	printf("%s ... ", cmd->desc); fflush(stdout);
	puts((usb_set_feature_report(dev_h, cmd->data, cmd->len) > 0) ? "successefully" : "failed");
}

/*-------------------------------------------------------------------------*/

int deattach_driver(libusb_device_handle* dev_h, int interface)
{
	/* detaching a kernel driver if need */
	switch(libusb_kernel_driver_active(dev_h, interface))
	{
		case 0:
			puts("No kernel driver is active.");
			break;

		case 1:
			printf
			(
				"Kernel driver is active. \n"
				"Detaching a kernel driver from an interface 0x%.2x ... ",
				__INTERFACE
			); fflush(stdout);

			if(libusb_detach_kernel_driver(dev_h, interface))
			{
				puts("failed");

				return(-1);
			}
			else
				puts("successfully");

			break;

		default:
			puts("Can't determine kernel driver status!");
			return(-1);
	}

	return(0);
}

/*-------------------------------------------------------------------------*/

int main(int argc, char** argv)
{
	struct magtek_device_info *dev = devices;
	libusb_device_handle* dev_h;
	int param;
	int detect = 1;
	int exit_code = 1;

	puts(copyright);

	while((param = getopt(argc, argv, "sh")) != -1)
	{
		switch(param)
		{
			case 's':
				detect = 0;
				break;
			case 'h':
				puts(help);
				exit_code = 0;
				goto exit;
			case '?':
				puts(help);
				goto exit;
        }
	}

	if(libusb_init(0))
	{
		puts("Can't initialize libusb!");

		goto exit;
	}

	/* try open as keyboard device */
	while(dev->vendor)
	{
		dev_h = libusb_open_device_with_vid_pid(0, dev->vendor, dev->product);

		if(dev_h)
		{
			printf("%s detected.\n", dev->name);

			/* only detect device */
			if(detect)
			{
				exit_code = 0;

				goto exit_device;
			}

			break;
		}

		++ dev;
	}

	if(!dev_h)
	{
		puts("Mag-Tek Mini Swipe Reader not found!");

		goto exit_libusb;
	}

	if(deattach_driver(dev_h, __INTERFACE))
		goto exit_device;

	if(libusb_claim_interface(dev_h, __INTERFACE))
	{
		puts("Claiming interface failed!");

		goto exit_device;
	}

	exit_code = 0;

	/* set feature report to device and */
	/* reseting device to apply changes */
	set_feature_report(dev_h, &dev->cmd);
	set_feature_report(dev_h, &dev->reset);

	libusb_release_interface(dev_h, 0);
	libusb_reset_device(dev_h);

exit_device:
	libusb_close(dev_h);

exit_libusb:
	libusb_exit(0);

exit:
	return(exit_code);
}
