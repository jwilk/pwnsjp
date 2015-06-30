/* Copyright © 2005-2013 Jakub Wilk <jwilk@jwilk.net>
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

#include "common.h"
#include "memory.h"
#include "terminfo.h"

#include <unistd.h>
#include <term.h>

bool is_term = false;

char *term_setaf[8];
char *term_setab[8];
char *term_sgr0;
char *term_bold;
char *term_rev;

static char* term_getstr(const char *str)
{
  char *result = tigetstr((char*) str);
  if (result == NULL || result == (char*)-1)
    return "";
  else
    return result;
}

void term_quit(void)
{
  unsigned int j;
  free(term_sgr0);
  free(term_rev);
  free(term_bold);
  for (j = 0; j < 8; j++)
    free(term_setaf[j]);
  for (j = 0; j < 8; j++)
    free(term_setab[j]);
}

void term_init(void)
{
  char *s0;
  int err;
  unsigned int j;

  if (!isatty(STDOUT_FILENO) || setupterm(NULL, STDOUT_FILENO, &err) != 0)
  {
    term_sgr0 = str_clone("");
    term_bold = str_clone("");
    term_rev = str_clone("");
    for (j = 0; j < 8; j++)
      term_setaf[j] = str_clone("");
    for (j = 0; j < 8; j++)
      term_setab[j] = str_clone("");
    atexit(term_quit);
    return;
  }
  is_term = true;

  term_sgr0 = str_clone(term_getstr("sgr0"));
  assert(term_sgr0 != NULL);

  term_bold = str_clone(term_getstr("bold"));
  assert(term_bold != NULL);

  term_rev = str_clone(term_getstr("rev"));
  assert(term_rev != NULL);

  memset(term_setaf, 0, sizeof(term_setaf));
  memset(term_setab, 0, sizeof(term_setab));

  s0 = term_getstr("setaf");
  if (*s0 != '\0')
    for (j = 0; j < 8; j++)
    {
      term_setaf[j] = tparm(s0, j);
      term_setaf[j] = str_clone(term_setaf[j] == NULL ? "" : term_setaf[j]);
      if (term_setaf[j] == NULL)
        term_setaf[j] = str_clone("");
    }
  else
    for (j = 0; j < 8; j++)
      term_setaf[j] = str_clone("");

  s0 = term_getstr("setab");
  if (*s0 != '\0')
    for (j = 0; j < 8; j++)
    {
      term_setab[j] = tparm(s0, j);
      term_setab[j] = str_clone(term_setab[j] == NULL ? "" : term_setab[j]);
      if (term_setab[j] == NULL)
        term_setab[j] = str_clone("");
    }
  else
    for (j=0; j<8; j++)
      term_setab[j] = str_clone("");

  atexit(term_quit);
}

// vim: ts=2 sts=2 sw=2 et
