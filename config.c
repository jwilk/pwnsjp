/* Copyright (c) 2005, 2006 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "config.h"
#include <getopt.h>

#include "terminfo.h"
#include "memory.h"

struct config_t config;

static void deconfigure(void)
{
  free(config.filename);
}

char *parse_options(int argc, char **argv)
{
  static const struct option gopts[] =
  {
    { .name = "deep",       .val = 'd' },
    { .name = "entry-only", .val = 'e' },
    { .name = "file",       .val = 'f', .has_arg = 1 },
    { .name = "help",       .val = 'h' },
    { .name = "ui",         .val = 'i' },
    { .name = "version",    .val = 'v' },
    { .name = "debug",      .val = 'D' }, // undocumented
    { .name = "quick",      .val = 'Q' }, // undocumented
    { .name = "raw",        .val = 'R' }, // undocumented
    { .name = NULL,         .val = '\0' }
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
    int c = getopt_long(argc, argv, "def:hivDQRT", gopts, &i);
    if (c < 0)
      break;
    if (c == 0)
      c = gopts[i].val;
 #define on(o) ( config.conf_##o = true )
    switch (c)
    {
    case 'f':
      config.filename = optarg;
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
    case 'd': on(deep);       break;
    case 'e': on(entry_only); break;
    case 'D': on(debug);      break;
    case 'Q': on(quick);      break;
    case 'R': on(raw);        break;
    case 'T': on(tabi);       break;
    }
#undef on
  }
  if (config.filename == NULL)
    config.filename = "slo.win";
  if (strchr(config.filename, '/') == NULL)
  {
    bool hasext = strchr(config.filename, '.') != NULL;
    int size = 1 + snprintf(NULL, 0, K_DATA_PATH "/%s%s", config.filename, hasext ? "" : ".win");
    char *buffer = alloz(size, 1);
    sprintf(buffer, K_DATA_PATH "/%s%s", config.filename, hasext ? "" : ".win");
    config.filename = buffer;
    atexit(deconfigure);
  }
  return
    (optind <= argc) ? argv[optind] : NULL;
}


// vim: ts=2 sw=2 et
