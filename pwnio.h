#include "common.h"

#ifndef PWNIO_H
#define PWNIO_H

struct header_t
{
  uint32_t __tmp1;
  uint32_t __tmp2;
  uint32_t __word_count;
  uint32_t index_base;
  uint32_t words_base;
}; 

struct pwnio_t
{
  FILE* file;
  size_t file_size;
  unsigned int word_count;
  unsigned int max_entry_size;
  struct header_t *header;
  uint32_t* offsets;
  char* entry;
};

bool pwnio_init(struct pwnio_t *pwnio, char* filename);
bool pwnio_validate(struct pwnio_t *pwnio);
bool pwnio_prepareindex(struct pwnio_t *pwnio);
bool pwnio_buildindex(struct pwnio_t *pwnio);
bool pwnio_fine(struct pwnio_t *pwnio);

#endif

// vim: ts=2 sw=2 et
