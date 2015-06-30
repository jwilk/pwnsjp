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

#include <errno.h>

void fatal_impl(const char* fstr, int line, const char* errstr)
{
  if (fstr == NULL)
    fstr = "?";
  if (errstr == NULL)
    errstr = strerror(errno);
  fprintf(stderr, "%s[%d]: %s\n", fstr, line, errstr);
  exit(EXIT_FAILURE);
}

#define memcheck(x) do { if ((x) == NULL) fatal(NULL); } while(0)

void *alloc(size_t nmemb, size_t size)
{
  void *mem = malloc(nmemb * size);
  memcheck(mem);
  return mem;
}

void *alloz(size_t nmemb, size_t size)
{
  void *mem = calloc(nmemb, size);
  memcheck(mem);
  return mem;
}

char *str_clone(const char *s)
{
  char *clone = strdup(s);
  memcheck(clone);
  return clone;
}

extern wchar_t *wcsdup(const wchar_t*);

wchar_t *wcs_clone(const wchar_t *s)
{
  wchar_t *clone = wcsdup(s);
  memcheck(clone);
  return clone;
}

// vim: ts=2 sts=2 sw=2 et
