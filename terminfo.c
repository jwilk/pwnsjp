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

unsigned char* term_setaf[8];
unsigned char* term_setab[8];
unsigned char* term_sgr0;
unsigned char* term_bold;

static unsigned char* term_getstr(const unsigned char *str)
{
  unsigned char* result = tigetstr((char*) str);
  if (result == NULL || result == (unsigned char*)-1)
    return "";
  else
    return result;
}

void term_quit(void)
{
  unsigned int j;
  free(term_sgr0);
  free(term_bold);
  for (j=0; j<8; j++)
    free(term_setaf[j]);
  for (j=0; j<8; j++)
    free(term_setab[j]);
}

void term_init(void)
{
  unsigned char *s0;
  int err;
  unsigned int j;

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
  assert(term_sgr0 != NULL);

  term_bold = strdup(term_getstr("bold"));
  assert(term_bold != NULL);
  
  memset(term_setaf, 0, sizeof(term_setaf));
  memset(term_setab, 0, sizeof(term_setab));
  
  s0 = term_getstr("setaf");
  if (*s0 != '\0')
    for (j=0; j<8; j++)
    {
      term_setaf[j] = tparm(s0, j);
      term_setaf[j] = strdup(term_setaf[j]==NULL ? (unsigned char*)"" : term_setaf[j]);
      if (term_setaf[j] == NULL)
        term_setaf[j] = strdup("");
    }
  else
    for (j=0; j<8; j++)
      term_setaf[j] = strdup("");
  
  s0 = term_getstr("setab");
  if (*s0 != '\0')
    for (j=0; j<8; j++)
    {
      term_setab[j] = tparm(s0, j);
      term_setab[j] = strdup(term_setab[j]==NULL ? (unsigned char*)"" : term_setab[j]);
      if (term_setab[j] == NULL)
        term_setab[j] = strdup("");
    }
  else
    for (j=0; j<8; j++)
      term_setab[j] = strdup("");

  atexit(term_quit);
}

// vim: ts=2 sw=2 et
