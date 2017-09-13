/* Copyright © 2005-2017 Jakub Wilk <jwilk@jwilk.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#include <getopt.h>

#include "terminfo.h"
#include "memory.h"

struct config_t config = { };

static void deconfigure(void)
{
  free(config.filename);
}

char *parse_options(int argc, char **argv)
{
  static const struct option gopts[] =
  {
    { .name = "all",        .val = 'a' },
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

  assert(argc > 0 && argv != NULL && *argv != NULL && **argv != '\0');

  if (is_term)
    config.conf_color = true;

  while (true)
  {
    int i = 0;
    int c = getopt_long(argc, argv, "adef:hivDQRT", gopts, &i);
    if (c < 0)
      break;
    if (c == 0)
      c = gopts[i].val;
 #define on(o) ( config.conf_##o = true )
    switch (c)
    {
    case 'a':
      config.conf_all = true;
      break;
    case 'f':
      config.filename = optarg;
      break;
    case 'h':
      config.action = action_help;
      break;
    case 'i':
      config.conf_ui = true;
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
    case '?':
      exit(1);
    }
#undef on
  }
  if (config.filename == NULL)
    config.filename = "slo.win";
  if (strchr(config.filename, '/') == NULL)
  {
    bool hasext = strchr(config.filename, '.') != NULL;
    int size = 1 + snprintf(NULL, 0, DICTDIR "/%s%s", config.filename, hasext ? "" : ".win");
    char *buffer = alloz(size, 1);
    sprintf(buffer, DICTDIR "/%s%s", config.filename, hasext ? "" : ".win");
    config.filename = buffer;
    atexit(deconfigure);
  }
  if (config.conf_all)
  {
    if (optind < argc)
    {
      fprintf(stderr, "%s: too many arguments\n", argv[0]);
      exit(1);
    }
    return NULL;
  }
  else if (optind == argc)
  {
    config.conf_ui = true;
    return NULL;
  }
  else if (optind + 1 == argc)
    return argv[optind];
  else
  {
    fprintf(stderr, "%s: too many arguments\n", argv[0]);
    exit(1);
  }
}


// vim: ts=2 sts=2 sw=2 et
