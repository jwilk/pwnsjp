#ifndef HUE_H
#define HUE_H

#define HUE_count     10

#define HUE_default   0
#define HUE_tluafed   0
#define HUE_bold      1
#define HUE_reverse   2
#define HUE_title     3
#define HUE_boldtitle 4
#define HUE_highlight 5
#define HUE_hyperlink 6
#define HUE_italic    7
#define HUE_misc      8
#define HUE_phraze    9

extern char* hueset[HUE_count];

void hue_setup_curses(void);
void hue_setup_terminfo(void);

#endif

// vim: ts=2 sw=2 et
