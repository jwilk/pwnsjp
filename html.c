/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "html.h"

#include "hue.h"
#include "terminfo.h"
#include "unicode.h"

/*
 * List of unhandled tags:
 *  - <above>
 *  - <font color=#23b002>
 *  - <font color=#3030bf>
 *  - <j>
 *  - <przysl>
 *  - <s>
 *  - <term z=puste>
 * List of ignored tags:
 *  - <font size=-1>
 *  - <font size=5>
 *  - <g>
 *  - <glo>
 *  - <k>
 *  - <l>
 *  - <math>
 *  - <q>
 *  - <r1>
 *  - <small>
 */

unsigned char* html_strip(unsigned char *str)
{
  enum
  {
    s_default,
    s_html_vague,
    s_html_open,
    s_html_close
  } state = s_default;
  unsigned char *head, *tail, *appendix;
  int len = strlen(str);
  bool first = true;
  bool ignorep = true;
  unsigned int cplace = 0;
  unsigned int cstack = 0;
  
  unsigned char result[4*len];
  appendix = result;

#define a(t) ( *appendix++ = t )
#define as(t) \
  do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
#define cpress(t) \
  do { \
    if (cstack == cplace) \
      as(t); \
    cstack++; \
  } while (0)
#define cpush(t) \
  do { as(HUE(t)); cplace = cstack; cstack++; } while(0)
#define cpop() \
  do { \
    if (cstack > 0 && --cstack == cplace ) \
      as(HUE(normal)); \
  } while (0)
#define sync() ( head = tail+1 )
#define nhave(s, n) ( !strncasecmp(head, s, n) )
#define have(s) ( !strcasecmp(head, s) )
  
  for (head=tail=str; *tail; tail++)
  switch(state)
  {
  case s_default:
    if (*tail == '<')
      state = s_html_vague;
    else
    {
      ignorep = false;
      a(*tail);
    }
    head++;
    break;
  case s_html_vague:
    if (*tail == '/')
    {
      state = s_html_close;
      head++;
    }
    else
      state = s_html_open;
    break;
  case s_html_open:
    if (*tail == '>') 
    {
      state = s_default;
      *tail = '\0';
      if (have("p"))
      {
        if (ignorep)
          ignorep = false;
        else
          a('\n');
      }
      else if ((ignorep=false) || have("p style=\"tab\""))
        as("\n   ");
      else if (have("br"))
        a('\n');
      else if (have("b"))
      {
        if (first)
        {
          cpush(highlight);
          first = false;
        }
        else
          cpush(bold);
      }
      else if (have("tr1"))
        cpush(highlight);
      else if (have("font color=#ff0000") || have("font color=red"))
        cpush(phrase);
      else if (have("font color=#fa8d00"))
        cpush(misc);
      else if (nhave("font", 4))
        cpress("");
      else if (have("i"))
        cpress(HUE(italic));
      else if (have("big"))
        cpush(highlight);
      else if (have("sup"))
        a('^');
      else if (have("sub"))
        a('_');
      else if (have("sqrt"))
        as("&sqrt;");
      else if (nhave("a href=", 7))
        cpress(HUE(hyperlink));
      sync();
    }
    break;
  case s_html_close:
    if (*tail == '>')
    {
      *tail = '\0';
      state = s_default;
      if (have("a") || have("b") || have("i") || have("big") || have("tr1") || have("font"))
        cpop();
      else if (have("p"))
        while(tail[1] == ' ')
          tail++;
      sync();
    }
    break;
  }
  as(HUE(normal));
  a('\0');

#undef a
#undef as
#undef sync
#undef cpush
#undef cpush
#undef cpop
#undef have
#undef nhave
  
  if (*result != '\0')
  {
    unsigned char *end = strchr(result, '\0');
    do
      end--;
    while (*end <= ' ');
    *++end = '\0';
  }
  return pwnstr_to_str(result);
}

// vim: ts=2 sw=2 et
