/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include <stdint.h>

#ifndef VALIDATE_H
#define VALIDATE_H

#ifdef K_VALIDATE_DATAFILE
const unsigned int datafile_size = 0x6694eb4;
const uint8_t datafile_sha1[20] = { 0x89, 0xbf, 0x99, 0xc2, 0x20, 0x89, 0x25, 0x2c, 0xaa, 0xa3, 0x15, 0xae, 0x90, 0x63, 0x06, 0xc7, 0x5a, 0x73, 0xdc, 0x9b };
#endif

#endif

// vim: ts=2 sw=2 et
