#include "common.h"

#include <unistd.h>

#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>

#include <regex.h>

#include <zlib.h>

#ifndef K_DATA_PATH
#  define K_DATA_PATH "./slo.win"
#endif
#ifndef K_VERSION
#  define K_VERSION "<devel>"
#endif

#include "byteorder.h"
#include "config.h"
#include "terminfo.h"
#include "unicode.h"
#include "validate.h"

static char* trim_html(char *str)
// Warning: characters of `str' are destroyed!
{
  enum
  {
    s_default,
    s_html_vague,
    s_html_open,
    s_html_close
  } state = s_default;
  char *head, *tail, *appendix;
  int len = strlen(str);
  char result[2*len];
  bool first = true;
  bool color = false;
  
  head=tail=str;
  appendix=result;
#define a(t) do *(appendix++)=t; while (0)
#define as(t) do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
#define sync do head = tail+1; while(0)
  for (head=tail=str; *tail; tail++)
  switch(state)
  {
  case s_default:
    if (*tail == '<')
      state = s_html_vague;
    else
      a(*tail);
    head++;
    break;
  case s_html_vague:
    if (*tail == '/')
    {
      state = s_html_close;
      head++;
    }
    else
      state = s_html_open;
    break;
  case s_html_open:
    if (*tail=='>') 
    {
      state=s_default;
      *tail='\0';
      if (!strcasecmp(head, "p style=\"tab\""))
        as("\n   ");
      else if (!strcasecmp(head, "p") || !strcasecmp(head, "br"))
        a('\n');
      else if (!strcasecmp(head, "b"))
      {
        if (first)
        {
          as(term_setab[5]);
          first = false;
          color = true;
        }
        as(term_bold);
      }
      else if (!strcasecmp(head, "tr1") && !color)
      {
        as(term_setab[5]);
        as(term_bold);
        color = true;
      }
      else if (!strcasecmp(head, "font color=#ff0000"))
      {
        as(term_setaf[1]);
        as(term_bold);
        color = true;
      }
/*    else if (!strcasecmp(head, "font color=#fa8d00"))
      {
        as(tput("setaf 3"));
        as(tput("bold"));
        color = true;
      } */
      else if (!strcasecmp(head, "i") && !color)
      {
        as(term_setaf[1]);
        color = true;
      }
      else if (!strcasecmp(head, "sup"))
        a('^');
      else if (!strcasecmp(head, "sub"))
        a('_');
      else if (!strcasecmp(head, "sqrt"))
        as("&sqrt;");
      else if (!strncasecmp(head, "a href=", 7))
      {
        if (!color)
        {
          as(term_setaf[6]);
          color = true;
        }
      }
      sync;
    }
    break;
  case s_html_close:
    if (*tail == '>')
    {
      *tail = '\0';
      state = s_default;
      if 
        (
          !strcasecmp(head, "a") || 
          !strcasecmp(head, "b") || 
          !strcasecmp(head, "i") || 
          !strcasecmp(head, "q") || 
          !strcasecmp(head, "tr1") || 
          !strcasecmp(head, "font") 
        )
      {
        as(term_sgr0);
        color = false;
      }
      else if (!strcasecmp(head, "p"))
        while(tail[1] == ' ')
          tail++;
      sync;
    }
    break;
  }
  a('\0');
#undef a
#undef as
#undef sync
  return pwnstr_to_str(result);
}

static inline int int32_compare(const void *i, const void *j)
{
  return (*(int32_t*)i)-(*(int32_t*)j);
}

static inline void uint32qsort(uint32_t* offsets, size_t count)
{
  qsort(offsets, count, 4, int32_compare);
}

#if 0
static inline void uint32isort(int32_t* offsets, unsigned int count)
{
  unsigned int i, j;
  int32_t tmp;
  for (i=2; i<count; i++)
  for (j=i-1; j!=(unsigned int)-1; j--)
  {
    if (offsets[j] <= offsets[j+1])
      break;
    tmp = offsets[j+1];
    offsets[j+1] = offsets[j];
    offsets[j] = tmp;
  }
}
#endif

inline void version(void)
{
  fprintf(stderr,
    "pwnsjp, version %s\n\n",
    K_VERSION);
}

inline void usage(void)
{
  fprintf(stderr,
    "Usage: pwnsjp [OPTIONS] PATTERN\n\n"
    "Options:\n"
    "  -d, --deep\n"
    "  -e, --entry-only\n"
    "  -h, --help\n"
    "  -v, --version\n\n");
}

void eabort(char* fstr, int line, char* errstr)
{
  if (fstr == NULL)
    fstr = "?";
  if (errstr == NULL)
    errstr = strerror(errno);
  fprintf(stderr, "%s[%d]: %s\n", fstr, line, errstr);
  exit(EXIT_FAILURE);
}

inline void regex_free(regex_t *regex)
{
  regfree(regex);
}

inline bool regex_compile(regex_t *regex, char* pattern)
{
  if (pattern != NULL)
  {
    debug("pattern = \"%s\"\n", pattern);
    return
      regcomp(regex, pattern, REG_NOSUB | REG_EXTENDED | REG_ICASE) == 0;
  }
  debug("pattern = NULL\n");
  return true;
}

inline bool regex_match(regex_t *regex, char *string)
{
  return regexec(regex, string, 0, NULL, 0) == 0;
}
      
