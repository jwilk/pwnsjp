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
#include "pwnio.h"
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

static void version(void)
{
  fprintf(stderr,
    "pwnsjp, version %s\n\n",
    K_VERSION);
}

static void usage(void)
{
  fprintf(stderr,
    "Usage: pwnsjp [OPTIONS] PATTERN\n\n"
    "Options:\n"
    "  -d, --deep\n"
    "  -e, --entry-only\n"
    "  -h, --help\n"
    "  -v, --version\n\n");
}

static void eabort(char* fstr, int line, char* errstr)
{
  if (fstr == NULL)
    fstr = "?";
  if (errstr == NULL)
    errstr = strerror(errno);
  fprintf(stderr, "%s[%d]: %s\n", fstr, line, errstr);
  exit(EXIT_FAILURE);
}

static void regex_free(regex_t *regex)
{
  regfree(regex);
}

static bool regex_compile(regex_t *regex, char* pattern)
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

static bool regex_match(regex_t *regex, char *string)
{
  return regexec(regex, string, 0, NULL, 0) == 0;
}
      
int main(int argc, char **argv)
{
 
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
 
  struct pwnio_t pwnio;
  if (!pwnio_init(&pwnio, K_DATA_PATH))
  {
    fprintf(stderr, "Unable to open data file.\n");
    return EXIT_FAILURE;
  }
  
#ifdef K_VALIDATE_DATAFILE
  if (pwnio.file_size != datafile_size)
  {
    fprintf(stderr, "Invalid data file size: %u, should be %u.\n", pwnio.file_size, datafile_size);
    return EXIT_FAILURE;
  }
  if (!pwnio_validate(&pwnio))
  {
    fprintf(stderr, "Invalid data file signature.\n");
    return EXIT_FAILURE;
  }
#else
  tri ( pwnio.size > (1<<26), "Unexpectedly short data file" );
  tri ( pwnio.size < (1<<28), "Unexpectedly long data file" );
#endif

  if (!pwnio_prepareindex(&pwnio))
  {
    fprintf(stderr, "Unable to prepare index.\n");
    return EXIT_FAILURE;
  }
  
  tri ( pwnio.word_count >= (1<<15), "Indecently few words" );
  tri ( pwnio.word_count <= (1<<17), "Indecently many words" );

  if (!pwnio_buildindex(&pwnio))
  {
    fprintf(stderr, "Unable to build index.\n");
    return EXIT_FAILURE;
  }
 
  char wordbuffer[pwnio.max_entry_size];
  if (!config.conf_tabi)
  for (i=0; i<pwnio.word_count-1; i++)
  {
    unsigned int size = pwnio.offsets[i+1]-pwnio.offsets[i];
    unsigned long dsize = size << 3;
    bool zipped = false;
    char debuffer[dsize], *tbuffer;
    memset(debuffer, 0, dsize); 
    try ( fseek(pwnio.file, pwnio.header->words_base + pwnio.offsets[i], SEEK_SET) == 0 );
    try ( fread(wordbuffer, size, 1, pwnio.file) == 1 );
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
        "\b\b<\n  localstr = %s\n  offset = %08x (file:%08x)\n  length = %06x\n>\n", 
        localstr, 
        pwnio.offsets[i],
        pwnio.header->words_base + pwnio.offsets[i],
        size);
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

  try ( pwnio_fine(&pwnio) );
  
  if (pattern)
    regex_free(&regex);

#undef try
#undef try_s

}

// vim: ts=2 sw=2 et
