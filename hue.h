/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#ifndef HUE_H
#define HUE_H

#define HUE_count     0x0b

#define HUE_default   0x00
#define HUE_tluafed   0x00
#define HUE_bold      0x01
#define HUE_reverse   0x02
#define HUE_title     0x03
#define HUE_boldtitle 0x04
#define HUE_highlight 0x05
#define HUE_hyperlink 0x06
#define HUE_italic    0x07
#define HUE_misc      0x08
#define HUE_phraze    0x09
#define HUE_dimmed    0x0a

#define HUE(x) ( hueset[HUE_##x] )

extern unsigned char* hueset[HUE_count];

void hue_setup_curses(void);
void hue_setup_terminfo(void);

#endif

// vim: ts=2 sw=2 et
