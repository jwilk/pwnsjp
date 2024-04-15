/* Copyright © 2005-2023 Jakub Wilk <jwilk@jwilk.net>
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
#include "hue.h"

#include "terminfo.h"

int hue_blend(int hue, int new_fg, int new_bg, int new_ex)
{
  if (new_fg != -1) { hue &= ~hue_fg_mask; hue |= new_fg; }
  if (new_bg != -1) { hue &= ~hue_bg_mask; hue |= new_bg; }
  if (new_ex != -1) { hue &= ~hue_ex_mask; hue |= new_ex; }
  return hue;
}

char *hue[hue_count];

void hue_setup_curses(void)
{
  static char cbuf[hue_count][6];
  unsigned int i;
  assert(hue_count < 0x1000);
  for (i = 0; i < hue_count; i++)
  {
    sprintf(cbuf[i], "\33%03xm", i);
    hue[i] = cbuf[i];
  }
}

static void xstrcat(char *dst, size_t size, const char *src)
{
  strncat(dst, src, size - strlen(dst) - 1);
}

void hue_setup_terminfo(void)
{
#define bufsize 32
  static char cbuf[hue_count][bufsize];

  for (int i = 0; i < hue_count; i++)
    xstrcat(cbuf[i], bufsize, term_sgr0);

  for (int fg = 0; fg < hue_fg_count; fg++)
  for (int bg = 0; bg < hue_bg_count; bg++)
  for (int ex = 0; ex < hue_ex_count; ex++)
  {
    int i = (fg << hue_fg_shift) + (bg << hue_bg_shift) + (ex << hue_ex_shift);
    xstrcat(cbuf[i], bufsize, term_setaf[fg]);
    xstrcat(cbuf[i], bufsize, term_setab[bg]);
    if ((ex << hue_ex_shift) & hue_bold)
      xstrcat(cbuf[i], bufsize, term_bold);
    if ((ex << hue_ex_shift) & hue_reverse)
      xstrcat(cbuf[i], bufsize, term_rev);
    assert(strlen(cbuf[i]) < bufsize);
    if (strlen(cbuf[i]) == bufsize - 1)
      // most likely truncation occurred
      cbuf[i][0] = '\0';
  }

  for (int i = 0; i < hue_count; i++)
    hue[i] = cbuf[i];

#undef bufsize
}

// vim: ts=2 sts=2 sw=2 et
