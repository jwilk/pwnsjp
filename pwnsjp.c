#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <getopt.h>

#include <langinfo.h>
#include <locale.h>

#include <fcntl.h>
#include <sys/types.h>

#include <regex.h>
#include <string.h>
#include <wchar.h>

#include <zlib.h>

#include "cmap-cp1250.h"
#include "cmap-usascii.h"
#include "cmap-iso8859-2.h"

#include "entity.h"
#include "entity-hash.h"

#define CMAP_USASCII  0
#define CMAP_ISO88592 1
#define CMAP_UTF8     2

typedef int bool;
#define true 1
#define false 0

struct
{
  enum
  {
    action_seek,
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

inline void debug(const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  if (config.conf_debug)
    vfprintf(stderr, message, ap);
  va_end(ap);
}

int charmap = CMAP_USASCII;

char* pwnstr_to_str(const char *str);

inline const char* color(const char *str)
{
  if (config.conf_color)
    return str;
  else
    return "";
}

char* trim_html(char *str)
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

inline const char* entity_grep(const char *str, wchar_t *result)
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

wchar_t* pwnstr_to_ustr(const char *str)
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

inline char* ustr_fallback_ascii(const wchar_t *ustr, int us)
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
#undef append
  return strdup(result);
}

inline char* ustr_fallback_sys(const wchar_t *ustr)
{
  int lim = 1 + wcstombs(NULL, ustr, 0);
  char* result = malloc(lim);
  if (wcstombs(result, ustr, lim) == (size_t)(-1))
    return strdup("<wcstombs failed!>");
  else
    return result;
}

char* ustr_to_str(const wchar_t *ustr)
{
  switch(charmap)
  {
    case CMAP_USASCII:
      return ustr_fallback_ascii(ustr, 0);
    case CMAP_ISO88592:
      return ustr_fallback_ascii(ustr, 1);
    case CMAP_UTF8:
      return ustr_fallback_sys(ustr);
  }
  return strdup("<unknown charmap>");
}

char* pwnstr_to_str(const char *str)
{
  wchar_t* ustr = pwnstr_to_ustr(str);
  char* result = ustr_to_str(ustr);
  free(ustr);
  return result;
}

inline int int32_compare(const void *i, const void *j)
{
  return (*(int32_t*)i)-(*(int32_t*)j);
}

inline void uint32qsort(uint32_t* offsets, size_t count)
{
  qsort(offsets, count, 4, int32_compare);
}

inline void uint32sort(int32_t* offsets, int count)
{
  int i, j;
  int32_t tmp;
  printf("count = %d\n", count);
  for (i=1; i<count; i++)
  for (j=i-1; j>=0; j--)
  {
    if (offsets[j]<=offsets[j+1])
      break;
    tmp=offsets[j+1];
    offsets[j+1]=offsets[j];
    offsets[j]=tmp;
  }
}

inline char* parse_options(int argc, char **argv)
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
        config.action = action_seek;
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

int main(int argc, char **argv)
{
  struct 
  {
    uint32_t word_count;
    uint32_t index_base;
    uint32_t words_base;
  } header;
  unsigned int i;

  setlocale(LC_ALL, "");
  charmap = CMAP_USASCII;
  char *codeset = nl_langinfo(CODESET);
  if (!strcmp(codeset, "UTF-8"))
    charmap = CMAP_UTF8;
  else if (!strcmp(codeset, "ISO-8859-2"))
    charmap = CMAP_ISO88592;
  
  regex_t match_regex;
  char* match = parse_options(argc, argv);
  if (match)
    regcomp(&match_regex, match, REG_NOSUB | REG_EXTENDED | REG_ICASE);

  debug("looking for: %s\n", match);
  
  FILE* f = fopen("slo.win", "rb");
  fseek(f, 0x18L, SEEK_SET);
  fread(&header, sizeof(header), 1, f);

  debug("word count = %d\n", header.word_count);
  debug("index #1 base = 0x%08x\n", header.index_base);
  header.index_base += sizeof(uint32_t)*header.word_count;
  debug("index #2 base = 0x%08x\n", header.index_base);
  debug("words base = 0x%08x\n", header.words_base);
  
  uint32_t offsets[header.word_count+1];
  fseek(f, header.index_base, SEEK_SET);
  fread(offsets, sizeof(uint32_t), header.word_count, f);
  
  for (i=0; i<header.word_count; i++)
    offsets[i] &= 0x07ffffff;
  uint32qsort(offsets, header.word_count);

  unsigned int size, maxsize=1024;
  for (i=0; i<header.word_count-1; i++)
  {
    size=offsets[i+1]-offsets[i];
    if (size>maxsize)
      maxsize=size;
  }
  
  char wordbuffer[maxsize+1];
  for (i=0; i<header.word_count-2; i++)
  {
    size=offsets[i+1]-offsets[i];
    unsigned long dsize = 5*size;
    bool zipped = false;
    char debuffer[dsize], *tbuffer;
    memset(debuffer, 0, dsize); 
    fseek(f, header.words_base + offsets[i], SEEK_SET);
    fread(wordbuffer, 1, size, f);
    char* localstr=wordbuffer + 12;
    bool dofree = false;
    if (!config.conf_quick)
    {
      localstr=pwnstr_to_str(localstr);
      dofree=true;
    }
    char* zipdata=wordbuffer + 12;
    zipdata+=strlen(zipdata) + 2;
    if (*zipdata < 20)
    {
      zipdata += (*zipdata) + 1;
      zipped = true;
    }
    if (!match || config.conf_deep || !regexec(&match_regex, localstr, 0, NULL, 0))
    {
      bool dofree = false;
      debug(
        "<\n  localstr = %s\n  offset = %08x\n  fileoffset = %08x\n>\n", 
        localstr, 
        offsets[i],
        header.words_base + offsets[i]);
      if (config.conf_entry_only)
        tbuffer=localstr;
      else 
      {
        if (zipped)
        {
          uncompress(debuffer, &dsize, zipdata, dsize);
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
  
  fclose(f);

  if (match)
    regfree(&match_regex);
}

// vim: ts=2 sw=2 et
