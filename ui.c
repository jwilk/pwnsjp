#include "common.h"
#include "ui.h"

#include <ctype.h>
#include <curses.h>
#include <limits.h>

#include "io.h"
#include "hue.h"
#include "html.h"
#include "terminfo.h"

static WINDOW *wscreen, *wmenu, *wview;

static int attrs[HUE_count];

#define c_menu_width 24
#define c_search_limit 18

void ui_cursor(bool store)
{
  static int y, x;
  if (store)
    getsyx(y, x);
  else
    setsyx(y, x);
}

#define ui_store_cursor() \
  do ui_cursor(true); while(false)
#define ui_restore_cursor() \
  do ui_cursor(false); while(false)

void ui_redraw_statusline(bool activemenu)
{
  ui_store_cursor();
  wattrset(wscreen, attrs[HUE_default]);
  mvwaddch(wscreen, LINES-1, c_menu_width+2, activemenu ? ACS_LRCORNER : ACS_LLCORNER);
  wattrset(wscreen, attrs[activemenu ? HUE_default : HUE_dimmed]);
  mvwhline(wscreen, LINES-1, 0, ACS_HLINE, c_menu_width+2);
  wattrset(wscreen, attrs[!activemenu ? HUE_default : HUE_dimmed]);
  mvwhline(wscreen, LINES-1, c_menu_width+3, ACS_HLINE, COLS-c_menu_width-5);
  wnoutrefresh(wscreen);
  ui_restore_cursor();
}

bool ui_prepare(void)
{
  if (!is_term)
    return false; // FIXME
  wscreen = initscr();

  halfdelay(3);
  noecho();
  nonl();
  intrflush(NULL, FALSE);
  
  start_color();
#define build_attr(k, f, g, a) \
  do { \
    init_pair(HUE_##k, COLOR_##f, COLOR_##g); attrs[HUE_##k] = COLOR_PAIR(HUE_##k) | (a); \
  } while(false)
  
  build_attr(tluafed, WHITE, BLACK, 0);
  attrs[HUE_misc] = attrs[HUE_default];
  attrs[HUE_bold] = attrs[HUE_default] | A_BOLD;
  build_attr(reverse, BLACK, WHITE, 0);
  build_attr(title, WHITE, BLUE, 0);
  attrs[HUE_boldtitle] = attrs[HUE_title] | A_BOLD;
  build_attr(highlight, WHITE, MAGENTA, A_BOLD);
  build_attr(hyperlink, CYAN, BLACK, 0);
  build_attr(italic, RED, BLACK, 0);
  attrs[HUE_phraze] = attrs[HUE_italic] | A_BOLD;
  build_attr(dimmed, BLACK, BLACK, A_BOLD);
#undef build_color
  
  wattrset(wscreen, attrs[HUE_reverse]);
  mvwhline(wscreen, 0, 0, ' ', COLS);
  mvwaddstr(wscreen, 0, 1, "pwnsjp-interactive " K_VERSION);
  wattrset(wscreen, attrs[HUE_default]);
  mvwvline(wscreen, 1, c_menu_width+2, ACS_VLINE, LINES-2);
  keypad(wscreen, TRUE);
  
  wmenu = newwin(LINES-3, c_menu_width, 2, 1);

  wview = newwin(LINES-3, 0, 2, c_menu_width+4);
  mvwaddstr(wview, 0, 0, "Proszê czekaæ, trwa budowanie indeksu..."); // FIXME: locale dependent
  
  ui_redraw_statusline(true);
  
  wnoutrefresh(wscreen);
  wnoutrefresh(wmenu);
  wnoutrefresh(wview);

  doupdate();
  
  return true;
}

#define KEY_ESC 0x1b

struct view_t;

struct menu_t
{
  struct io_t* io;
  struct view_t* view;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
  int entry_page_no;
  char search[c_search_limit];
  unsigned int search_len;
  unsigned int search_pos;
};

struct view_t
{
  struct io_t* io;
  struct menu_t* menu;
  char* content;
  bool content_needfree;
  unsigned int lines;
  int position;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
};

static bool ui_search(struct menu_t *menu)
{
  unsigned int prev = menu->entry_no;
  menu->entry_no = io_locate(menu->io, menu->search);
  if (prev == menu->entry_no)
    return false;
  if 
    ( menu->entry_no >= menu->entry_page_no + menu->height ||
      menu->entry_no < (unsigned int)menu->entry_page_no )
    menu->entry_page_no = menu->entry_no;
  return true;
}

static void ui_redraw_menu(struct menu_t *menu)
{
  unsigned int i, j;

  werase(wmenu);
  wattrset(wmenu, A_BOLD);
  waddch(wmenu, '[');
  mvwhline(wmenu, 0, 1, '_', menu->width-2);
  mvwaddch(wmenu, 0, menu->width-1, ']');
  mvwaddstr(wmenu, 0, 1, menu->search);
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
  wmove(wmenu, 0, 1+menu->search_pos);
  wnoutrefresh(wmenu);
}

static void ui_show_content(struct view_t *view)
{
  ui_store_cursor();
  wattrset(wview, A_NORMAL);
  werase(wview);

  bool needrestart = false;
  int lines = 0;
  
  if (view->content == NULL || view->entry_no != view->menu->entry_no)
  {
    if (view->content_needfree && view->content != NULL)
      free(view->content);
    view->entry_no = view->menu->entry_no;
    io_read(view->io, view->entry_no);
    view->content_needfree = true;
    view->content = html_strip(view->io->cbuffer);
    view->lines = 0;
    view->position = 0;
    needrestart = true;
  }
  else
  {
    if (view->position < 0)
      view->position = 0;
    if 
      ( view->position + view->height > 0 &&
        (unsigned int)(view->position + view->height) > view->lines 
      )
      view->position = view->lines - view->height;
  }

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
    if (len > xlimit) cr(); \
    xlimit -= len; \
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
      ylimit--; \
      waddch(wview, '\n'); \
    } \
    y++; \
  } while (false)
  
  while (*right && ylimit > 0)
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
    case KEY_ESC:
      esc = true;
      flush();
      left++;
      break;
    case ' ':
      flush();
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
  flush();
