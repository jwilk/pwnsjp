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

#ifndef IO_H
#define IO_H

struct io_header_t
{
  uint32_t tmp[5];
  uint32_t word_count;
  uint32_t index_base;
  uint32_t words_base;
  uint32_t tmq[5];
};

struct io_iitem_t
{
  char *xentry;
  char *entry;
  uint32_t offset;
  unsigned int size : 30;
  unsigned int zipped : 1;
  unsigned int stirring : 1;
};

struct io_t
{
  FILE* file;
  size_t file_size;
  int encoding;

  struct io_header_t* header;

  size_t isize;
  struct io_iitem_t* iitems;

  size_t csize;
  char* cbuffer;
};

bool io_init(struct io_t *io, const char* filename);
bool io_validate(struct io_t *io);
bool io_prepare_index(struct io_t *io);
bool io_build_index(struct io_t *io);
void io_read(struct io_t *io, size_t indexno);
bool io_fine(struct io_t *io);
unsigned int io_locate(struct io_t *io, const char* search);

#endif

// vim: ts=2 sts=2 sw=2 et
