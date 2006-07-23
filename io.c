/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "memory.h"
#include "io.h"

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
  io->isize = 0;
  io->header = NULL;
  debug("data file size = %u MiB\n", io->file_size>>20);
  return true;
}

bool io_validate(struct io_t *io)
{
  if (fseek(io->file, 0, SEEK_SET) != 0)
    return false;
  uint8_t sig[2];
  if (fread(sig, 1, sizeof(sig), io->file) != sizeof(sig))
    return false;
  if (sig[0]!='G' || sig[1]!='W')
    return false;
  return true;
}

bool io_prepareindex(struct io_t *io)
{
  if (fseek(io->file, 0x4, SEEK_SET)!=0)
    return false;
#define hsize (sizeof(struct io_header_t))
  io->header = alloc(1, hsize);
  if (fread(io->header, hsize, 1, io->file)!=1)
    return false;
#undef hsize

  unsigned int i;
  for (i=1; i<5; i++)
  {
    io->header->__tmp[i] = le2cpu(io->header->__tmp[i]);
    debug("m(%u) = 0x%08x\n", i, io->header->__tmp[i]);
  }

  for (i=0; i<5; i++)
  {
    io->header->__tmq[i] = le2cpu(io->header->__tmq[i]);
    debug("m(%u) = 0x%08x\n", i+6, io->header->__tmq[i]);
  }
  
  io->iitems = NULL;
  io->isize = le2cpu(io->header->__word_count);
  debug("word count = %d\n", io->isize);

  io->header->index_base = le2cpu(io->header->index_base);
  debug("index #1 base = 0x%08x\n", io->header->index_base);
  
  io->header->index_base += sizeof(uint32_t)*io->isize;
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
      for (i=l+1; i<=r; i++)
      {
        p = *i;
        for (j=i; j>l && j[-1]>p; j--)
          j[0] = j[-1];
        j[0] = p;
      }
      return;
    }
    swap(l[dist/2], r[0]);
    p = temp;
    i = l-1;
    for (j=l; j<=r; j++)
      if (*j <= p)
      {
        i++;
        swap(*i, *j);
      }
    if (i >= r)
      i--;
    if (l < j)
      uint32_qsort(l, i);
    l = i+1;
  }
}

inline static void uint32_sort(uint32_t* table, size_t count)
{
  uint32_qsort(table, table+(count-1));
}

#define gt(w, v) (strcmp(w, v) > 0)

static void iitem_qsort(struct io_iitem_t *l, struct io_iitem_t *r)
{
  char *p;
  struct io_iitem_t *i, *j, temp;
  int dist;

  while(true)
  {
    dist = r - l;
    assert(gt(r[1].xentry, r[0].xentry));
    if (dist <= 16)
    {
      for (i=r-1; i>=l; i--)
      {
        temp = *i;
        for (j=i; gt(temp.xentry, j[1].xentry); j++)
          j[0] = j[1];
        j[0] = temp;
      }
      return;
    }
    swap(l[dist/2], r[0]);
    p = temp.xentry;
    i = l-1;
    for (j=l; j<=r; j++)
      if (!gt(j->xentry, p))
      {
        i++;
        swap(*i, *j);
      }
    if (i >= r)
      i--;
    if (l < j)
      iitem_qsort(l, i);
    l = i+1;
  }
}

#undef gt
#undef swap

static void iitem_sort(struct io_iitem_t* table, size_t count)
{
#if defined(MERGESORT)

  debug("mergesort = yes\n");
  unsigned int buffs, blocks, blockc = 0;
  buffs = blockc = 0;
  blocks = 1;
  struct io_iitem_t *iitem;
  unsigned int i;

#define forallitems for (i=0, iitem=table; i<count; i++, iitem++)
  forallitems
    if (strcmp(iitem[1].xentry, iitem->xentry) < 0)
    {
      iitem->stirring = 1;
      blockc++;
      if (blocks > buffs)
        buffs = blocks;
      blocks = 1;
    }
    else
      blocks++;
  
  debug("mergesort blocks = %u\n", blockc);
  unsigned int qlog, qlim = 2*(blockc + 2);
 	for (qlog=0; qlim>0; qlog++)
    qlim >>= 1;
  qlim = (1<<qlog) - 1;
  debug("mergesort stack limit = 2^%u - 1 = %u\n", qlog, qlim);
  
  struct io_iitem_t* queue[qlim+1];
  unsigned int qhead, qtail, qlen;
  qlen = 0;
  qhead = qtail = -1;
  
#define pop() (qlen--, qtail = ((qtail+1) & qlim), queue[qtail])
#define push(x) (qlen++, qhead = ((qhead+1) & qlim), queue[qhead] = (x))
  
  push(table);
  forallitems
    if (iitem->stirring)
    {
      push(iitem);
      push(iitem + 1);
    }
#undef forallitems
  push(iitem);
  
  struct io_iitem_t* buffer = alloc(count, sizeof(struct io_iitem_t));
  
  while (qlen >= 4)
  {
    struct io_iitem_t *a, *al, *ah, *bl, *bh, *r;
    al = pop(); ah = pop();
    bl = pop(); bh = pop();
    if (bl != ah+1)
    {
      push(al); push(ah);
      al = bl; ah = bh;
      bl = pop(); bh = pop();
    }
    a = al;
    push(al);
    push(bh);
    r = buffer;
    while (true)
    {
      if (bl > bh)
      {
        if (ah >= al)
          memcpy(r, al, (1+ah-al)*sizeof(struct io_iitem_t));
        break;
      }
      if (al > ah)
      {
        bh = bl - 1;
        break;
      }
      if (strcmp(al->xentry, bl->xentry) < 0)
        *r++ = *al++;
      else
        *r++ = *bl++;
    }
    memcpy(a, buffer, (1+bh-a)*sizeof(struct io_iitem_t));
  }

#undef push
#undef pop

  free(buffer);

#else
  
  debug("quicksort = yes\n");
  iitem_qsort(table, table + count - 1);

#endif
}

