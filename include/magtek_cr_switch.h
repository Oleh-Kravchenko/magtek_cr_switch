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

#ifndef __MAGTEK_CR_SWITCH_H
#define __MAGTEK_CR_SWITCH_H

/** interface for feature reports */
#define __INTERFACE 0

/** see <http://www.usb.org> for more information */
#define __HID_SET_REPORT 0x09

/*-------------------------------------------------------------------------*/

struct command_info
{
	/** command description */
	char desc[0xff];

	unsigned char len;
	unsigned char data[0xff];
};

/*-------------------------------------------------------------------------*/

struct magtek_device_info
{
	/** device name */
	char name[32];

	/** vendor id */
	unsigned short vendor;

	/** product id */
	unsigned short product;

	struct command_info cmd;
	struct command_info reset;
};

#endif /* __MAGTEK_CR_SWITCH_H */
