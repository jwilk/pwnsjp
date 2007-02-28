/* Copyright (c) 2005, 2006 Jakub Wilk
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
  
#define try(s) do { if (!(s)) fatal(""); } while(0)
#define tri(s, err) do { if (!(s)) fatal(err); } while(0)
  
  regex_t regex;
  tri ( regex_compile(&regex, pattern), "Invalid pattern" );
 
  struct io_t io;
  if (!io_init(&io, config.filename))
  {
    fprintf(stderr, "Unable to open data file: %s\n", config.filename);
    return EXIT_FAILURE;
  }
  
  if (!io_validate(&io))
  {
    fprintf(stderr, "Invalid data file signature.\n");
    return EXIT_FAILURE;
  }
  tri ( io.file_size > (1 << 24), "Unexpectedly short data file" );
  tri ( io.file_size < (1 << 28), "Unexpectedly long data file" );
  
  if (config.conf_ui && !ui_prepare())
  {
    fprintf(stderr, "Unable to initialize user interface.\n");
    return EXIT_FAILURE;
  }

  if (!io_prepare_index(&io))
  {
    fprintf(stderr, "Unable to prepare index.\n");
    return EXIT_FAILURE;
  }
  
  tri ( io.isize >= (1 << 12), "Indecently few words" );
  tri ( io.isize <= (1 << 17), "Indecently many words" );

  if (!io_build_index(&io))
  {
    fprintf(stderr, "Unable to build index.\n");
    return EXIT_FAILURE;
  }
 
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

  try ( io_fine(&io) );
  
  if (pattern)
    regex_free(&regex);
#undef try
#undef try_s

  return EXIT_SUCCESS;
}

// vim: ts=2 sw=2 et
