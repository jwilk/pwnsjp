#include "common.h"

#ifndef PWNIO_H
#define PWNIO_H

struct pwnio_t
{
  FILE* file;
  size_t size;
  uint32_t word_count;
};

bool pwnio_init(struct pwnio_t *pwnio, char* filename);
bool pwnio_validate(struct pwnio_t *pwnio);
bool pwnio_fine(struct pwnio_t *pwnio);

#endif

// vim: ts=2 sw=2 et
