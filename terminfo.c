/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "terminfo.h"

#include <unistd.h>
#include <ncursesw/term.h>

bool is_term = false;

char* term_setaf[8];
char* term_setab[8];
char* term_sgr0;
char* term_bold;

static char* term_getstr(const char *str)
{
  char* result = tigetstr((char*) str);
  if (result == NULL || result == (char*)-1)
    return "";
  else
    return result;
}

void term_quit(void)
{
  int j;
  free(term_sgr0);
  free(term_bold);
  for (j=0; j<8; j++)
    free(term_setaf[j]);
  for (j=0; j<8; j++)
    free(term_setab[j]);
}

void term_init(void)
{
  char* s0;
  int err, j;

  if (!isatty(STDOUT_FILENO) || setupterm(NULL, STDOUT_FILENO, &err) != 0)
  {
    term_sgr0 = strdup("");
    term_bold = strdup("");
    for (j=0; j<8; j++)
      term_setaf[j] = strdup("");
    for (j=0; j<8; j++)
      term_setab[j] = strdup("");
    return;
  }
  
  is_term = true;
  
  term_sgr0 = strdup(term_getstr("sgr0"));
  term_bold = strdup(term_getstr("bold"));
  
  memset(term_setaf, 0, sizeof(term_setaf));
  memset(term_setab, 0, sizeof(term_setab));
  
  s0 = term_getstr("setaf");
  if (*s0 != '\0')
  for (j=0; j<8; j++)
  {
    term_setaf[j] = tparm(s0, j);
    term_setaf[j] = strdup(term_setaf[j]==NULL?"":term_setaf[j]);
  }
  
  s0 = term_getstr("setab");
  if (*s0 != '\0')
  for (j=0; j<8; j++)
  {
    term_setab[j] = tparm(s0, j);
    term_setab[j] = strdup(term_setab[j]==NULL?"":term_setab[j]);
  }
  atexit(term_quit);
}

// vim: ts=2 sw=2 et
