#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>

#include <langinfo.h>
#include <locale.h>

#include <fcntl.h>
#include <sys/types.h>

#include <regex.h>
#include <string.h>
#include <wchar.h>

#include <zlib.h>

#include <endian.h>
#include <byteswap.h>

typedef int bool;
#define true 1
#define false 0

#ifndef K_DATA_PATH
#  define K_DATA_PATH "./slo.win"
#endif
#ifndef K_VERSION
#  define K_VERSION "<devel>"
#endif

#include "cmap-cp1250.h"
#include "cmap-usascii.h"
#include "cmap-iso8859-2.h"

#include "entity.h"
#include "entity-hash.h"

static struct
{
  enum
  {
    action_seek = 0,
    action_help,
    action_version
  } action;
  bool conf_debug;
  bool conf_deep;
  bool conf_entry_only;
  bool conf_quick;
  bool conf_raw;
  bool conf_color;
} config;

static inline void debug(const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  if (config.conf_debug)
  {
    fprintf(stderr, "| ");
    vfprintf(stderr, message, ap);
  }
  va_end(ap);
}

static enum { cmap_usascii, cmap_iso88592, cmap_utf8 } cmap = cmap_usascii;

static char* pwnstr_to_str(const char *str);

static inline const char* color(const char *str)
{
  if (config.conf_color)
    return str;
  else
    return "";
}

