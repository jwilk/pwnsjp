/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#ifndef MEMORY_H
#define MEMORY_H

void __fatal(unsigned char* fstr, int line, unsigned char* errstr);
#define fatal(errstr) __fatal(__FILE__, __LINE__, errstr)

inline void* alloc(size_t nmemb, size_t size);
inline void* alloz(size_t nmemb, size_t size);
void free(void *ptr);

inline unsigned char* str_clone(const unsigned char*);
inline wchar_t* wcs_clone(const wchar_t*);

#endif

// vim: ts=2 sw=2 et
