/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include <wchar.h>
#include <wctype.h>

#ifndef UNICODE_H
#define UNICODE_H

void unicode_init(void);
char* pwnstr_to_str(const char *);

unsigned char* ustr_to_str(const wchar_t *wcs);
wchar_t* str_to_ustr(const unsigned char *s);

size_t strnwidth(const unsigned char *s, size_t n);

int wcwidth(wchar_t c);
wchar_t *wcsdup(const wchar_t *s);

#endif

// vim: ts=2 sw=2 et

