#include "common.h"
#include "pwnio.h"
#include "config.h"

bool pwnio_init(struct pwnio_t *pwnio, char* filename)
{
  debug("data file = \"%s\"\n", filename);
  pwnio->file = fopen(filename, "rb");
  if (pwnio->file == NULL)
    return false;
  if (fseek(pwnio->file, 0, SEEK_END) != 0)
    return false;
  pwnio->size = ftell(pwnio->file);
  debug("data file size = %u MiB\n", pwnio->size>>20);
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

bool pwnio_fine(struct pwnio_t *pwnio)
{
  return fclose(pwnio->file) == 0;
}

// vim: ts=2 sw=2 et
