/* Copyright (c) 2005, 2006 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "hue.h"

#include "terminfo.h"

int hue_blend(int hue, int new_fg, int new_bg, int new_ex)
{
  if (new_fg != -1) { hue &= ~hue_fg_mask; hue |= new_fg; }
  if (new_bg != -1) { hue &= ~hue_bg_mask; hue |= new_bg; }
  if (new_ex != -1) { hue &= ~hue_ex_mask; hue |= new_ex; }
  return hue;
}

char *hue[hue_count];

void hue_setup_curses(void)
{
  static char cbuf[hue_count][6];
  unsigned int i;
  assert(hue_count < 0x1000);
  for (i = 0; i < hue_count; i++)
  {
    sprintf(cbuf[i], "\x1B%03xm", i);
    hue[i] = cbuf[i];
  }
}

void hue_setup_terminfo(void)
{
#define bufsize 32
  static char cbuf[hue_count][bufsize];

  for (int i = 0; i < hue_count; i++)
    strncat(cbuf[i], term_sgr0, bufsize - 1);

  for (int fg = 0; fg < hue_fg_count; fg++)
  for (int bg = 0; bg < hue_bg_count; bg++)
  for (int ex = 0; ex < hue_ex_count; ex++)
  {
    int i = (fg << hue_fg_shift) + (bg << hue_bg_shift) + (ex << hue_ex_shift);
    strncat(cbuf[i], term_setaf[fg], bufsize - 1);
    strncat(cbuf[i], term_setab[bg], bufsize - 1);
    if ((ex << hue_ex_shift) & hue_bold)
      strncat(cbuf[i], term_bold, bufsize - 1); 
    if ((ex << hue_ex_shift) & hue_reverse)
      strncat(cbuf[i], term_rev, bufsize - 1);
  }
  
  for (int i = 0; i < hue_count; i++)
    hue[i] = cbuf[i];

#undef bufsize
#undef build_color
}

// vim: ts=2 sw=2 et
