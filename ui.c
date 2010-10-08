/* Copyright © 2005, 2006, 2010 Jakub Wilk <jwilk@jwilk.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "common.h"
#include "ui.h"

#define _XOPEN_SOURCE_EXTENDED
#include <signal.h>
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

static WINDOW *windows[w_count];

static int attrkit[hue_count];

#define ATTR(h) attrkit[hue_##h]

#define c_menu_width 24
#define c_search_limit 18

static int scr_width = 0;
static int scr_height = 0;
static bool scr_needresize = false;

#define c_min_scr_width c_menu_width + 10
#define c_min_scr_height 12

static void ui_show_statusline(bool activemenu)
{
  wattrset(wstatus, ATTR(normal));
  mvwaddch(wstatus, 0, c_menu_width + 2, activemenu ? ACS_LRCORNER : ACS_LLCORNER);
  wattrset(wstatus, activemenu ? ATTR(normal) : ATTR(dimmed));
  mvwhline(wstatus, 0, 0, ACS_HLINE, c_menu_width + 2);
  wattrset(wstatus, !activemenu ? ATTR(normal) : ATTR(dimmed));
  mvwhline(wstatus, 0, c_menu_width + 3, ACS_HLINE, scr_width - c_menu_width - 3);
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
  wbkgd(scrollbar->window, ACS_VLINE | ATTR(normal)); werase(scrollbar->window);
  if (scrollbar->range > 0)
  {
    height--;
    unsigned int range = scrollbar->range;
    unsigned int start = (range/2 + height*scrollbar->start)/range;
//               start = round ( height * start/range )
    unsigned int stop = (range/2 + height*scrollbar->stop)/range;
//               stop = round ( height * stop/range )
    mvwvline(scrollbar->window, start, 0, ACS_CKBOARD | ATTR(normal), stop-start+1);
  }
  wnoutrefresh(scrollbar->window);
}

static void ui_windows_create(void)
{
  erase();
  wnoutrefresh(stdscr);

  getmaxyx(stdscr, scr_height, scr_width);
  assert(scr_width > 0 && scr_height > 0);
  bool object = false;
  if (scr_width  < c_min_scr_width ) { scr_width  = c_min_scr_width;  object = true; }
  if (scr_height < c_min_scr_height) { scr_height = c_min_scr_height; object = true; }
  if (object)
    resize_term(scr_height, scr_width);

  wtitle = newwin(1, 0, 0, 0);
  wbkgd(wtitle, ' ' | ATTR(normal_rev)); werase(wtitle);
  mvwaddstr(wtitle, 0, 1, "pwnsjp-interactive " K_VERSION);

  wscroll = newwin(scr_height - 2, 1, 1, c_menu_width + 2);
  struct scrollbar_t scrollbar;
  memset(&scrollbar, 0, sizeof(scrollbar));
  scrollbar.window = wscroll;
  scrollbar.height = scr_height - 2;
  ui_show_scrollbar(&scrollbar);

  wmenu = newwin(scr_height - 3, c_menu_width, 2, 1);
  wview = newwin(scr_height - 3, scr_width-c_menu_width-5, 2, c_menu_width+4);

  wstatus = newwin(0, 0, scr_height-1, 0);
  ui_show_statusline(true);
}

static void ui_windows_destroy(void)
{
  unsigned int i;
  for (i=0; i<w_count; i++)
  if (windows[i] != NULL)
    delwin(windows[i]);
}

static void ui_windows_refresh(void)
{
  unsigned int i;
  for (i = 0; i < w_count; i++)
    wnoutrefresh(windows[i]);
  doupdate();
}

static void ui_stop(void)
{
  static bool done = false;
  if (!done)
  {
    done = true;
    ui_windows_destroy();
    erase();
    refresh();
    endwin();
  }
}

static void ui_windows_recreate(void)
{
  scr_needresize = false;
  endwin();
  ui_windows_destroy();
  refresh();
  ui_windows_create();
  ui_windows_refresh();
}

static void ui_sig_resize()
{
  scr_needresize = true;
}

bool ui_prepare(void)
{
  if (!is_term)
    return false; // FIXME

  memset(windows, 0, sizeof(windows));

  initscr();
  atexit(ui_stop);
  raw(); noecho(); nonl();
  intrflush(NULL, FALSE);
  keypad(stdscr, TRUE);
  timeout(500);

  curs_set(0);

#define fail() do { ui_stop(); return false; } while (0)

  if (start_color() != OK)
    fail();

  int i = 0;
  for (int fg = 0; fg < hue_fg_count; fg++)
  for (int bg = 0; bg < hue_bg_count; bg++)
  {
    if (fg << hue_fg_shift == hue_fg_white && bg << hue_bg_shift == hue_bg_white)
      continue;
    i++;
    if (init_pair(i, fg, bg) != OK)
    {
      fprintf(stderr, "%d\n", i);
      fail();
    }
    for (int ex = 0; ex < hue_ex_count; ex++)
    {
      int j = (fg << hue_fg_shift) + (bg << hue_bg_shift) + (ex << hue_ex_shift);
      attrkit[j] = COLOR_PAIR(i);
      if ((ex << hue_ex_shift) & hue_bold)
        attrkit[j] |= A_BOLD;
      if ((ex << hue_ex_shift) & hue_reverse)
        attrkit[j] |= A_REVERSE;
    }
  }

  ui_windows_create();
  char *message = ustr_to_str(L"Prosz\u0119 czeka\u0107, trwa budowanie indeksu...");
  mvwaddstr(wview, 0, 0, message);
  free(message);

  ui_windows_refresh();

  struct sigaction sa;
  sa.sa_handler = ui_sig_resize;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGWINCH, &sa, NULL);

#undef fail

  return true;
}

struct view_t;

struct menu_t
{
  struct io_t *io;
  struct view_t *view;
  struct scrollbar_t scrollbar;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
  int entry_page_no;
  wchar_t search[c_search_limit+1];
  unsigned int search_len;
  unsigned int search_pos;
};

struct view_t
{
  struct io_t *io;
  struct menu_t* menu;
  struct scrollbar_t scrollbar;
  char *content;
  bool content_needfree;
  bool raw;
  int position;
  unsigned int lines;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
};

static bool ui_search(struct menu_t *menu)
{
  assert(menu != NULL);
  assert(menu->io != NULL);

  unsigned int prev = menu->entry_no;
  char *search = ustr_to_str(menu->search);
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
    char *str = menu->io->iitems[j].entry;
    wattrset(wmenu, j==menu->entry_no ? A_REVERSE : A_NORMAL);
    mvwhline(wmenu, i+1, 0, ' ', menu->width);
    mvwaddnstr(wmenu, i+1, 1, str, menu->width-2);
    if (strnwidth(str, -1) > menu->width-2)
      mvwaddch(wmenu, i+1, menu->width-1, '>');
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
    view->position = 0;
    if (view->raw)
    {
      view->content_needfree = false;
      view->content = view->io->cbuffer;
      view->lines = (strlen(view->content) + view->width - 1) / view->width;
    }
    else
    {
      view->content_needfree = true;
      view->content = html_strip(view->io->cbuffer);
      view->lines = 0;
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
    char *s = view->content + view->position*view->width;
    unsigned int x, lim;
    lim = view->height*view->width;
    for (x = 0; x < lim && *s; x++, s++)
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
        unsigned int hue;
        for (int i = 0; i < 3; i++)
          assert(right[i] >= '0' && right[i] < 'z');
        assert(right[3] == 'm');
        sscanf(right, "%x", &hue);
        wattrset(wview, attrkit[hue]);
        esc = false;
        right += 3;
        left = right + 1;
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

static void ui_react_view(struct view_t *view, wchar_t ch)
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

  assert(scr_width>0 && scr_height>0);

#define set_menu_extent() \
  do { \
    menu.width = c_menu_width; \
    menu.height = scr_height - 4; \
    menu.scrollbar.window = wscroll; \
    menu.scrollbar.height = scr_height - 2; \
  } while (0)

#define set_view_extent() \
  do { \
    view.width = scr_width - c_menu_width - 5; \
    view.height = scr_height - 3; \
    view.scrollbar.window = wscroll; \
    view.scrollbar.height = scr_height - 2; \
  } while (0)

  menu.io = io;
  menu.view = &view;
  set_menu_extent();
  menu.scrollbar.range = io->isize-1;

  view.io = io;
  view.menu = &menu;
  set_view_extent();
  view.scrollbar.disabled = true;

  ui_show_content(&view);
  ui_show_menu(&menu);
  doupdate();

  bool doexit = false;

  while (!doexit)
  {
    if (scr_needresize)
      unget_wch(L'L' - L'@'); // Control + L

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
    case L'@' - L'C': // Control + C
    case L'@' - L'\\': // Control + Backslash
      doexit = true;
      break;
    case L'@' - L'L': // Control + L
      ui_windows_recreate();
      set_menu_extent();
      menu.entry_page_no = menu.entry_no;
      set_view_extent();
      if (view.content_needfree)
      {
        view.content = NULL;
        free(view.content);
      }
      ui_show_content(&view);
      ui_show_menu(&menu);
      doupdate();
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

#undef set_menu_extent
#undef set_view_extent

}

// vim: ts=2 sw=2 et