unsigned int io_locate(struct io_t *io, const char *search)
{
  struct io_iitem_t *l, *r, *m;
 
  char *xsearch = strxform(search);
  
  l = io->iitems;
  r = l + io->isize-1;
  while (r > l)
  {
    m = l + (r-l)/2;
    if (strcmp(xsearch, m->xentry) > 0)
      l = m+1;
    else
      r = m;
  }

  free(xsearch);
  
  assert(l >= io->iitems);
  assert(l < io->iitems + io->isize);
  return l - io->iitems;
}

void io_read(struct io_t *io, unsigned int indexno)
{
  assert(io != NULL);
  assert(io->iitems != NULL);
  struct io_iitem_t* iitem = io->iitems + indexno;
  
  char buffer[io->csize];
 
  assert(io->header != NULL);
  if 
    ( (fseek(io->file, io->header->words_base + iitem->offset, SEEK_SET) != 0) ||
      (fread(buffer, iitem->size, 1, io->file) != 1) )
  {
    io->cbuffer[0] = '\0';
    return;
  }
  
  unsigned long dsize = io->csize << 3;
  assert(io->cbuffer != NULL);
  if (iitem->zipped)
  {
    memset(io->cbuffer, 0, dsize);
    uncompress((unsigned char*)io->cbuffer, &dsize, (unsigned char*)buffer, io->csize);
  }
  else
    memcpy(io->cbuffer, buffer, iitem->size);
}

bool io_buildindex(struct io_t *io)
{
  if (fseek(io->file, io->header->index_base, SEEK_SET)!=0)
    return false;

  uint32_t* offsets = alloc(io->isize, sizeof(uint32_t));
  if (offsets == NULL)
    return false;
  if (fread(offsets, sizeof(uint32_t), io->isize, io->file) != io->isize)
    return false;

  debug("sizeof(struct io_iitem_t) = %u\n", sizeof(struct io_iitem_t));
  
  io->iitems = alloz(io->isize-1, sizeof(struct io_iitem_t));
  if (io->iitems == NULL)
  {
    free(offsets);
    return false;
  }

  unsigned int i;
  for (i=0; i<io->isize; i++)
    offsets[i] = le2cpu(offsets[i]) & 0x07ffffff;
  uint32_sort(offsets, io->isize);

  io->isize -= 2;
  
  unsigned int size, maxsize = 1024;
  for (i=0; i<io->isize; i++)
  {
    assert(offsets[i+1] > offsets[i]);
    size = offsets[i+1]-offsets[i];
    io->iitems[i].size = size;
    io->iitems[i].offset = offsets[i];
    if (size > maxsize)
      maxsize = size;
  }
  free(offsets);

  if (maxsize >= (1<<14))
    return false;
  
  io->csize = (maxsize|0xff)+1;
  io->cbuffer = alloc(io->csize << 3, sizeof(char));
  if (io->cbuffer == NULL)
    return false;
 
  struct io_iitem_t* iitem;

#define forallitems for (i=0, iitem=io->iitems; i<io->isize; i++, iitem++)
  forallitems
  {
    char wordbuffer[io->csize], *dataptr;
    unsigned int diffsize;

    if (fseek(io->file, io->header->words_base + iitem->offset, SEEK_SET) != 0)
      return false;
    if (fread(wordbuffer, iitem->size, 1, io->file) != 1)
      return false;
    dataptr = wordbuffer + 12;
    iitem->entry = 
      config.conf_quick ? 
        str_clone(dataptr) : 
        pwnstr_to_str(dataptr);
    dataptr = strchr(dataptr, '\0') + 2;
    if (*dataptr < ' ')
    {
      dataptr += (*dataptr) + 1;
      iitem->zipped = 1;
    }
    diffsize = dataptr - wordbuffer;
    iitem->size -= diffsize;
    iitem->offset += diffsize;
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
  char *plusinf; // +oo == "\xff\xff...\xff"
  plusinf = alloc(maxlen + 1, sizeof(char));
  memset(plusinf, -1, maxlen);
  plusinf[maxlen]='\0';
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
    struct io_iitem_t* iitem = io->iitems;
    for (i=0; i<=io->isize; i++, iitem++)
    {
      if (iitem->xentry != iitem->entry)
        free(iitem->xentry);
      free(iitem->entry);
    }
    free(io->iitems);
  }
  return fclose(io->file) == 0;
}

// vim: ts=2 sw=2 et