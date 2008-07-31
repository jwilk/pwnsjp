/* Copyright © 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include <wchar.h>
#include <wctype.h>

#ifndef UNICODE_H
#define UNICODE_H

void unicode_init(void);
bool posix_coll(void);
void set_pwn_charset(bool);
char* pwnstr_to_str(const char *);

char* ustr_to_str(const wchar_t*);
wchar_t* str_to_ustr(const char *);

size_t strnwidth(const char*, size_t);
char* strxform(const char*);

int wcwidth(wchar_t);

#endif

// vim: ts=2 sw=2 et
