/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"
#include "io.h"
#include "unicode.h"

#include "config.h"
#include "byteorder.h"

#include <zlib.h>

bool io_init(struct io_t *io, const unsigned char* filename)
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
#define hsize (sizeof(struct io_header_t))
  if (fseek(io->file, 0x10, SEEK_SET)!=0)
    return false;
  io->header = malloc(hsize);
  if (io->header == NULL)
    return false;
  if (fread(io->header, hsize, 1, io->file)!=1)
    return false;

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
#undef hsize
}

#define swap(w, v) do { temp = w; w = v; v = temp; } while(false)

static void uint32sqsort(uint32_t *l, uint32_t *r)
{
  uint32_t *i, *j;
  uint32_t p, temp;
  int dist;
  
  while (true)
  {
    dist = r-l;
    if (dist < 32)
    {
      for (i=l+1; i<=r; i++)
      for (j=i-1; j>=l && j[0]>j[1]; j--)
        swap(j[0], j[1]);
      return;
    }
    i = l;
    j = r;
    p = l[dist/2];
    do
    {
      while (*i<p) i++;
      while (*j>p) j--;
      if (i <= j)
      {
        swap(*i, *j);
        i++, j--;
      }
    }
    while (i < j);

    if (l < j)
      uint32sqsort(l, j);
    l = i;
  }
}

inline static void uint32sort(uint32_t* table, size_t count)
{
  uint32sqsort(table, table+(count-1));
}

#define gt(w, v) (strcmp(w, v) > 0)
#define isort() \
  do { \
    if (dist < 64) \
    { \
      for (i=l+1; i<=r; i++) \
      for (j=i-1; j>=l && gt(j[0].xentry, j[1].xentry); j--) \
        swap(j[0], j[1]); \
      return; \
    } \
  } while(false);

static void iitems_qsort(struct io_iitem_t *l, struct io_iitem_t *r)
{
  register unsigned char* p;
  register struct io_iitem_t *i, *j;
  struct io_iitem_t temp;
  int dist;

  while(true)
  {
    dist = r - l;
    isort();
    i = l;
    j = r;
    p = l[dist/2].xentry;
    do
    {
      while (gt(p, i->xentry)) i++;
      while (gt(j->xentry, p)) j--;
      if (i <= j)
      {
        swap(*i, *j);
        i++, j--;
      }
    }
    while (i < j);

    if (l < j)
      iitems_qsort(l, j);
    l = i;
  }
}

static void iitems_qsort_l(struct io_iitem_t *l, struct io_iitem_t *r)
{
  register unsigned char* p;
  register struct io_iitem_t *i, *j;
  struct io_iitem_t temp;
  int dist;

  while(true)
  {
    dist = r - l;
    isort();
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
      iitems_qsort_l(l, i);
    l = i+1;
  }
}

#undef gt
#undef swap

inline static void iitem_sort(struct io_iitem_t* table, size_t count)
{
#if defined(MERGESORT)
#error Merge sort has not been implemented (as yet)
#elif defined(LOMUTO)
  // table[count] == +oo
  iitems_qsort_l(table, table + count);
#else
  // table[count] == +oo
  iitems_qsort(table, table + count);
#endif
}

unsigned int io_locate(struct io_t *io, const unsigned char* search)
{
  struct io_iitem_t *l, *r, *m;
 
  unsigned char* xsearch = strxform(search);
  
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
  
  unsigned char buffer[io->csize];
 
  assert(io->header != NULL);
  if 
    ( (fseek(io->file, io->header->words_base + iitem->offset, SEEK_SET) != 0) ||
      (fread(buffer, iitem->size, 1, io->file) != 1) )
  {
    io->cbuffer[0] = '\0';
    return;
  }
  
  long dsize = io->csize << 3;
  assert(io->cbuffer != NULL);
  if (iitem->zipped)
  {
    memset(io->cbuffer, 0, dsize);
    uncompress(io->cbuffer, &dsize, buffer, io->csize);
  }
  else
    memcpy(io->cbuffer, buffer, iitem->size);
}

bool io_buildindex(struct io_t *io)
{
  if (fseek(io->file, io->header->index_base, SEEK_SET)!=0)
    return false;

  uint32_t* offsets = malloc(sizeof(uint32_t)*io->isize);
  if (offsets == NULL)
    return false;
  if (fread(offsets, sizeof(uint32_t), io->isize, io->file) != io->isize)
    return false;
 
  io->iitems = calloc(io->isize-1, sizeof(struct io_iitem_t));
  if (io->iitems == NULL)
  {
    free(offsets);
    return false;
  }

  unsigned int i;
  for (i=0; i<io->isize; i++)
    offsets[i] = le2cpu(offsets[i]) & 0x07ffffff;
  uint32sort(offsets, io->isize);

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

  if (maxsize > (1<<16))
    return false;
  
  io->csize = (maxsize|0xff)+1;
  io->cbuffer = malloc(io->csize << 3);
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
        strdup(dataptr) : 
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
      len = maxlen;
  }
  unsigned char *plusinf; // +oo == "\xff\xff...\xff"
  plusinf = malloc(maxlen+1);
  memset(plusinf, -1, maxlen);
  plusinf[maxlen]='\0';
  iitem->entry = iitem->xentry = plusinf;

#if 0  
  forallitems
  {
    printf("%s\n", iitem->entry);
    if (strcmp(iitem[1].xentry, iitem->xentry) < 0)
      printf("\n");
  }
#endif

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
