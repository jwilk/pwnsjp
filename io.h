/* Copyright (C) 2005 Jakub Wilk
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published 
 * by the Free Software Foundation.
 */

#include "common.h"

#ifndef IO_H
#define IO_H

struct io_header_t
{
  uint32_t __tmp1;
  uint32_t __tmp2;
  uint32_t __word_count;
  uint32_t index_base;
  uint32_t words_base;
}; 

struct io_iitem_t
{
  char* entry;
  uint32_t offset;
  unsigned int size : 31;
  unsigned int zipped : 1;
};

struct io_t
{
  FILE* file;
  size_t file_size;

  struct io_header_t* header;
  
  size_t isize;
  struct io_iitem_t* iitems;
  
  size_t csize;
  unsigned char* cbuffer;
};

bool io_init(struct io_t *io, const char* filename);
bool io_validate(struct io_t *io);
bool io_prepareindex(struct io_t *io);
bool io_buildindex(struct io_t *io);
void io_read(struct io_t *io, size_t indexno);
bool io_fine(struct io_t *io);
unsigned int io_locate(struct io_t *io, const char* search);

#endif

// vim: ts=2 sw=2 et
