/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "unicode.h"

#include <langinfo.h>
#include <locale.h>

#include "cmap-cp1250.h"
#include "cmap-usascii.h"
#include "cmap-iso8859-2.h"
#include "cmap-iso8859-16.h"
#include "entity.h"
#include "entity-hash.h"

#include "config.h"

static const unsigned char* rev_iso8859n = NULL;

static enum 
{ 
  cmap_usascii    = 0, 
  cmap_iso88592   = 2, 
  cmap_iso885916  = 16, 
  cmap_utf8       = -1
} cmap = cmap_usascii;

static enum
{
  coll_posix = 0,
  coll_unknown = -1
} coll = coll_unknown;

void unicode_init(void)
{
  char *locale = setlocale(LC_ALL, "");
  if (locale)
  {
    debug("LC_ALL = \"%s\"\n", locale);
    char *codeset = nl_langinfo(CODESET);
    if (!strcmp(codeset, "UTF-8"))
      cmap = cmap_utf8;
    else if (!strcmp(codeset, "ISO-8859-2"))
    {
      cmap = cmap_iso88592;
      rev_iso8859n = rev_iso88592;
    }
    else if (!strcmp(codeset, "ISO-8859-16"))
    {
      cmap = cmap_iso885916;
      rev_iso8859n = rev_iso885916;
    }
    char* collstr = setlocale(LC_COLLATE, NULL);
    if (collstr != NULL)
    {
      if (!strcmp(collstr, "C") || !strcmp(collstr, "POSIX"))
        coll = coll_posix;
      debug("LC_COLLATE = \"%s\" (%sPOSIX)\n", collstr, coll == coll_posix ? "" : "non-");
    }
  }
  else
    debug("unable to set locale!\n");
}

static const char* entity_grep(const unsigned char *str, wchar_t *result)
{
  unsigned int hash = 0;
  int i, j;
  for(j=0; j<hash_coeff_count && str[j]!=0 && str[j]!=';'; j++)
    hash ^= str[j] + hash_coeff[j];
  if ( (i = entity_hash[hash % hash_size]) >= 0 )
  {
    *result = entity_list[i].value;
    return str + j;
  }
  return str;
}

static wchar_t* pwnstr_to_ustr(const unsigned char *str)
{
  wchar_t* result = calloc(1 + strlen(str), sizeof(wchar_t));
  wchar_t* resptr;
  for (resptr=result; *str; str++, resptr++)
  {
    if (*str & 0x80)
      *resptr = cp1250[*str & 0x7f];
    else if ((*resptr = *str) == '&')
    {
      str = entity_grep(str+1, resptr);
      if (*resptr & 0x10000000)
      {
        *resptr++ &= 0xffff;
        *resptr    = 0x20d7;
      }
    }
  }
  *resptr = L'\0';
  return result;
}

static unsigned char* ustr_fallback_ascii(const wchar_t *ustr)
{
  int i, len = wcslen(ustr), biglen=4*(len+1);
  unsigned char result[biglen], *appendix;
  memset(result, 0, biglen); 
  appendix = result;
#define a(t) do *appendix++ = t; while (0)
#define as(t) do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
  for (i=0; i<len; i++)
  {
    if (ustr[i] < 0x00a0)
      a(ustr[i]);
    else if (ustr[i] < 0x0180)
    {
      unsigned int j = ustr[i]-0xa0;
      unsigned char code = 0;
      if (rev_iso8859n != NULL)
        code = rev_iso8859n[j];
      if (code != 0)
        a(code);
      else
        as(rev_usascii[j]);
    }
    else
    {
      const struct entity* ent;
      for (ent = entity_list; ent->name; ent++)
      if (ustr[i] == ent->value && ent->str)
      {
        as(ent->str);
        break;
      }
      if (ent->name == NULL)
        as("{?}");
    }
  }
#undef a
#undef as
  return strdup(result);
}

static unsigned char* ustr_fallback_sys(const wchar_t *ustr)
{
  int lim = 1 + wcstombs(NULL, ustr, 0);
  unsigned char* result = malloc(lim * sizeof(unsigned char));
  if (wcstombs(result, ustr, lim) == (size_t)(-1))
    return strdup("{wcstombs failed!}");
  else
    return result;
}

wchar_t* str_to_ustr(const unsigned char *str)
{
  int lim = 1 + mbstowcs(NULL, str, 0);
  wchar_t* result = malloc(lim * sizeof(wchar_t));
  if (mbstowcs(result, str, lim) == (size_t)(-1))
    return wcsdup(L"{mbstowcs failed!}");
  else
    return result;
}

unsigned char* ustr_to_str(const wchar_t *ustr)
{
  switch(cmap)
  {
    case cmap_usascii:
    case cmap_iso88592:
    case cmap_iso885916:
      return ustr_fallback_ascii(ustr);
    case cmap_utf8:
      return ustr_fallback_sys(ustr);
  }
  assert("invalid character map" == NULL);
  return NULL; // suppress compiler warnings
}

char* pwnstr_to_str(const unsigned char *str)
{
  wchar_t* ustr = pwnstr_to_ustr(str);
  unsigned char* result = ustr_to_str(ustr);
  free(ustr);
  return result;
}

size_t strnwidth(const unsigned char *str, size_t len)
{
  size_t width = 0;
  // FIXME: it _might_ produce wrong results (yet I presume it won't)
  if (cmap == cmap_utf8)
  {
    for ( ; len>0; str++, len--)
    if (*str == '\0')
      break;
    else if ((*str & 0xc0) != 0x80)
      width++;
    return width;
  }
  else
  {
    for ( ; len>0; str++, len--)
    if (*str == '\0')
      break;
    else
      width++;
  }
  return width;
}

unsigned char* strxform(const unsigned char *str)
{
  int lim = 1 + strxfrm(NULL, str, 0);
  unsigned char* result = malloc(lim);
  strxfrm(result, str, lim);
  return result;
}

inline bool posix_coll(void)
{
  return coll == coll_posix; 
}

// vim: ts=2 sw=2 et
