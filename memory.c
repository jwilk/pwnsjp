/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "memory.h"

#include <errno.h>

void __fatal(const char* fstr, int line, const char* errstr)
{
  if (fstr == NULL)
    fstr = "?";
  if (errstr == NULL)
    errstr = strerror(errno);
  fprintf(stderr, "%s[%d]: %s\n", fstr, line, errstr);
  exit(EXIT_FAILURE);
}

#define memcheck(x) do { if ((x) == NULL) fatal(NULL); } while(0)

void *alloc(size_t nmemb, size_t size)
{
  void *mem = malloc(nmemb * size);
  memcheck(mem);
  return mem;
}

void *alloz(size_t nmemb, size_t size)
{
  void *mem = calloc(nmemb, size);
  memcheck(mem);
  return mem;
}

char *str_clone(const char *s)
{
  char *clone = strdup(s);
  memcheck(clone);
  return clone;
}

extern wchar_t *wcsdup(const wchar_t*);

wchar_t *wcs_clone(const wchar_t *s)
{
  wchar_t *clone = wcsdup(s);
  memcheck(clone);
  return clone;
}

// vim: ts=2 sw=2 et
