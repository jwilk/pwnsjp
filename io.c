/* Copyright © 2005-2026 Jakub Wilk <jwilk@jwilk.net>
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
#include "io.h"

#include <errno.h>
#include <zlib.h>

#include "byteorder.h"
#include "config.h"
#include "unicode.h"

bool io_init(struct io_t *io, const char *filename)
{
  debug("data file = \"%s\"\n", filename);
  io->file = fopen(filename, "rb");
  if (io->file == NULL)
    return false;
  if (fseek(io->file, 0, SEEK_END) != 0)
    return false;
  io->file_size = ftell(io->file);
  io->encoding = 88592;
  io->isize = 0;
  io->header = NULL;
  debug("data file size = %zu MiB\n", io->file_size >> 20);
  return true;
}

bool io_validate(struct io_t *io)
{
  if (fseek(io->file, 0, SEEK_SET) != 0)
    return false;
  uint8_t sig[2];
  if (fread(sig, 1, sizeof sig, io->file) != sizeof sig)
    return false;
  if (sig[0] != 'G' || sig[1] != 'W')
    return false;
  return true;
}

bool io_prepare_index(struct io_t *io)
{
  if (fseek(io->file, 0x4, SEEK_SET) != 0)
    return false;
#define hsize (sizeof (struct io_header_t))
  io->header = alloc(1, hsize);
  if (fread(io->header, hsize, 1, io->file) != 1)
    return false;
#undef hsize

  unsigned int i;
  for (i = 1; i < 5; i++)
  {
    io->header->tmp[i] = le2cpu(io->header->tmp[i]);
    debug("m(%u) = 0x%08x\n", i, io->header->tmp[i]);
  }

  for (i = 0; i < 5; i++)
  {
    io->header->tmq[i] = le2cpu(io->header->tmq[i]);
    debug("m(%u) = 0x%08x\n", i+6, io->header->tmq[i]);
  }

  io->iitems = NULL;
  io->isize = le2cpu(io->header->word_count);
  debug("word count = %zu\n", io->isize);

  io->header->index_base = le2cpu(io->header->index_base);
  debug("index #1 base = 0x%08x\n", io->header->index_base);

  io->header->index_base += sizeof (uint32_t) * io->isize;
  debug("index #2 base = 0x%08x\n", io->header->index_base);

  io->header->words_base = le2cpu(io->header->words_base);
  debug("words base = 0x%08x\n", io->header->words_base);

  return true;
}

#define swap(w, v) do { temp = w; w = v; v = temp; } while(false)

static void uint32_qsort(uint32_t *l, uint32_t *r)
{
  uint32_t *i, *j, p, temp;
  int dist;

  while (true)
  {
    dist = r - l;
    if (dist <= 16)
    {
      for (i = l + 1; i <= r; i++)
      {
        p = *i;
        for (j = i; j > l && j[-1] > p; j--)
          j[0] = j[-1];
        j[0] = p;
      }
      return;
    }
    swap(l[dist/2], r[0]);
    p = r[0];
    i = l - 1;
    for (j = l; j <= r; j++)
      if (*j <= p)
      {
        i++;
        swap(*i, *j);
      }
    if (l < j)
      uint32_qsort(l, i - 1);
    l = i + 1;
  }
}

inline static void uint32_sort(uint32_t* table, size_t count)
{
  uint32_qsort(table, table + count - 1);
}

#define gt(w, v) (strcmp(w, v) > 0)

static void iitem_qsort(struct io_iitem_t *l, struct io_iitem_t *r)
{
  char *p;
  struct io_iitem_t *i, *j, temp;
  int dist;

  while (true)
  {
    dist = r - l;
    if (dist <= 16)
    {
      for (i = r - 1; i >= l; i--)
      {
        temp = *i;
        for (j = i; gt(temp.xentry, j[1].xentry); j++)
          j[0] = j[1];
        j[0] = temp;
      }
      return;
    }
    swap(l[dist / 2], r[0]);
    p = r[0].xentry;
    i = l - 1;
    for (j = l; j <= r; j++)
      if (!gt(j->xentry, p))
      {
        i++;
        swap(*i, *j);
      }
    if (l < j)
      iitem_qsort(l, i - 1);
    l = i + 1;
  }
}

#undef gt
#undef swap

static void iitem_sort(struct io_iitem_t *table, size_t count)
{
  iitem_qsort(table, table + count - 1);
}

unsigned int io_locate(struct io_t *io, const char *search)
{
  struct io_iitem_t *l, *r, *m;

  char *xsearch = strxform(search);

  l = io->iitems;
  r = l + io->isize-1;
  while (r > l)
  {
    m = l + (r - l)/2;
    if (strcmp(xsearch, m->xentry) > 0)
      l = m + 1;
    else
      r = m;
  }

  free(xsearch);

  assert(l >= io->iitems);
  assert(l < io->iitems + io->isize);
  return l - io->iitems;
}

void io_read(struct io_t *io, size_t indexno)
{
  assert(io != NULL);
  assert(io->iitems != NULL);
  struct io_iitem_t *iitem = io->iitems + indexno;

  char buffer[io->csize];

  assert(io->header != NULL);
  if
    ( (fseek(io->file, io->header->words_base + iitem->offset, SEEK_SET) != 0) ||
      (fread(buffer, iitem->size, 1, io->file) != 1) )
  {
    errno = EIO;
    fatal(NULL);
    return;
  }

  unsigned long dsize = io->csize << 3;
  assert(io->cbuffer != NULL);
  if (iitem->zipped)
  {
    memset(io->cbuffer, 0, dsize);
    uncompress((unsigned char *)io->cbuffer, &dsize, (unsigned char *)buffer, io->csize);
  }
  else
    memcpy(io->cbuffer, buffer, iitem->size);
}

bool io_build_index(struct io_t *io)
{
  if (fseek(io->file, io->header->index_base, SEEK_SET) != 0)
    return false;

  uint32_t *offsets = alloc(io->isize, sizeof (uint32_t));
  if (offsets == NULL)
    return false;
  if (fread(offsets, sizeof (uint32_t), io->isize, io->file) != io->isize)
    return false;

  debug("sizeof(struct io_iitem_t) = %zu\n", sizeof(struct io_iitem_t));

  io->iitems = alloz(io->isize - 1, sizeof (struct io_iitem_t));
  if (io->iitems == NULL)
  {
    free(offsets);
    return false;
  }

  unsigned int i;
  for (i = 0; i < io->isize; i++)
    offsets[i] = le2cpu(offsets[i]) & 0x07FFFFFF;
  uint32_sort(offsets, io->isize);

  io->isize -= 2;

  unsigned int size, maxsize = 1024;
  for (i = 0; i < io->isize; i++)
  {
    assert(offsets[i + 1] > offsets[i]);
    size = offsets[i + 1] - offsets[i];
    io->iitems[i].size = size;
    io->iitems[i].offset = offsets[i];
    if (size > maxsize)
      maxsize = size;
  }
  free(offsets);

  if (maxsize >= (1 << 14))
    return false;

  io->csize = (maxsize | 0xFF) + 1;
  io->cbuffer = alloc(io->csize << 3, 1);
  if (io->cbuffer == NULL)
    return false;

  struct io_iitem_t *iitem;

#define forallitems for (i = 0, iitem = io->iitems; i < io->isize; i++, iitem++)
  forallitems
  {
    char wordbuffer[io->csize], *dataptr;
    unsigned int diffsize;

    if (fseek(io->file, io->header->words_base + iitem->offset, SEEK_SET) != 0)
      return false;
    if (fread(wordbuffer, iitem->size, 1, io->file) != 1)
      return false;
    dataptr = wordbuffer + 12;
    iitem->entry = str_clone(dataptr);
    if (strchr(iitem->entry, 0x9C))
      // 0x9C is LATIN SMALL LETTER S WITH ACUTE in Windows-1250
      // but a C1 control character in ISO-8859-2
      io->encoding = 1250;
    dataptr = strchr(dataptr, '\0') + 2;
    if ((unsigned int) *dataptr < ' ')
    {
      dataptr += (*dataptr) + 1;
      iitem->zipped = 1;
    }
    diffsize = dataptr - wordbuffer;
    iitem->size -= diffsize;
    iitem->offset += diffsize;
  }
  set_pwn_charset(io->encoding == 1250);
  if (!config.conf_quick)
    forallitems
    {
      char *entry = iitem->entry;
      iitem->entry = pwnstr_to_str(entry);
      free(entry);
    }
  if (posix_coll())
    forallitems iitem->xentry = iitem->entry;
  else
    forallitems iitem->xentry = strxform(iitem->entry);

  unsigned int len, maxlen = 0;
  forallitems
  {
    len = strlen(iitem->xentry);
    if (len > maxlen)
      maxlen = len;
  }
  char *plusinf; // +oo == "\xFF\xFF...\xFF"
  plusinf = alloc(maxlen + 1, 1);
  memset(plusinf, -1, maxlen);
  plusinf[maxlen] = '\0';
  iitem->entry = iitem->xentry = plusinf;

#undef forallitems

  iitem_sort(io->iitems, io->isize);

  return true;
}

bool io_fine(struct io_t *io)
{
  free(io->header);
  free(io->cbuffer);
  if (io->iitems != NULL)
  {
    unsigned int i;
    struct io_iitem_t *iitem = io->iitems;
    for (i = 0; i <= io->isize; i++, iitem++)
    {
      if (iitem->xentry != iitem->entry)
        free(iitem->xentry);
      free(iitem->entry);
    }
    free(io->iitems);
  }
  return fclose(io->file) == 0;
}

// vim: ts=2 sts=2 sw=2 et
