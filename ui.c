/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "ui.h"

#include <signal.h>

#define _XOPEN_SOURCE_EXTENDED 
#include <ncursesw/ncurses.h>

#include "html.h"
#include "hue.h"
#include "io.h"
#include "terminfo.h"
#include "unicode.h"

#define w_count 5
#define wmenu   windows[0]
#define wview   windows[1]
#define wtitle  windows[2]
#define wscroll windows[3]
#define wstatus windows[4]

static WINDOW* windows[w_count];

static int attrs[HUE_count];

#define c_menu_width 24
#define c_search_limit 18

void ui_show_statusline(bool activemenu)
{
  wattrset(wstatus, attrs[HUE_default]);
  mvwaddch(wstatus, 0, c_menu_width+2, activemenu ? ACS_LRCORNER : ACS_LLCORNER);
  wattrset(wstatus, attrs[activemenu ? HUE_default : HUE_dimmed]);
  mvwhline(wstatus, 0, 0, ACS_HLINE, c_menu_width+2);
  wattrset(wstatus, attrs[!activemenu ? HUE_default : HUE_dimmed]);
  mvwhline(wstatus, 0, c_menu_width+3, ACS_HLINE, COLS-c_menu_width-3);
  wnoutrefresh(wstatus);
}

struct scrollbar_t
{
  WINDOW* window;
  unsigned int range;
  unsigned int start;
  unsigned int stop;
  unsigned int height;
  bool disabled;
};

static void ui_show_scrollbar(struct scrollbar_t *scrollbar)
{
  if (scrollbar->disabled)
    return;
  unsigned int height = scrollbar->height;
  wattrset(scrollbar->window, attrs[HUE_default]);
  mvwvline(scrollbar->window, 0, 0, ACS_VLINE, height);
  if (scrollbar->range > 0)
  {
    height--;
    unsigned int range = scrollbar->range;
    unsigned int start = (range/2 + height*scrollbar->start)/range;
//               start = round ( height * start/range )
    unsigned int stop = (range/2 + height*scrollbar->stop)/range;
//               stop = round ( height * stop/range )
    mvwvline(scrollbar->window, start, 0, ACS_CKBOARD, stop-start+1);
  }
  wnoutrefresh(scrollbar->window);
}

#define COLOR_DEFAULT (-1)

