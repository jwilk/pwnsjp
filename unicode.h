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
inline bool posix_coll(void);
char* pwnstr_to_str(const char *);

unsigned char* ustr_to_str(const wchar_t *);
wchar_t* str_to_ustr(const unsigned char *);

size_t strnwidth(const unsigned char *, size_t);
unsigned char* strxform(const unsigned char*);

int wcwidth(wchar_t);
wchar_t *wcsdup(const wchar_t *);

#endif

// vim: ts=2 sw=2 et

