/* Copyright (c) 2005, 2006 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "hue.h"

#include "terminfo.h"

char* HUE(count);

void hue_setup_curses(void)
{
  static char cbuf[HUE_count][3];
  unsigned int i;
  for (i=0; i<HUE_count; i++)
  {
    cbuf[i][0] = '\x1B';
    cbuf[i][1] = i + '0';
    cbuf[i][2] = '\0';
    __huekit[i] = cbuf[i];
  }
}

void hue_setup_terminfo(void)
{
#define bufsize 24
  HUE(normal) = term_sgr0;
  static char cbuf[HUE_count][bufsize];

#define build_color(k, s1, s2) \
  do { \
    snprintf(cbuf[HUE_##k], bufsize, "%s%s%s", term_sgr0, s1, s2); \
    cbuf[HUE_##k][bufsize-1]='\0'; \
    HUE(k) = cbuf[HUE_##k]; \
  } while (false)

  HUE(normal) = HUE(misc) = term_sgr0;
  build_color(title, term_setab[4], "");
  build_color(boldtitle, term_setab[4], term_bold);
  build_color(highlight, term_setaf[4], term_bold);
  build_color(bold, term_bold, "");
  build_color(hyperlink, term_setaf[6], "");
  build_color(italic, term_setaf[1], "");
  build_color(phrase, term_setaf[1], term_bold);
  build_color(reverse, term_setab[7], term_setaf[0]);
  
#undef bufsize
#undef build_color
}

// vim: ts=2 sw=2 et
