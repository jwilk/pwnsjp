#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
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

#endif

// vim: ts=2 sw=2 et
