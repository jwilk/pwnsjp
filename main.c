/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>

#include <regex.h>

#include "byteorder.h"
#include "config.h"
#include "html.h"
#include "hue.h"
#include "io.h"
#include "terminfo.h"
#include "ui.h"
#include "unicode.h"
#include "validate.h"

static void version(void)
{
  fprintf(stderr, "pwnsjp, version " K_VERSION "\n\n");
}

static void usage(void)
{
  fprintf(stderr,
    "Usage: pwnsjp [OPTIONS] PATTERN\n\n"
    "Options:\n"
    "  -d, --deep\n"
    "  -e, --entry-only\n"
    "  -i, --ui\n"
    "  -h, --help\n"
    "  -v, --version\n\n");
}

static void eabort(char* fstr, int line, char* errstr)
{
  if (fstr == NULL)
    fstr = "?";
  if (errstr == NULL)
    errstr = strerror(errno);
  fprintf(stderr, "%s[%d]: %s\n", fstr, line, errstr);
  exit(EXIT_FAILURE);
}

static void regex_free(regex_t *regex)
{
  regfree(regex);
}

static bool regex_compile(regex_t *regex, char* pattern)
{
  if (pattern != NULL)
  {
    debug("pattern = \"%s\"\n", pattern);
    return
      regcomp(regex, pattern, REG_NOSUB | REG_EXTENDED | REG_ICASE) == 0;
  }
  debug("pattern = NULL\n");
  return true;
}

static bool regex_match(regex_t *regex, char *string)
{
  return regexec(regex, string, 0, NULL, 0) == 0;
}
      
int main(int argc, char **argv)
{
  unsigned int i;
 
  term_init();

  char* pattern = parse_options(argc, argv);
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

#if 0
  char* s = ustr_to_str(L"Z\u017c\u00f3\u0142kn\u0105\u0107by");
  int len = strlen(s)-2;
  debug("width(\"%s\", %d) = %u\n", s, len, strnwidth(s, len));
  free(s);
#endif
  
#define try(s) do { if (!(s)) eabort(__FILE__, __LINE__, ""); } while(0)
#define tri(s, err) do { if (!(s)) eabort(__FILE__, __LINE__, err); } while(0)
  
  regex_t regex;
  tri ( regex_compile(&regex, pattern), "Invalid pattern" );
 
  struct io_t io;
  if (!io_init(&io, K_DATA_PATH))
  {
    fprintf(stderr, "Unable to open data file.\n");
    return EXIT_FAILURE;
  }
  
#ifdef K_VALIDATE_DATAFILE
  if (io.file_size != datafile_size)
  {
    fprintf(stderr, "Invalid data file size: %u, should be %u.\n", io.file_size, datafile_size);
    return EXIT_FAILURE;
  }
  if (!io_validate(&io))
  {
    fprintf(stderr, "Invalid data file signature.\n");
    return EXIT_FAILURE;
  }
#elif 0
  tri ( io.file_size > (1<<26), "Unexpectedly short data file" );
  tri ( io.file_size < (1<<28), "Unexpectedly long data file" );
#endif
  
  if (config.conf_ui && !ui_prepare())
  {
    fprintf(stderr, "Unable to initialize user interface.\n");
    return EXIT_FAILURE;
  }

  if (!io_prepareindex(&io))
  {
    fprintf(stderr, "Unable to prepare index.\n");
    return EXIT_FAILURE;
  }
  
  tri ( io.isize >= (1<<15), "Indecently few words" );
  tri ( io.isize <= (1<<17), "Indecently many words" );

  if (!io_buildindex(&io))
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
    
    struct io_iitem_t* iitem = io.iitems;
    for (i=0; i<io.isize; i++, iitem++)
    {
      char *tbuffer;
      if (config.conf_deep || !pattern || regex_match(&regex, iitem->entry))
      {
        bool dofree = false;
        debug(
          "\b\b<\n  localstr = %s\n  offset = %08x (file:%08x)\n  length = %06x\n>\n", 
          iitem->entry, 
          iitem->offset,
          io.header->words_base + iitem->offset,
          iitem->size);
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
          if (!config.conf_entry_only)
            printf("%s\n::: %s%s%s :::%s\n\n", 
              hueset[HUE_title], 
              hueset[HUE_boldtitle], 
              iitem->entry, 
              hueset[HUE_title], 
              hueset[HUE_default]);
          printf("%s\n", tbuffer);
        }
        if (dofree)
          free(tbuffer);
      }
    }
  }

  try ( io_fine(&io) );
  
  if (pattern)
    regex_free(&regex);

  if (config.conf_ui)
    ui_stop();
  
#undef try
#undef try_s

}

// vim: ts=2 sw=2 et