int main(int argc, char **argv)
{
  struct 
  {
    uint32_t word_count;
    uint32_t index_base;
    uint32_t words_base;
  } header;
  unsigned int i;
 
  term_init();

  char* pattern = parse_options(argc, argv);
  switch (config.action)
  {
  case action_help:
    usage();
    return EXIT_SUCCESS;
  case action_version:
    version();
    return EXIT_SUCCESS;
  case action_seek:
    ;
  }

  unicode_init();

#define try(s) do { if (!(s)) eabort(__FILE__, __LINE__, ""); } while(0)
#define tri(s, err) do { if (!(s)) eabort(__FILE__, __LINE__, err); } while(0)
  
  regex_t regex;
  tri ( regex_compile(&regex, pattern), "Invalid pattern" );
 
  debug("data file = \"%s\"\n", K_DATA_PATH);
  FILE* f = fopen(K_DATA_PATH, "rb");
  try ( f != NULL );
  try ( fseek(f, 0, SEEK_END) == 0 );
  unsigned int size = ftell(f);
  debug("data file size = %u MiB\n", size>>20);
#ifdef K_VALIDATE_DATAFILE
  if (size != datafile_size)
  {
    fprintf(stderr, "Invalid data file size: %u, should be %u.\n", size, datafile_size);
    return EXIT_FAILURE;
  }
  {
    uint8_t sig[2];
    try ( fseek(f, 0, SEEK_SET) == 0 );
    if ( fread(sig, 1, sizeof(sig), f) != sizeof(sig) || sig[0]!='G' || sig[1]!='W' )
    {
      fprintf(stderr, "Invalid data file signature: %02x%02x, should be 4787.\n", sig[0], sig[1]);
      return EXIT_FAILURE;
    }
  }
#else
  tri ( size > (1<<26), "Unexpectedly short data file" );
  tri ( size < (1<<28), "Unexpectedly long data file" );
#endif
  try ( fseek(f, 0x18, SEEK_SET) == 0 );
  try ( fread(&header, sizeof(header), 1, f) == 1 );

  header.word_count = le2cpu(header.word_count);
  debug("word count = %d\n", header.word_count);

  header.index_base = le2cpu(header.index_base);
  debug("index #1 base = 0x%08x\n", header.index_base);
  
  header.index_base += sizeof(uint32_t)*header.word_count;
  debug("index #2 base = 0x%08x\n", header.index_base);
  
  header.words_base = le2cpu(header.words_base);
  debug("words base = 0x%08x\n", header.words_base);
  tri ( header.word_count >= (1<<15), "Unexpectedly few words" );
  tri ( header.word_count <= (1<<17), "Unexpectedly many words" );
  
  uint32_t offsets[header.word_count];
  try ( fseek(f, header.index_base, SEEK_SET) >= 0 );
  try ( fread(offsets, sizeof(uint32_t), header.word_count, f) == header.word_count );
 
  for (i=0; i<header.word_count; i++)
    offsets[i] = le2cpu(offsets[i]) & 0x07ffffff;
  uint32qsort(offsets, header.word_count);

  unsigned int maxsize=1024;
  for (i=0; i<header.word_count-1; i++)
  {
    size=offsets[i+1]-offsets[i];
    if (size>maxsize)
      maxsize=size;
  }

  debug("max entry size = %u\n", maxsize);
  tri ( maxsize <= (1<<16), "Unexpectedly long entries" );
  
  char wordbuffer[(maxsize|0x07)+1];
  for (i=0; i<header.word_count-1; i++)
  {
    size=offsets[i+1]-offsets[i];
    unsigned long dsize = size << 3;
    bool zipped = false;
    char debuffer[dsize], *tbuffer;
    memset(debuffer, 0, dsize); 
    try ( fseek(f, header.words_base + offsets[i], SEEK_SET) == 0 );
    try ( fread(wordbuffer, size, 1, f) == 1 );
    char* localstr = wordbuffer + 12;
    bool dofree = false;
    if (!config.conf_quick)
    {
      localstr = pwnstr_to_str(localstr);
      dofree = true;
    }
    char* zipdata = wordbuffer + 12;
    zipdata += strlen(zipdata) + 2;
    if (*zipdata < ' ')
    {
      zipdata += (*zipdata) + 1;
      zipped = true;
    }
    if (config.conf_deep || !pattern || regex_match(&regex, localstr))
    {
      bool dofree = false;
      debug(
        "\b\b<\n  localstr = %s\n  offset = %08x : %08x\n>\n", 
        localstr, 
        offsets[i],
        header.words_base + offsets[i]);
      if (config.conf_entry_only)
        tbuffer = localstr;
      else 
      {
        if (zipped)
        {
          uncompress((unsigned char*)debuffer, &dsize, (unsigned char*)zipdata, dsize);
          tbuffer = debuffer;
        }
        else
          tbuffer = zipdata;
        if (!config.conf_raw)
        {
          tbuffer = trim_html(tbuffer);
          dofree = true;
        }
      }
      if (!config.conf_deep || !pattern || regex_match(&regex, tbuffer))
        printf("%s\n", tbuffer);
      if (dofree)
        free(tbuffer);
    }
    if (dofree)
      free(localstr);
  }
  
  try ( fclose(f) == 0 );

  if (pattern)
    regex_free(&regex);

#undef try
#undef try_s

}

// vim: ts=2 sw=2 et
