/* Copyright (c) 2005, 2006 Jakub Wilk
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

char *html_strip(char *str)
{
  enum
  {
    s_default,
    s_entity,
    s_tag_vague,
    s_tag_open,
    s_tag_close
  } state = s_default;
  char *head, *tail, *appendix;
  int len = strlen(str);
  bool first = true;
  unsigned int nl = 2;

  char result[4 * len];
  appendix = result;

#define stack_size 10
  int stack[stack_size];
  int cstack = 0;
  stack[0] = hue_normal;
  int current_hue = hue_normal;

#define a(t) ( *appendix++ = t )
#define as(t) \
  do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
#define cpush(t) \
  do { \
    cstack++; \
    if (cstack < stack_size) \
    { \
      if (current_hue & hue_persistent) \
        stack[cstack] = current_hue; \
      else \
      { \
        as(hue[t]); \
        stack[cstack] = t; \
        current_hue = t; \
      } \
    } \
  } while(0)
#define cpop() \
  do { \
    if (cstack == 0) \
      break; \
    cstack--; \
    if (cstack < stack_size) \
    { \
      current_hue = stack[cstack]; \
      as(hue[current_hue]); \
    } \
  } while (0)
#define sync() ( head = tail + 1 )
#define nhave(s, n) ( !strncasecmp(head, s, n) )
#define have(s) ( !strcasecmp(head, s) )
  
  for (head = tail = str; *tail; tail++)
  switch (state)
  {
  case s_default:
    if (*tail == '<')
      state = s_tag_vague;
    else if (*tail == '&')
      state = s_entity;
    else
    {
      if (*tail != ' ')
        nl = 0;
      if (nl == 0 || *tail != ' ')
        a(*tail);
    }
    head++;
    break;
  case s_entity:
    if (*tail == ';')
    {
      *tail = '\0';
      if (have("nbsp"))
      {
        if (nl == 0)
          a(' ');
      }
      else
      {
        nl = 0;
        a('&');
        as(head);
        a(';');
        sync();
      }
      state = s_default;
    }
    break;
  case s_tag_vague:
    if (*tail == '/')
    {
      state = s_tag_close;
      head++;
    }
    else
      state = s_tag_open;
    break;
  case s_tag_open:
    if (*tail == '>') 
    {
      state = s_default;
      *tail = '\0';
      if (have("p"))
      {
        while (nl < 2)
          a('\n'), nl++;
      }
      else if (have("p style=\"tab\""))
      {
        while (nl < 2)
          a('\n'), nl++;
        as("  ");
      }
      else if (have("br"))
      {
        while (nl < 1)
          a('\n'), nl++;
      }
      else
      {
        nl = 0;
        if (have("b"))
        {
          if (first)
          {
            cpush(hue_highlight);
            first = false;
          }
          else
            cpush(hue_blend(current_hue, -1, -1, hue_bold));
        }
        else if (have("tr1"))
          cpush(hue_highlight);
        else if (have("font color=#ff0000") || have("font color=red") || have("font color=\"#d32b2b\""))
          cpush(hue_phrase);
        else if (have("font color=\"#057e4c\"") || have("font color=#fa8d00"))
          cpush(hue_source);
        else if (have("font color=\"#959493\""))
          cpush(hue_source_p);
        else if (nhave("font", 4))
          cpush(current_hue);
        else if (have("i"))
        {
          if (tail[1] == ':' && tail[2] == ' ')
          {
            as(": ");
            tail += 2;
          }
          cpush(hue_italic);
        }
        else if (have("big"))
          cpush(hue_highlight);
        else if (have("sup"))
          a('^');
        else if (have("sub"))
          a('_');
        else if (have("sqrt"))
          as("&sqrt;");
        else if (nhave("a href=", 7))
          cpush(hue_hyperlink);
      }
      sync();
    }
    break;
  case s_tag_close:
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
  as(hue[hue_normal]);
  a('\0');

#undef a
#undef as
#undef sync
#undef cpush
#undef cpop
#undef have
#undef nhave
#undef stack_size
  
  if (*result != '\0')
  {
    char *end = strchr(result, '\0');
    do
      end--;
    while ((unsigned char)*end <= ' ');
    *++end = '\0';
  }
  return pwnstr_to_str(result);
}

// vim: ts=2 sw=2 et
