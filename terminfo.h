/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"

#ifndef TERMINFO_H
#define TERMINFO_H

void term_init(void);

extern bool is_term;

extern char *term_setaf[8];
extern char *term_setab[8];
extern char *term_sgr0;
extern char *term_bold;

#endif

// vim: ts=2 sw=2 et
