#include "common.h"
#include "ui.h"

#include <ctype.h>
#include <curses.h>

#include "io.h"
#include "hue.h"
#include "html.h"
#include "terminfo.h"

static WINDOW *wscreen, *wmenu, *wgeneral;

static int attrs[HUE_count];

#define c_menu_width 24
#define c_search_limit 18

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
#undef build_color
  
  wattrset(wscreen, attrs[HUE_reverse]);
  int i;
  for (i=0; i<COLS; i++)
    waddch(wscreen, ' ');
  mvwaddstr(wscreen, 0, 1, "pwnsjp-interactive " K_VERSION);
  wattrset(wscreen, attrs[HUE_default]);
  mvwvline(wscreen, 1, c_menu_width+2, ACS_VLINE, LINES-1);
  wnoutrefresh(wscreen);
  
  wmenu = newwin(0, c_menu_width, 2, 1);
  keypad(wmenu, TRUE);
  wnoutrefresh(wmenu);

  wgeneral = newwin(0, 0, 2, c_menu_width+4);
  mvwaddstr(wgeneral, 0, 0, "Proszê czekaæ, trwa budowanie indeksu..."); // FIXME: locale dependent
  wnoutrefresh(wgeneral);

  doupdate();
  
  return true;
}

#define KEY_ESC 0x1b

struct menu_t
{
  struct io_t* io;
  unsigned int width;
  unsigned int height;
  unsigned int entry_no;
  int entry_page_no;
  char search[c_search_limit];
  unsigned int search_len;
  unsigned int search_pos;
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

static void ui_show_content(struct menu_t *menu)
{
  io_read(menu->io, menu->entry_no);
  char* result = html_strip(menu->io->cbuffer);
  int y, x;
  getyx(wmenu, y, x);
  
  wattrset(wgeneral, A_NORMAL);
  werase(wgeneral);
  wmove(wgeneral, 0, 0);
  bool esc = false;

  char* t = result;
  while (*t)
  {
    if (esc)
    {
      wattrset(wgeneral, attrs[*t - '0']);
      esc = false;
    }
    else if (*t == '\x1b')
      esc = true;
    else
      waddnstr(wgeneral, t, 1);
    t++;
  }
  
  wmove(wmenu, y, x);
  
  wnoutrefresh(wgeneral);
  wnoutrefresh(wscreen);

  free(result);
}

void ui_start(struct io_t *io)
{
  assert(is_term);
  assert(wscreen!=NULL && wmenu!=NULL && wgeneral!=NULL);
  assert(io!=NULL);
  
  werase(wgeneral);
  wrefresh(wgeneral);

  struct menu_t menu;
  memset(&menu, 0, sizeof(menu));
  menu.io = io;
  menu.width = c_menu_width;
  menu.height = LINES - 4; // FIXME: this could be negative
  
  ui_show_content(&menu);
  ui_redraw_menu(&menu);
  doupdate();
  
  while (true)
  {
    int ch;
    ch = wgetch(wmenu);
    switch (ch)
    {
    case KEY_DC:
      if (menu.search_pos >= menu.search_len)
      {
        beep();
        break;
      }
      menu.search_pos++;
    case KEY_BACKSPACE:
      if (menu.search_len > 0 && menu.search_pos > 0)
      {
        unsigned int i;
        for (i=menu.search_pos-1; i<menu.search_len; i++)
          menu.search[i]=menu.search[i+1];
        menu.search_len--;
        menu.search_pos--;
        if (ui_search(&menu))
          ui_show_content(&menu);
        ui_redraw_menu(&menu);
        doupdate();
      }
      else
        beep();
      break;
    case KEY_ESC:
      ch = wgetch(wmenu);
      if (ch == KEY_ESC || ch == ERR)
        return;
      break;
    case KEY_RIGHT:
      if (menu.search_pos >= menu.search_len)
      {
        beep();
        break;
      }
      menu.search_pos += 2;
      ui_redraw_menu(&menu);
      doupdate();
    case KEY_LEFT:
      if (menu.search_pos <= 0)
        beep();
      else
      {
        menu.search_pos--;
        ui_redraw_menu(&menu);
        doupdate();
      }
      break;
    case KEY_UP:
      if (menu.entry_no > 0)
        menu.entry_no -= 2;
      else
      {
        beep();
        break;
      }
    case KEY_DOWN:
      if (menu.entry_no + 1 >= io->isize)
        beep();
      else
      {
        menu.entry_no++;
        ui_redraw_menu(&menu);
        ui_show_content(&menu);
        doupdate();
      }
      break;
    case KEY_NPAGE:
      menu.entry_no += menu.height;
      if (menu.entry_no >= io->isize)
      {
        menu.entry_no = io->isize-1;
        beep();
      }
      ui_redraw_menu(&menu);
      ui_show_content(&menu);
      doupdate();
      break;
    case KEY_PPAGE:
      if (menu.entry_no < menu.height)
      {
        menu.entry_no = 0;
        beep();
      }
      else
        menu.entry_no -= menu.height;
      ui_redraw_menu(&menu);
      ui_show_content(&menu);
      doupdate();
      break;
    case '\r':
      ui_show_content(&menu);
      doupdate();
    case ERR:
      break;
    default:
      if (isprint(ch) && (unsigned int)ch < 0x100)
      {
        if (menu.search_len >= c_search_limit)
          beep();
        else
        {
          unsigned int i;
          for (i=menu.search_len; i>menu.search_pos; i--)
            menu.search[i]=menu.search[i-1];
          menu.search[menu.search_pos] = ch;
          menu.search_len++;
          menu.search_pos++;
          if (ui_search(&menu))
            ui_show_content(&menu);
          ui_redraw_menu(&menu);
          doupdate();
        }
      }
    }
  }
}

void ui_stop(void)
{
  assert(wscreen!=NULL && wmenu!=NULL && wgeneral!=NULL);
  delwin(wmenu);
  delwin(wgeneral);
  werase(wscreen);
  wrefresh(wscreen);
  endwin();
}

// vim: ts=2 sw=2 et
