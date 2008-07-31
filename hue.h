/* Copyright © 2005, 2006 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#ifndef HUE_H
#define HUE_H

enum hue_fg
{
  hue_fg_black      = 0x000,
  hue_fg_red        = 0x001,
  hue_fg_green      = 0x002,
  hue_fg_brown      = 0x003,
  hue_fg_blue       = 0x004,
  hue_fg_magenta    = 0x005,
  hue_fg_cyan       = 0x006,
  hue_fg_white      = 0x007
};
#define hue_fg_count 8
#define hue_fg_mask 0x00f
#define hue_fg_shift 0

enum hue_bg
{
  hue_bg_black   = 0x000,
  hue_bg_red     = 0x010,
  hue_bg_green   = 0x020,
  hue_bg_brown   = 0x030,
  hue_bg_blue    = 0x040,
  hue_bg_magenta = 0x050,
  hue_bg_cyan    = 0x060,
  hue_bg_white   = 0x070
};
#define hue_bg_count 8
#define hue_bg_mask 0x0f0
#define hue_bg_shift 4

enum hue_ex
{
  hue_bold       = 0x100,
  hue_reverse    = 0x200,
  hue_persistent = 0x400
};
#define hue_ex_count 8
#define hue_ex_mask 0xf00
#define hue_ex_shift 8

#define hue_count (hue_ex_count << hue_ex_shift)

#define hue_none (hue_count - 1)

#define hue_normal    (hue_fg_white   | hue_bg_black)
#define hue_source    (hue_fg_black   | hue_bg_black | hue_bold)
#define hue_title     (hue_fg_white   | hue_bg_blue)
#define hue_highlight (hue_fg_blue    | hue_bg_black | hue_bold)
#define hue_hyperlink (hue_fg_magenta | hue_bg_black | hue_bold)
#define hue_italic    (hue_fg_cyan    | hue_bg_black)
#define hue_phrase    (hue_fg_red     | hue_bg_black | hue_bold)
#define hue_phrase2   (hue_fg_green   | hue_bg_black)
#define hue_dimmed    (hue_fg_black   | hue_bg_black | hue_bold)

#define hue_source_p   (hue_source + hue_persistent)
#define hue_normal_rev (hue_normal + hue_reverse)



int hue_blend(int hue, int new_fg, int new_bg, int new_ext);

extern char *hue[hue_count];

void hue_setup_curses(void);
void hue_setup_terminfo(void);

#endif

// vim: ts=2 sw=2 et
