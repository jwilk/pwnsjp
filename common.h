/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define debug(s, ...) \
  do \
  { \
    if (config.conf_debug) \
    { \
      fprintf(stderr, "| "); \
      fprintf(stderr, s, ## __VA_ARGS__); \
    } \
  } while(0)

#ifndef K_DATA_PATH
#  define K_DATA_PATH "./slo.win"
#endif

#ifndef K_VERSION
#  define K_VERSION "<devel>"
#endif

#endif

// vim: ts=2 sw=2 et
