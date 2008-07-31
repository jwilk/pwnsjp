/* Copyright © 2005, 2006 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "memory.h"
#include "unicode.h"

#include <langinfo.h>
#include <locale.h>

#include "cmap-cp1250.h"
#include "cmap-iso8859-13.h"
#include "cmap-iso8859-16.h"
#include "cmap-iso8859-2.h"
#include "cmap-usascii.h"
#include "entity-hash.h"
#include "entity.h"

#include "config.h"

static const char *rev_iso8859n = NULL;

static enum 
{ 
  cmap_usascii    = 0, 
  cmap_iso88592   = 2, 
  cmap_iso885913  = 13, 
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
    else if (!strcmp(codeset, "ISO-8859-13"))
    {
      cmap = cmap_iso885913;
      rev_iso8859n = rev_iso885913;
    }
    else if (!strcmp(codeset, "ISO-8859-16"))
    {
      cmap = cmap_iso885916;
      rev_iso8859n = rev_iso885916;
    }
    else
      codeset = "US-ASCII";
    debug("codeset = %s\n", codeset);
    char *collstr = setlocale(LC_COLLATE, NULL);
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

static const char *entity_grep(const char *str, wchar_t *result)
{
  char *str_next = strchr(str, ';');
  if (str_next == NULL)
    return str;
  int len = str_next - str;
  const struct entity_ptr *enptr = entity_lookup(str, len);
  if (enptr == NULL)
    return str;
  wchar_t *resolved = entity_list[enptr->id - 1].values;
  while (*resolved != L'\0')
  {
    *result++ = *resolved++;
  }
  return str + len;
}

static wchar_t *pwn_charset = (wchar_t*) cp1250;

void set_pwn_charset(bool use_cp1250)
{
  pwn_charset = (wchar_t*) (use_cp1250 ? cp1250 : iso8859_2);
}

static wchar_t *pwnstr_to_ustr(const char *str)
{
  wchar_t* result = alloz(1 + strlen(str), sizeof(wchar_t));
  wchar_t* resptr;
  for (resptr = result; *str; str++, resptr++)
  {
    if (*str & 0x80)
      *resptr = pwn_charset[*str & 0x7f];
    else if ((*resptr = *str) == '&')
    {
      str = entity_grep(str + 1, resptr);
      while (resptr[1] != L'\0')
        resptr++;
    }
  }
  *resptr = L'\0';
  return result;
}

static char *ustr_fallback_ascii(const wchar_t *ustr)
{
  int i, len = wcslen(ustr), biglen = 4 * (len + 1);
  char result[biglen], *appendix;
  memset(result, 0, biglen); 
  appendix = result;
#define a(t) ( *appendix++ = t )
#define as(t) do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
  for (i = 0; i < len; i++)
  {
    if (ustr[i] < 0xa0)
      a(ustr[i]);
    else if (ustr[i] < 0x180)
    {
      unsigned int j = ustr[i] - 0xa0;
      char code = '\0';
      if (rev_iso8859n != NULL)
        code = rev_iso8859n[j];
      if (code != '\0')
        a(code);
      else
        as(rev_usascii[j]);
    }
    else if (ustr[i] >= 0x300 && ustr[i] < 0x370)
    {
      // combining character
      if (ustr[i] == 0x32f)
      {
        // inversed breve below a characted
        appendix[0] = appendix[-1];
        appendix[-1] = '{';
        appendix[1] = '}';
        appendix += 2;
      }
    }
    else
    {
      const struct entity *ent;
      for (ent = entity_list; ent->name; ent++)
      if (ustr[i] == ent->values[0] && ent->str)
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
  return str_clone(result);
}

static char* ustr_fallback_sys(const wchar_t *ustr)
{
  int lim = 1 + wcstombs(NULL, ustr, 0);
  char* result = alloc(lim, sizeof(char));
  if (wcstombs(result, ustr, lim) == (size_t)(-1))
    fatal("wcstombs failed!");
  else
    return result;
  return NULL; // suppress compiler warnings
}

wchar_t *str_to_ustr(const char *str)
{
  int lim = 1 + mbstowcs(NULL, str, 0);
  wchar_t *result = alloc(lim, sizeof(wchar_t));
  if (mbstowcs(result, str, lim) == (size_t)(-1))
    fatal("{mbstowcs failed!}");
  else
    return result;
  return NULL; // suppress compiler warnings
}

char *ustr_to_str(const wchar_t *ustr)
{
  switch(cmap)
  {
    case cmap_usascii:
    case cmap_iso88592:
    case cmap_iso885913:
    case cmap_iso885916:
      return ustr_fallback_ascii(ustr);
    case cmap_utf8:
      return ustr_fallback_sys(ustr);
  }
  assert("invalid character map" == NULL);
  return NULL; // suppress compiler warnings
}

char *pwnstr_to_str(const char *str)
{
  wchar_t* ustr = pwnstr_to_ustr(str);
  char* result = ustr_to_str(ustr);
  free(ustr);
  return result;
}

size_t strnwidth(const char *str, size_t len)
{
  size_t width = 0;
  // FIXME: it _might_ produce wrong results (yet I presume it won't)
  if (cmap == cmap_utf8)
  {
    for ( ; len > 0; str++, len--)
    if (*str == '\0')
      break;
    else if ((*str & 0xc0) != 0x80)
      width++;
    return width;
  }
  else
  {
    for ( ; len > 0; str++, len--)
    if (*str == '\0')
      break;
    else
      width++;
  }
  return width;
}

char *strxform(const char *str)
{
  int lim = 1 + strxfrm(NULL, str, 0);
  char *result = alloc(lim, sizeof(char));
  strxfrm(result, str, lim);
  return result;
}

bool posix_coll(void)
{
  return coll == coll_posix; 
}

// vim: ts=2 sw=2 et
