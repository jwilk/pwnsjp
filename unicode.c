#include "common.h"
#include <langinfo.h>
#include <locale.h>

#include "cmap-cp1250.h"
#include "cmap-usascii.h"
#include "cmap-iso8859-2.h"
#include "cmap-iso8859-16.h"
#include "entity.h"
#include "entity-hash.h"

#include "config.h"

static enum 
{ 
  cmap_usascii    = 0, 
  cmap_iso88592   = 2, 
  cmap_iso885916  = 16, 
  cmap_utf8       = -1
} cmap = cmap_usascii;

void unicode_init(void)
{
  char *locale = setlocale(LC_ALL, "");
  if (locale)
  {
    debug("set locale to: <%s>\n", locale);
    char *codeset = nl_langinfo(CODESET);
    if (!strcmp(codeset, "UTF-8"))
      cmap = cmap_utf8;
    else if (!strcmp(codeset, "ISO-8859-2"))
      cmap = cmap_iso88592;
    else if (!strcmp(codeset, "ISO-8859-16"))
      cmap = cmap_iso885916;
  }
  else
    debug("unable to set locale!");
}

static const char* entity_grep(const unsigned char *str, wchar_t *result)
{
  unsigned int hash = 0;
  int i, j;
  for(j=0; j<hash_coeff_count && str[j]!=0 && str[j]!=';'; j++)
    hash ^= str[j]+hash_coeff[j];
  if ( (i = entity_hash[hash % hash_size]) >= 0 )
  {
    *result = entity_list[i].value;
    return str+j;
  }
  return str;
}

static wchar_t* pwnstr_to_ustr(const unsigned char *str)
{
  size_t i;
  wchar_t* result = calloc(1+strlen(str), sizeof(wchar_t));
  for (i=0; *str; str++, i++)
  {
    if (*str >= 0x80)
      result[i] = cp1250[*str & 0x7f];
    else
    if ((result[i] = *str) == '&')
    {
      str = entity_grep(str+1, result+i);
      if (result[i] & 0x10000000)
      {
        result[i]  &= 0xffff;
        result[++i] = 0x20d7;
      }
    }
  }
  result[i] = 0;
  return result;
}

static unsigned char* ustr_fallback_ascii(const wchar_t *ustr, unsigned int us)
{
  int i, len = wcslen(ustr), biglen=4*(len+1);
  unsigned char result[biglen], *appendix;
  memset(result, 0, biglen); 
  appendix = result;
#define a(t) do *(appendix++)=t; while (0)
#define as(t) do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
  for (i=0; i<len; i++)
  {
    if (ustr[i] < 0x00a0)
      a(ustr[i]);
    else if (ustr[i] < 0x017f)
    {
      unsigned char code=0;
      if (us == 2)
        code=rev_iso88592[ustr[i]-0x00a0];
      else if (us == 16)
        code=rev_iso885916[ustr[i]-0x00a0];
      if (code != 0)
        a(code);
      else
        as(rev_usascii[ustr[i]-0x00a0]);
    }
    else
    {
      int j;
      for (j=0; entity_list[j].name; j++)
      if (ustr[i] == entity_list[j].value && entity_list[j].str)
      {
        as(entity_list[j].str);
        break;
      }
      if (entity_list[j].name == NULL)
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
  unsigned char* result = malloc(lim);
  if (wcstombs(result, ustr, lim) == (size_t)(-1))
    return strdup("{wcstombs failed!}");
  else
    return result;
}

static unsigned char* ustr_to_str(const wchar_t *ustr)
{
  switch(cmap)
  {
    case cmap_usascii:
    case cmap_iso88592:
    case cmap_iso885916:
      return ustr_fallback_ascii(ustr, cmap);
    case cmap_utf8:
      return ustr_fallback_sys(ustr);
  }
  return "{invalid character map}"; // this should not happen
}

char* pwnstr_to_str(const char *str)
{
  wchar_t* ustr = pwnstr_to_ustr(str);
  char* result = ustr_to_str(ustr);
  free(ustr);
  return result;
}

// vim: ts=2 sw=2 et
