/* Copyright (c) 2005, 2006, 2007 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "memory.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "byteorder.h"
#include "config.h"
#include "html.h"
#include "hue.h"
#include "io.h"
#include "regex.h"
#include "terminfo.h"
#include "ui.h"
#include "unicode.h"

static void version(void)
{
  fprintf(stderr, "pwnsjp, version " K_VERSION "\n\n");
}

static void usage(void)
{
  fprintf(stderr,
    "Usage: pwnsjp [OPTIONS] PATTERN\n\n"
    "Options:\n"
    "  -f, --file=DATAFILE\n"
    "  -d, --deep\n"
    "  -e, --entry-only\n"
    "  -i, --ui\n"
    "  -h, --help\n"
    "  -v, --version\n\n");
}

int main(int argc, char **argv)
{
  term_init();

  char *pattern = parse_options(argc, argv);
  switch (config.action)
  {
  case action_help:
    usage();
    return EXIT_SUCCESS;
  case action_version:
    version();
    return EXIT_SUCCESS;
  case action_seek:
    ;
  }
  
  unicode_init();

#define fail(format, ...) \
  do { fprintf(stderr, format, ## __VA_ARGS__); return EXIT_FAILURE; } while (0)
  
  regex_t regex;
  if (!regex_compile(&regex, pattern))
    fail("Invalid pattern.\n");

  struct io_t io;
  if (!io_init(&io, config.filename))
    fail("Unable to open data file: %s\n", config.filename);
  
  if (!io_validate(&io))
    fail("Invalid data file signature.\n");

  if (io.file_size < (1 << 24))
    fail("Unexpectedly short data file.\n");
  
  if (io.file_size > (1 << 28))
    fail("Unexpectedly long data file.\n");
  
  if (config.conf_ui && !ui_prepare())
    fail("Unable to initialize user interface.\n");

  if (!io_prepare_index(&io))
    fail("Unable to prepare index.\n");
  
  if (io.isize < (1 << 12))
    fail("Indecently few words.\n");
  if (io.isize > (1 << 17))
    fail("Indecently many words.\n");

  if (!io_build_index(&io))
    fail("Unable to build index.\n");

#undef fail

  if (config.conf_tabi)
    ; // we do nothing
  else if (config.conf_ui)
  {
    hue_setup_curses();
    ui_start(&io);
  }
  else
  {
    hue_setup_terminfo();

    unsigned int i = 0;
    char *bspattern = NULL;
    if (!config.conf_deep)
    {
      bspattern = pattern_head(pattern);
      if (bspattern)
        i = io_locate(&io, bspattern);
    }

    struct io_iitem_t *iitem;
    unsigned int pmc = 0;
    for (iitem = io.iitems + i; i<io.isize; i++, iitem++)
    {
      char *tbuffer;
      bool doesmatch = true;
      if (config.conf_deep || !pattern || (doesmatch = regex_match(&regex, iitem->entry)))
      {
        debug(
          "localstr = \"%s\" ; location = %08x + %06x\n", 
          iitem->entry, 
          iitem->offset,
          iitem->size);
        bool dofree = false;
        if (config.conf_entry_only)
          tbuffer = iitem->entry;
        else 
        {
          io_read(&io, i);
          tbuffer = io.cbuffer;
          if (!config.conf_raw)
          {
            tbuffer = html_strip(tbuffer);
            dofree = true;
          }
        }
        if (!config.conf_deep || !pattern || regex_match(&regex, tbuffer))
        {
          pmc++;
          if (!config.conf_entry_only)
            printf("%s\n::: %s%s%s :::%s\n\n", 
              hue[hue_title], 
              hue[hue_title + hue_bold], 
              iitem->entry, 
              hue[hue_title], 
              hue[hue_normal]);
          printf("%s\n", tbuffer);
        }
        if (dofree)
          free(tbuffer);
      }
#if 0
      else if (!doesmatch && bspattern != NULL && pmc > 0)
        break;
#endif
    }
    if (bspattern != NULL)
      free(bspattern);
    debug("number of matches = %u\n", pmc);
  }

  if (!io_fine(&io))
    fatal("");
  
  if (pattern)
    regex_free(&regex);

  return EXIT_SUCCESS;
}

// vim: ts=2 sw=2 et