static char* trim_html(char *str)
// Warning: characters of `str' are destroyed!
{
  enum
  {
    s_default,
    s_html_vague,
    s_html_open,
    s_html_params,
    s_html_close
  } state = s_default;
  char *head, *tail, *appendix;
  int len = strlen(str);
  char result[2*len];
  bool first = true;
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
    if (*tail=='/')
    {
      state = s_html_close;
      head++;
    }
    else
      state = s_html_open;
    break;
  case s_html_open:
    if (*tail==' ' || *tail=='>') 
    {
      state=(*tail==' ')?s_html_params:s_default;
      *tail='\0';
      if (!strcasecmp(head, "p") || !strcasecmp(head, "br"))
        a('\n');
      else if (!strcasecmp(head, "b"))
      {
        if (first)
        {
          as(color("\x1b[45m"));
          first = false;
        }
        as(color("\x1b[1m"));
      }
      sync;
    }
    break;
  case s_html_params:
    if (*tail == '>')
    {
      state = s_default;
      *tail='\0';
      if (!strcasecmp(head, "style=\"tab\""))
        as("   ");
      sync;
    }
    break;
  case s_html_close:
    if (*tail == '>')
    {
      *tail = '\0';
      state = s_default;
      if (!strcasecmp(head, "b"))
        as(color("\x1b[22;49m"));
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

static inline const char* entity_grep(const char *str, wchar_t *result)
{
  unsigned int hash = 0;
  int i, j;
  for(j=0; j<hash_coeff_count && str[j]!=0 && str[j]!=';'; j++)
    hash ^= str[j]+hash_coeff[j];
  if ( (i = entity_hash[hash % hash_size]) >= 0 )
  {
    *result = entity_list[i].value;
    return str+j;
  }
  return str;
}

static wchar_t* pwnstr_to_ustr(const char *str)
{
  size_t i;
  wchar_t* result = calloc(1+strlen(str), sizeof(wchar_t));
  for (i=0; *str; str++, i++)
  {
    result[i]=cp1250[(uint8_t)(*str)];
    if (result[i]=='&')
      str = entity_grep(str+1, result+i);
  }
  return result;
}

static inline char* ustr_fallback_ascii(const wchar_t *ustr, int us)
{
  int i, len = wcslen(ustr), biglen=4*(len+1);
  char result[biglen], *appendix;
  memset(result, 0, biglen); 
  appendix = result;
#define a(t) do *(appendix++)=t; while (0)
#define as(t) do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
  for (i=0; i<len; i++)
  {
    if (ustr[i] < 0x00a0)
      a(ustr[i]);
    else if (ustr[i] < 0x017f)
    {
      char code=0;
      if (us)
        code=rev_iso88592[ustr[i]-0x00a0];
      if (code)
        a(code);
      else
        as(rev_usascii[ustr[i]-0x00a0]);
    }
    else
    {
      char *ename;
      int j;
      for (j=0; (ename = entity_list[j].name); j++)
      if (ustr[i] == entity_list[j].value && entity_list[j].str)
      {
        as(entity_list[j].str);
        break;
      }
      if (!ename)
        a('?');
    }
  }
#undef a
#undef as
  return strdup(result);
}

static inline char* ustr_fallback_sys(const wchar_t *ustr)
{
  int lim = 1 + wcstombs(NULL, ustr, 0);
  char* result = malloc(lim);
  if (wcstombs(result, ustr, lim) == (size_t)(-1))
    return strdup("<wcstombs failed!>");
  else
    return result;
}

static char* ustr_to_str(const wchar_t *ustr)
{
  switch(cmap)
  {
    case cmap_usascii:
      return ustr_fallback_ascii(ustr, 0);
    case cmap_iso88592:
      return ustr_fallback_ascii(ustr, 1);
    case cmap_utf8:
      return ustr_fallback_sys(ustr);
  }
  return "<invalid character map>"; // this should not happen
}

static char* pwnstr_to_str(const char *str)
{
  wchar_t* ustr = pwnstr_to_ustr(str);
  char* result = ustr_to_str(ustr);
  free(ustr);
  return result;
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

static inline char* parse_options(int argc, char **argv)
{
  static struct option gopts[]=
  {
    { "debug",      0, 0, 'D' },
    { "deep",       0, 0, 'd' },
    { "entry-only", 0, 0, 'e' },
    { "help",       0, 0, 'h' },
    { "quick",      0, 0, 'Q' },
    { "raw",        0, 0, 'R' },
    { "version",    0, 0, 'v' },
    { NULL,         0, 0, '\0' }
  };

  memset(&config, sizeof(config), 0);
  if (isatty(STDOUT_FILENO))
    config.conf_color = true;
 
  while (true)
  {
    int i = 0;
    int c = getopt_long(argc, argv, "dehvDQR", gopts, &i);
    if (c < 0)
      break;
    if (c == 0)
      c = gopts[i].val;
    switch (c)
    {
      case 'd':
        config.conf_deep = true;
        break;
      case 'e':
        config.conf_entry_only = true;
        break;
      case 'h':
        config.action = action_help;
        break;
      case 'v':
        config.action = action_version;
        break;
      case 'D':
        config.conf_debug = true;
        break;
      case 'Q':
        config.conf_quick = true;
        break;
      case 'R':
        config.conf_raw = true;
        break;
     }
  }
  return
    (optind<=argc)?argv[optind]:NULL;
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

#if __BYTE_ORDER == __LITTLE_ENDIAN
#  define le2cpu(x) x
#elif __BYTE_ORDER == __BIG_ENDIAN
#  define le2cpu bswap_32
#endif

int main(int argc, char **argv)
{
  struct 
  {
    uint32_t word_count;
    uint32_t index_base;
    uint32_t words_base;
  } header;
  unsigned int i;
  
  char* match = parse_options(argc, argv);
  if (config.action == action_help)
  {
    usage();
    return EXIT_SUCCESS;
  }
  else if (config.action == action_version)
  {
    version();
    return EXIT_SUCCESS;
  }

  char *locale = setlocale(LC_ALL, "");
  if (locale)
  {
    debug("set locale to: <%s>\n", locale);
    char *codeset = nl_langinfo(CODESET);
    if (!strcmp(codeset, "UTF-8"))
      cmap = cmap_utf8;
    else if (!strcmp(codeset, "ISO-8859-2"))
      cmap = cmap_iso88592;
  }
  else
    debug("unable to set locale!");

#define try(s) do { if (!(s)) eabort(__FILE__, __LINE__, ""); } while(0)
#define tri(s, err) do { if (!(s)) eabort(__FILE__, __LINE__, err); } while(0)
  
  regex_t match_regex;
  if (match)
  {
    debug("pattern = \"%s\"\n", match);
    tri ( regcomp(&match_regex, match, REG_NOSUB | REG_EXTENDED | REG_ICASE) == 0, "Invalid pattern" );
  }
  else
    debug("pattern = NULL\n");
 
  debug("data file = \"%s\"\n", K_DATA_PATH);
  FILE* f = fopen(K_DATA_PATH, "rb");
  try ( f != NULL );
  try ( fseek(f, 0, SEEK_END) == 0 );
  unsigned int size = ftell(f);
  debug("data file size = %u MiB\n", size>>20);
  tri ( size > (1<<26), "Unexpectedly short data file" );
  tri ( size < (1<<28), "Unexpectedly long data file" );
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
      localstr=pwnstr_to_str(localstr);
      dofree=true;
    }
    char* zipdata=wordbuffer + 12;
    zipdata+=strlen(zipdata) + 2;
    if (*zipdata < ' ')
    {
      zipdata += (*zipdata) + 1;
      zipped = true;
    }
    if (!match || config.conf_deep || !regexec(&match_regex, localstr, 0, NULL, 0))
    {
      bool dofree = false;
      debug(
        "<\n  localstr = %s\n  offset = %08x : %08x\n>\n", 
        localstr, 
        offsets[i],
        header.words_base + offsets[i]);
      if (config.conf_entry_only)
        tbuffer=localstr;
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
      if (!config.conf_deep || !match || !regexec(&match_regex, tbuffer, 0, NULL, 0))
        printf("%s\n", tbuffer);
      if (dofree)
        free(tbuffer);
    }
    if (dofree)
      free(localstr);
  }
  
  try ( fclose(f) == 0 );

  if (match)
    regfree(&match_regex);

#undef try
#undef try_s

}

// vim: ts=2 sw=2 et
