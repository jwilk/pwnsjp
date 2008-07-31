/* Copyright © 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include <regex.h>

#ifndef REGEX_H
#define REGEX_H

void regex_free(regex_t *regex);
bool regex_compile(regex_t *regex, const char* pattern);
bool regex_match(regex_t *regex, const char *string);
char *pattern_head(const char* pattern);

#endif

// vim: ts=2 sw=2 et