inline bool ui_prepare(void)
{
  if (!is_term)
    return false; // FIXME

  initscr();
  halfdelay(1); raw();
  noecho(); nonl();
  intrflush(NULL, FALSE);
  keypad(stdscr, TRUE);
 
  curs_set(0);
  
  start_color();
  use_default_colors();
#define build_attr(k, f, g, a) \
  do { \
    init_pair(HUE_##k, COLOR_##f, COLOR_##g); attrs[HUE_##k] = COLOR_PAIR(HUE_##k) | (a); \
  } while(false)
  build_attr(tluafed, WHITE, DEFAULT, 0);
  attrs[HUE_misc] = attrs[HUE_default];
  attrs[HUE_bold] = attrs[HUE_default] | A_BOLD;
  attrs[HUE_reverse] = attrs[HUE_default] | A_REVERSE;
  build_attr(title, WHITE, BLUE, 0);
  attrs[HUE_boldtitle] = attrs[HUE_title] | A_BOLD;
  build_attr(highlight, WHITE, MAGENTA, A_BOLD);
  build_attr(hyperlink, CYAN, DEFAULT, 0);
  build_attr(italic, RED, DEFAULT, 0);
  attrs[HUE_phraze] = attrs[HUE_italic] | A_BOLD;
  build_attr(dimmed, BLACK, DEFAULT, A_BOLD);
#undef build_color
  
  wnoutrefresh(stdscr);
 
  wtitle = newwin(1, 0, 0, 0);
  wattrset(wtitle, attrs[HUE_reverse]);
  mvwhline(wtitle, 0, 0, ' ', COLS);
  mvwaddstr(wtitle, 0, 1, "pwnsjp-interactive " K_VERSION);
  
  wscroll = newwin(LINES-2, 1, 1, c_menu_width+2);
  struct scrollbar_t scrollbar;
  memset(&scrollbar, 0, sizeof(scrollbar));
  scrollbar.window = wscroll;
  scrollbar.height = LINES-2;
  ui_show_scrollbar(&scrollbar);
  
  wmenu = newwin(LINES-3, c_menu_width, 2, 1);

  wview = newwin(LINES-3, COLS-c_menu_width-5, 2, c_menu_width+4);
  char* message = ustr_to_str(L"Prosz\u0119 czeka\u0107, trwa budowanie indeksu...");
  mvwaddstr(wview, 0, 0, message);
  free(message);
 
  wstatus = newwin(0, 0, LINES-1, 0);
  ui_show_statusline(true);
  
  unsigned int i;
  for (i=0; i<w_count; i++)
    wnoutrefresh(windows[i]);
  doupdate();
  
  return true;
}

struct view_t;

struct menu_t
{
  struct io_t* io;
  struct view_t* view;
  struct scrollbar_t scrollbar;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
  int entry_page_no;
  wchar_t search[c_search_limit];
  unsigned int search_len;
  unsigned int search_pos;
};

struct view_t
{
  struct io_t* io;
  struct menu_t* menu;
  struct scrollbar_t scrollbar;
  char* content;
  bool content_needfree;
  bool raw;
  int position;
  unsigned int lines;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
};

static inline bool ui_search(struct menu_t *menu)
{
  assert(menu != NULL);
  assert(menu->io != NULL);

  unsigned int prev = menu->entry_no;
  char* search = ustr_to_str(menu->search);
  menu->entry_no = io_locate(menu->io, search);
  free(search);
  if (prev == menu->entry_no)
    return false;
  if 
    ( menu->entry_no >= menu->entry_page_no + menu->height ||
      menu->entry_no < (unsigned int)menu->entry_page_no )
    menu->entry_page_no = menu->entry_no;
  return true;
}

static void ui_hide_menu_cursor(void)
{
  curs_set(0);
}

static void ui_show_menu_cursor(struct menu_t *menu)
{
  curs_set(1);
  wmove(wmenu, 0, 1+menu->search_pos);
  wnoutrefresh(wmenu);
}

static void ui_show_menu(struct menu_t *menu)
{
  unsigned int i, j;
  bool almostfound;
  
  assert(menu != NULL);
  assert(menu->io != NULL);
  
  wchar_t* hide = str_to_ustr(menu->io->iitems[menu->entry_no].entry);
  wchar_t* seek = menu->search;
  i = wcslen(hide);
  j = wcslen(seek);
  if (i < j)
    almostfound = false;
  else
  {
    hide[j] = L'\0';
    almostfound = wcscoll(hide, seek) == 0;
  }
  free(hide);

  werase(wmenu);
  wattrset(wmenu, almostfound ? A_BOLD : A_NORMAL);
  waddch(wmenu, '[');
  mvwhline(wmenu, 0, 1, '_', menu->width-2);
  mvwaddch(wmenu, 0, menu->width-1, ']');
  mvwaddwstr(wmenu, 0, 1, menu->search);
  if (menu->entry_no >= menu->entry_page_no + menu->height)
    menu->entry_page_no += menu->height;
  if ((unsigned int)(menu->entry_page_no + menu->height) > menu->io->isize)
    menu->entry_page_no = menu->io->isize - menu->height;
  if (menu->entry_page_no >= 0 && menu->entry_no < (unsigned int)menu->entry_page_no)
    menu->entry_page_no -= menu->height;
  if (menu->entry_page_no < 0)
    menu->entry_page_no = 0;
  for (i=0, j=menu->entry_page_no; i<menu->height; i++, j++)
  {
    wattrset(wmenu, j==menu->entry_no ? A_REVERSE : A_NORMAL);
    mvwhline(wmenu, i+1, 0, ' ', menu->width);
    mvwaddnstr(wmenu, i+1, 1, menu->io->iitems[j].entry, menu->width-2);
  }
  wattrset(wmenu, A_NORMAL);

  menu->scrollbar.start = menu->scrollbar.stop = menu->entry_no;
  ui_show_scrollbar(&menu->scrollbar);
  
  ui_show_menu_cursor(menu);
}

static void ui_show_content(struct view_t *view)
{
  wattrset(wview, A_NORMAL);
  werase(wview);

  bool needrestart = false;
  
  if (view->content == NULL || view->entry_no != view->menu->entry_no)
  {
    if (view->content_needfree && view->content != NULL)
      free(view->content);
    view->entry_no = view->menu->entry_no;
    io_read(view->io, view->entry_no);
    if (view->raw)
    {
      view->content_needfree = false;
      view->content = view->io->cbuffer;
      view->lines = (strlen(view->content) + view->width - 1) / view->width;
      view->position = 0;
    }
    else
    {
      view->content_needfree = true;
      view->content = html_strip(view->io->cbuffer);
      view->lines = 0;
      view->position = 0;
      needrestart = true;
    }
  }
  else
  {
    if 
      ( view->position + view->height > 0 &&
        (unsigned int)(view->position + view->height) > view->lines 
      )
      view->position = view->lines - view->height;
    if (view->position < 0)
      view->position = 0;
    
    assert(view->position >= 0);
    assert(view->height > view->lines || view->position + view->height <= view->lines);
  }
  
  int lines = 1;
 
  if (view->raw)
  {
    int attr = A_BOLD;
    unsigned char* s = view->content + view->position*view->width;
    unsigned int x, lim;
    lim = view->height*view->width;
    for (x=0; x<lim && *s; x++, s++)
    {
      chtype ch;
      if (*s == '<') 
        attr = A_NORMAL; 
      if (*s < ' ')
        ch = '$' | A_REVERSE;
      else if (*s >= 0x7f)
        ch = '.' | A_REVERSE;
      else
        ch = *s;
      waddch(wview, ch | attr);
      if (*s == '>')
        attr = A_BOLD;
    }
  }
  else
  {
    char *left, *right;
    
    int xlimit, y, ylimit;
    xlimit = view->width;
    ylimit = needrestart ? INT_MAX : view->height;
    y = 0;

    left = right = view->content;
    bool esc = false;

#define canwrite() (y >= view->position)
#define flush() \
  do { \
    int len = right-left; \
    int width = strnwidth(left, len); \
    if ((width >= xlimit) || (width < 3 && xlimit < 7)) cr(); \
    xlimit -= width; \
    if (canwrite()) \
      waddnstr(wview, left, len); \
    if (xlimit <= 0) cr(); \
    left = right + 1; \
  } while (false)
#define cr() \
  do { \
    lines++; \
    xlimit = view->width; \
    if (canwrite()) \
    { \
      if (--ylimit <= 0) goto exceed; \
      waddch(wview, '\n'); \
    } \
    y++; \
  } while (false)
  
    while (*right)
    {
      if (esc)
      {
        assert(*right >= '0' && *right < 'z');
        wattrset(wview, attrs[*right - '0']);
        esc = false;
      }
      else 
      switch (*right)
      {
      case '\x1b':
        esc = true;
        flush();
        left++;
        break;
      case ' ':
        flush();
        if (xlimit == 0)
          break;
        if (xlimit > 2)
        {
          xlimit--;
          if (canwrite())
            waddch(wview, ' ');
        } 
        else
          cr();
        break;
      case '\n':
        flush();
        cr();
        break;
      default:
        ;
      }
      right++;
    }
exceed:
    if (ylimit > 0)
      flush();
    if (xlimit > 0)
      lines++;
#undef canwrite
#undef flush
#undef cr

    if (needrestart)
    {
      view->lines = lines;
      return ui_show_content(view);
    }
  }
  
  wnoutrefresh(wview);

  if (view->height > view->lines)
    view->scrollbar.range = 0;
  else
    view->scrollbar.range = view->lines;
  view->scrollbar.start = view->position;
  view->scrollbar.stop = view->position + view->height;
  ui_show_scrollbar(&view->scrollbar);
}

static void ui_react_menu(struct menu_t *menu, wchar_t ch)
{
  assert(menu != NULL);
  
#define reject(p) \
  do if (p) { beep(); return; } while(false)

  unsigned int i;
  
  switch (ch)
  {
  case KEY_DC:
    reject(menu->search_pos >= menu->search_len);
    menu->search_pos++;
  case -L'\b':
  case -L'\x7f':
  case KEY_BACKSPACE:
    reject(menu->search_len == 0 || menu->search_pos == 0);
    for (i=menu->search_pos-1; i<menu->search_len; i++)
      menu->search[i]=menu->search[i+1];
    menu->search_len--;
    menu->search_pos--;
    if (ui_search(menu))
      ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
    break;
  case L'@'-L'A': // Control + A
    reject(menu->search_pos == 0);
    menu->search_pos = 0;
    ui_show_menu(menu);
    doupdate();
    break;
  case L'@'-L'E': // Control + E
    reject(menu->search_pos >= menu->search_len);
    menu->search_pos = menu->search_len;
    ui_show_menu(menu);
    doupdate();
    break;
  case KEY_RIGHT:
    reject(menu->search_pos >= menu->search_len);
    menu->search_pos += 2;
  case KEY_LEFT:
    reject(menu->search_pos == 0);
    menu->search_pos--;
    ui_show_menu(menu);
    doupdate();
    break;
  case KEY_HOME:
    reject(menu->entry_no == 0);
    menu->entry_page_no = menu->entry_no = 0;
    ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
    break;
  case KEY_END:
    reject(menu->entry_no == menu->io->isize-1);
    menu->entry_page_no = menu->entry_no = menu->io->isize-1;
    ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
    break;
  case KEY_UP:
    reject(menu->entry_no == 0);
    menu->entry_no -= 2;
  case KEY_DOWN:
    reject(menu->entry_no + 1 >= menu->io->isize);
    menu->entry_no++;
    ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
    break;
  case KEY_NPAGE:
    menu->entry_no += menu->height;
    if (menu->entry_no >= menu->io->isize)
    {
      menu->entry_no = menu->io->isize-1;
      beep();
    }
    ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
    break;
  case KEY_PPAGE:
    if (menu->entry_no < menu->height)
    {
      menu->entry_no = 0;
      beep();
    }
    else
      menu->entry_no -= menu->height;
    ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
    break;
  default:
    reject 
      ( (ch = -ch) < L'\0' ||
        !iswprint(ch) ||
        wcwidth(ch) != 1 ||
        menu->search_len >= c_search_limit );
    for (i=menu->search_len; i>menu->search_pos; i--)
      menu->search[i]=menu->search[i-1];
    menu->search[menu->search_pos] = ch;
    menu->search_len++;
    menu->search_pos++;
    if (ui_search(menu))
      ui_show_content(menu->view);
    ui_show_menu(menu);
    doupdate();
  }
#undef reject
}

static inline void ui_react_view(struct view_t *view, wchar_t ch)
{
  assert(view!=NULL);

  /* TODO: add some beeps */
  switch (ch)
  {
  case KEY_UP:
    view->position -= 2;
  case KEY_DOWN:
  case KEY_ENTER:
  case -L'\n':
  case -L'\r':
    view->position++;
    ui_show_content(view);
    doupdate();
    break;
  case KEY_HOME:
    view->position=0;
    ui_show_content(view);
    doupdate();
    break;
  case KEY_END:
    view->position = INT_MAX;
    ui_show_content(view);
    doupdate();
    break;
  case KEY_PPAGE:
    view->position -= 2*view->height;
  case KEY_NPAGE:
    view->position += view->height;
    ui_show_content(view);
    doupdate();
    break;
  case -L'\\':
    if (view->content_needfree && view->content != NULL)
      free(view->content);
    view->content = NULL;
    view->content_needfree = false;
    view->raw = !view->raw;
    ui_show_content(view);
    doupdate();
    break;
  default:
    ;
  }
  return;
}

void ui_start(struct io_t *io)
{
  assert(is_term);
  assert(io != NULL);

  bool activemenu = true;
  struct menu_t menu;
  struct view_t view;
  memset(&menu, 0, sizeof(menu));
  memset(&view, 0, sizeof(view));
  
  menu.io = io;
  menu.view = &view;
  menu.width = c_menu_width;
  menu.height = LINES - 4; // FIXME: this could be negative
  menu.scrollbar.height = LINES - 2;
  menu.scrollbar.range = io->isize-1;
  menu.scrollbar.window = wscroll;
  
  view.io = io;
  view.menu = &menu;
  view.width = COLS - c_menu_width - 5; // FIXME
  view.height = LINES - 3; // FIXME: this could be negative
  view.scrollbar.height = LINES - 2;
  view.scrollbar.window = wscroll;
  view.scrollbar.disabled = true;
  
  ui_show_content(&view);
  ui_show_menu(&menu);
  doupdate();
  
  bool doexit = false;
  
  while (!doexit)
  {
    wint_t chi;
    int r = get_wch(&chi);
    wchar_t ch = chi;
    switch (r)
    {
    case OK:
      if (ch > L'\0')
      {
        ch = -ch;
        break;
      }
    case ERR:
      ch = L'\0';
      break;
    default:
      ;
    }
    switch (ch)
    {
    case L'\0':
      break;
    case -L'\t':
      activemenu = !activemenu;
      menu.scrollbar.disabled = !activemenu;
      view.scrollbar.disabled = activemenu;
      ui_show_statusline(activemenu);
      if (activemenu)
      {
        ui_show_scrollbar(&menu.scrollbar);
        ui_show_menu_cursor(&menu);
      }
      else
      {
        ui_show_content(&view);
        ui_hide_menu_cursor();
      }
      doupdate();
      break;
    case -L'\x1b': // Escape
      r = get_wch(&chi);
      ch = chi;
      if (r != ERR && ch != L'\x1b')
        break;
    case L'@'-L'C': // Control + C
    case L'@'-L'\\': // Control + Backslash
      doexit = true;
      break;
    default:
      if (activemenu)
        ui_react_menu(&menu, ch);
      else
        ui_react_view(&view, ch);
    }
  }

  if (view.content_needfree)
    free(view.content);
}

void ui_stop(void)
{
  unsigned int i;
  for (i=0; i<=w_count; i++)
    delwin(windows[i]);
  erase();
  refresh();
  endwin();
}

// vim: ts=2 sw=2 et
