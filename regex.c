/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "config.h"
#include "regex.h"

inline void regex_free(regex_t *regex)
{
  regfree(regex);
}

inline bool regex_compile(regex_t *regex, const unsigned char* p)
{
  if (p == NULL)
  {
    debug("pattern = NULL\n");
    return true;
  }
  debug("pattern = \"%s\"\n", p);
  return
    regcomp(regex, p, REG_NOSUB | REG_EXTENDED | REG_ICASE) == 0;
}

inline unsigned char* pattern_head(const unsigned char* p)
{
  if (p == NULL || *p != '^')
    return NULL;
  
  do p++; while (*p == '^');
  
  bool esc = false;
  unsigned char result[1 + strlen(p)];
  result[0]='\0';
  unsigned char* r;
  for (r=result+1; *p; p++)
  if (esc)
    switch (*p)
    {
    case '^': case '.': case '[': case ']': case '$': case '(': case ')': 
    case '|': case '*': case '+': case '?': case '{': case '}': case '\\':
      *r++ = *p;
      esc = false;
      break;
    default:
      goto enough;
    }
  else
    switch (*p)  
    {
    case '\\':
      esc = true;
      break;
    case '^': case '|':
      return NULL;
    case '+': case '.': case '$': case '[': case '(':
      goto enough;
    case '*': case '?': case '{':
      r--;
      goto enough;
    default:
      *r++ = *p;
    }
enough:
  *r = '\0';
  debug("pattern head = \"%s\"\n", result+1);
  return strdup(result+1);
}

inline bool regex_match(regex_t *regex, const unsigned char *string)
{
  return regexec(regex, string, 0, NULL, 0) == 0;
}

// vim: ts=2 sw=2 et
