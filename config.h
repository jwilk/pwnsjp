/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"

#ifndef CONFIG_H
#define CONFIG_H

struct config_t
{
  enum
  {
    action_seek = 0,
    action_help,
    action_version
  } action;
  char *filename;
  bool conf_color;
  bool conf_debug;
  bool conf_deep;
  bool conf_entry_only;
  bool conf_quick;
  bool conf_raw;
  bool conf_tabi;
  bool conf_ui;
};

char* parse_options(int, char**);
extern struct config_t config;

#endif

// vim: ts=2 sw=2 et
