/* Copyright © 2005, 2010 Jakub Wilk <jwilk@jwilk.net>
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
#include "config.h"
#include "memory.h"
#include "regex.h"

void regex_free(regex_t *regex)
{
  regfree(regex);
}

bool regex_compile(regex_t *regex, const char* p)
{
  if (p == NULL)
  {
    debug("pattern = NULL\n");
    return true;
  }
  debug("pattern = \"%s\"\n", p);
  return
    regcomp(regex, p, REG_NOSUB | REG_EXTENDED | REG_ICASE) == 0;
}

char* pattern_head(const char* p)
{
  if (p == NULL || *p != '^')
    return NULL;
  
  do p++; while (*p == '^');
  
  bool esc = false;
  char result[1 + strlen(p)];
  result[0]='\0';
  char *r;
  for (r=result+1; *p; p++)
  if (esc)
    switch (*p)
    {
    case '^': case '.': case '[': case ']': case '$': case '(': case ')': 
    case '|': case '*': case '+': case '?': case '{': case '}': case '\\':
      *r++ = *p;
      esc = false;
      break;
    default:
      goto enough;
    }
  else
    switch (*p)  
    {
    case '\\':
      esc = true;
      break;
    case '^': case '|':
      return NULL;
    case '+': case '.': case '$': case '[': case '(':
      goto enough;
    case '*': case '?': case '{':
      r--;
      goto enough;
    default:
      *r++ = *p;
    }
enough:
  *r = '\0';
  debug("pattern head = \"%s\"\n", result+1);
  return str_clone(result+1);
}

bool regex_match(regex_t *regex, const char *string)
{
  return regexec(regex, string, 0, NULL, 0) == 0;
}

// vim: ts=2 sw=2 et
