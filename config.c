/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "config.h"
#include <getopt.h>

#include "terminfo.h"

struct config_t config;

char* parse_options(int argc, char **argv)
{
  static const struct option gopts[]=
  {
    { "debug",      0, 0, 'D' },
    { "deep",       0, 0, 'd' },
    { "entry-only", 0, 0, 'e' },
    { "ui",         0, 0, 'i' },
    { "help",       0, 0, 'h' },
    { "quick",      0, 0, 'Q' },
    { "raw",        0, 0, 'R' },
    { "version",    0, 0, 'v' },
    { NULL,         0, 0, '\0' }
  };

  memset(&config, sizeof(config), 0);

  assert(argc>0 && argv!=NULL && *argv!=NULL && **argv!='\0');
  char* tmp;
  tmp=*argv;
  while (*tmp) tmp++;
  tmp--;
  if (*tmp=='i')
    config.conf_ui = true;

  if (is_term)
    config.conf_color = true;
 
  while (true)
  {
    int i = 0;
    int c = getopt_long(argc, argv, "dehivDQRT", gopts, &i);
    if (c < 0)
      break;
    if (c == 0)
      c = gopts[i].val;
    switch (c)
    {
    case 'd':
      config.conf_deep = true;
      break;
    case 'e':
      config.conf_entry_only = true;
      break;
    case 'h':
      config.action = action_help;
      break;
    case 'i':
      config.conf_ui = !config.conf_ui;
      break;
    case 'v':
      config.action = action_version;
      break;
    case 'D':
      config.conf_debug = true;
      break;
    case 'Q':
      config.conf_quick = true;
      break;
    case 'R':
      config.conf_raw = true;
      break;
    case 'T':
      config.conf_tabi = true;
      break;
    }
  }
  return
    (optind<=argc)?argv[optind]:NULL;
}

// vim: ts=2 sw=2 et