#undef canwrite
#undef flush
#undef cr

  if (needrestart)
  {
    view->lines = lines;
    return ui_show_content(view);
  }

  wnoutrefresh(wscreen);
  wnoutrefresh(wview);

  ui_restore_cursor();
}

inline void ui_react_menu(struct menu_t *menu, int ch)
{
  switch (ch)
  {
  case KEY_DC:
    if (menu->search_pos >= menu->search_len)
    {
      beep();
      break;
    }
    menu->search_pos++;
  case '\b':
  case '\x7f':
  case KEY_BACKSPACE:
    if (menu->search_len > 0 && menu->search_pos > 0)
    {
      unsigned int i;
      for (i=menu->search_pos-1; i<menu->search_len; i++)
        menu->search[i]=menu->search[i+1];
      menu->search_len--;
      menu->search_pos--;
      if (ui_search(menu))
        ui_show_content(menu->view);
      ui_redraw_menu(menu);
      doupdate();
    }
    else
      beep();
    break;
  case KEY_RIGHT:
    if (menu->search_pos >= menu->search_len)
    {
      beep();
      break;
    }
    menu->search_pos += 2;
    ui_redraw_menu(menu);
    doupdate();
  case KEY_LEFT:
    if (menu->search_pos <= 0)
      beep();
    else
    {
      menu->search_pos--;
      ui_redraw_menu(menu);
      doupdate();
    }
    break;
  case KEY_UP:
    if (menu->entry_no > 0)
      menu->entry_no -= 2;
    else
    {
      beep();
      break;
    }
  case KEY_DOWN:
    if (menu->entry_no + 1 >= menu->io->isize)
      beep();
    else
    {
      menu->entry_no++;
      ui_show_content(menu->view);
      ui_redraw_menu(menu);
      doupdate();
    }
    break;
  case KEY_NPAGE:
    menu->entry_no += menu->height;
    if (menu->entry_no >= menu->io->isize)
    {
      menu->entry_no = menu->io->isize-1;
      beep();
    }
    ui_show_content(menu->view);
    ui_redraw_menu(menu);
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
    ui_redraw_menu(menu);
    doupdate();
    break;
  default:
    if (!isprint(ch) || (unsigned int)ch >= 0x100 || menu->search_len >= c_search_limit)
    {
      beep();
      break;
    }
    unsigned int i;
    for (i=menu->search_len; i>menu->search_pos; i--)
      menu->search[i]=menu->search[i-1];
    menu->search[menu->search_pos] = ch;
    menu->search_len++;
    menu->search_pos++;
    if (ui_search(menu))
      ui_show_content(menu->view);
    ui_redraw_menu(menu);
    doupdate();
  }
}

inline void ui_react_view(struct view_t *view, int ch)
{
  /* TODO: add some beeps */
  switch (ch)
  {
  case KEY_UP:
    view->position -= 2;
  case KEY_DOWN:
    view->position++;
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
  default:
    ;
  }
  return;
}

void ui_start(struct io_t *io)
{
  assert(is_term);
  assert(wscreen!=NULL && wmenu!=NULL && wview!=NULL);
  assert(io!=NULL);
  
  werase(wview);
  wrefresh(wview);

  bool activemenu = true;
  struct menu_t menu;
  struct view_t view;
  memset(&menu, 0, sizeof(menu));
  memset(&view, 0, sizeof(view));
  
  menu.io = io;
  menu.view = &view;
  menu.width = c_menu_width;
  menu.height = LINES - 4; // FIXME: this could be negative
  
  view.io = io;
  view.menu = &menu;
  view.width = COLS - c_menu_width - 5; // FIXME
  view.height = LINES - 3; // FIXME: this could be negative
  
  ui_show_content(&view);
  ui_redraw_menu(&menu);
  doupdate();
 
  bool doexit = false;
  
  while (!doexit)
  {
    int ch;
    ch = getch();
    switch (ch)
    {
    case ERR:
      break;
    case '\t':
      activemenu = !activemenu;
      ui_redraw_statusline(activemenu);
      doupdate();
      break;
    case KEY_ESC:
      ch = wgetch(wmenu);
      if (ch == KEY_ESC || ch == ERR)
        doexit = true;
      break;
    case KEY_ENTER:
    case '\n':
    case '\r':
      ui_show_content(&view);
      doupdate();
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
  assert(wscreen!=NULL && wmenu!=NULL && wview!=NULL);
  delwin(wmenu);
  delwin(wview);
  werase(wscreen);
  wrefresh(wscreen);
  endwin();
}

// vim: ts=2 sw=2 et
