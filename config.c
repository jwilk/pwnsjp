/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

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

  assert(argc > 0 && argv != NULL && *argv != NULL && **argv != '\0');
  if (strchr(*argv, '\0')[-1] == 'i')
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
 #define on(o) ( config.conf_##o = true )
    switch (c)
    {
    case 'h':
      config.action = action_help;
      break;
    case 'i':
      config.conf_ui = !config.conf_ui;
      break;
    case 'v':
      config.action = action_version;
      break;
    case 'd': on(deep);       break;
    case 'e': on(entry_only); break;
    case 'D': on(debug);      break;
    case 'Q': on(quick);      break;
    case 'R': on(raw);        break;
    case 'T': on(tabi);       break;
    }
#undef on
  }
  return
    (optind <= argc) ? argv[optind] : NULL;
}

// vim: ts=2 sw=2 et
