#include "common.h"
#include "pwnio.h"

#include "config.h"
#include "byteorder.h"

bool pwnio_init(struct pwnio_t *pwnio, char* filename)
{
  debug("data file = \"%s\"\n", filename);
  pwnio->file = fopen(filename, "rb");
  if (pwnio->file == NULL)
    return false;
  if (fseek(pwnio->file, 0, SEEK_END) != 0)
    return false;
  pwnio->file_size = ftell(pwnio->file);
  pwnio->word_count = 0;
  pwnio->header = NULL;
  debug("data file size = %u MiB\n", pwnio->file_size>>20);
  return true;
}

bool pwnio_validate(struct pwnio_t *pwnio)
{
  if (fseek(pwnio->file, 0, SEEK_SET) != 0)
    return false;
  uint8_t sig[2];
  if (fread(sig, 1, sizeof(sig), pwnio->file) != sizeof(sig))
    return false;
  if (sig[0]!='G' || sig[1]!='W')
    return false;
  return true;
}

bool pwnio_prepareindex(struct pwnio_t *pwnio)
{
  if (fseek(pwnio->file, 0x10, SEEK_SET)!=0)
    return false;
  pwnio->header = malloc(sizeof(struct header_t));
  if (pwnio->header == NULL)
    return false;
  if (fread(pwnio->header, sizeof(struct header_t), 1, pwnio->file)!=1)
    return false;

  pwnio->word_count = le2cpu(pwnio->header->__word_count);
  debug("word count = %d\n", pwnio->word_count);

  pwnio->header->index_base = le2cpu(pwnio->header->index_base);
  debug("index #1 base = 0x%08x\n", pwnio->header->index_base);
  
  pwnio->header->index_base += sizeof(uint32_t)*pwnio->word_count;
  debug("index #2 base = 0x%08x\n", pwnio->header->index_base);
  
  pwnio->header->words_base = le2cpu(pwnio->header->words_base);
  debug("words base = 0x%08x\n", pwnio->header->words_base);
  
  return true;
}

static void uint32sqsort(uint32_t *l, uint32_t *r)
{
  uint32_t *i, *j;
  uint32_t p, temp, dist;
  
  do
  {
    i = l;
    j = r;
    dist = r-l;
    if (dist < 32)
    {
      for (i=l+1; i<=r; i++)
      for (j=i-1; j>=l && j[0]>j[1]; j--)
        // swap(i[0], j[0]):
        temp = j[1], j[1] = j[0], j[0] = temp;
      return;
    }
    p = l[dist/2];
    do
    {
      while (*i<p) i++;
      while (*j>p) j--;
      if (i <= j)
      {
        // swap(*i, *j):
        temp = *i, *i = *j, *j = temp;
        i++, j--;
      }
    }
    while (i <= j);
    if (l < j)
      uint32sqsort(l, j);
    l = i;
  }
  while (i < r);
}

inline static void uint32qsort(uint32_t* table, size_t count)
{
  uint32sqsort(table, table+(count-1));
}

bool pwnio_buildindex(struct pwnio_t *pwnio)
{
  if (fseek(pwnio->file, pwnio->header->index_base, SEEK_SET)!=0)
    return false;
  pwnio->offsets = malloc(sizeof(uint32_t)*pwnio->word_count);
  if (pwnio->offsets == NULL)
    return false;
  if (fread(pwnio->offsets, sizeof(uint32_t), pwnio->word_count, pwnio->file) != pwnio->word_count)
    return false;

  unsigned int i;
  for (i=0; i<pwnio->word_count; i++)
    pwnio->offsets[i] = le2cpu(pwnio->offsets[i]) & 0x07ffffff;
  uint32qsort(pwnio->offsets, pwnio->word_count);

  unsigned int size, maxsize = 1024;
  for (i=0; i<pwnio->word_count-1; i++)
  {
    size = pwnio->offsets[i+1]-pwnio->offsets[i];
    if (size > maxsize)
      maxsize = size;
  }
  if (maxsize > (1<<16))
    return false;
  pwnio->max_entry_size = (maxsize|0xff)+1;
  pwnio->entry = malloc(pwnio->max_entry_size << 3);
  return (pwnio->entry != NULL);
}

bool pwnio_fine(struct pwnio_t *pwnio)
{
  free(pwnio->header);
  free(pwnio->offsets);
  free(pwnio->entry);
  return fclose(pwnio->file) == 0;
}

// vim: ts=2 sw=2 et
