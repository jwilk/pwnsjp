#include "common.h"
#include "html.h"

#include "hue.h"
#include "terminfo.h"
#include "unicode.h"

char* html_strip(char *str)
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
  bool ignorep = true;
  bool color = false;
  
  head=tail=str;
  appendix=result;
#define a(t) do *(appendix++)=t; while (0)
#define as(t) do { strcpy(appendix, t); while (*appendix) appendix++; } while (0)
#define ac(t) do { strcpy(appendix, hueset[t]); while (*appendix) appendix++; } while (0)
#define sync do head = tail+1; while(0)
  for (head=tail=str; *tail; tail++)
  switch(state)
  {
  case s_default:
    if (*tail == '<')
      state = s_html_vague;
    else
    {
      ignorep = false;
      a(*tail);
    }
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
      if (!strcasecmp(head, "p"))
      {
        if (ignorep)
          ignorep=false;
        else
          a('\n');
      }
      else if ((ignorep=false) || !strcasecmp(head, "p style=\"tab\""))
        as("\n   ");
      else if (!strcasecmp(head, "br"))
        a('\n');
      else if (!strcasecmp(head, "b"))
      {
        if (first)
        {
          ac(HUE_highlight);
          first = false;
        }
        else
          ac(HUE_bold);
      }
      else if (!strcasecmp(head, "tr1") && !color)
      {
        ac(HUE_highlight);
        color = true;
      }
      else if (!strcasecmp(head, "font color=#ff0000"))
      {
        ac(HUE_phraze);
        color = true;
      }
/*    else if (!strcasecmp(head, "font color=#fa8d00"))
      {
        ac(HUE_misc);
        color = true;
      } */
      else if (!strcasecmp(head, "i") && !color)
      {
        ac(HUE_italic);
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
          ac(HUE_hyperlink);
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
        ac(HUE_default);
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

// vim: ts=2 sw=2 et
