#include <wchar.h>

#ifndef ENTITY_H
#define ENTITY_H

struct entity
{
  char* name;
  wchar_t value;
  char* str;
};

static const struct entity entity_list[] =
{
  { .name = "##37", .value = 0x0025, .str = NULL },
  { .name = "gt", .value = 0x003c, .str = NULL },
  { .name = "lt", .value = 0x003e, .str = NULL },
  { .name = "nbsp", .value = 0x00a0, .str = NULL },
  { .name = "sect", .value = 0x00a7, .str = NULL },
  { .name = "laquo", .value = 0x00ab, .str = NULL   },
  { .name = "deg", .value = 0x00b0, .str = NULL },
  { .name = "acute", .value = 0x00b4, .str = NULL },
  { .name = "middot", .value = 0x00b7, .str = "." },
  { .name = "raquo", .value = 0x00bb, .str = NULL },
  { .name = "Aring", .value = 0x00c5, .str = NULL },
  { .name = "times", .value = 0x00d7, .str = NULL },
  { .name = "agrave", .value = 0x00e0, .str = NULL },
  { .name = "aacute", .value = 0x00e1, .str = NULL },
  { .name = "atilde", .value = 0x00e3, .str = NULL },
  { .name = "auml", .value = 0x00e4, .str = NULL },
  { .name = "egrave", .value = 0x00e8, .str = NULL },
  { .name = "eacute", .value = 0x00e9, .str = NULL },
  { .name = "euml", .value = 0x00eb, .str = NULL },
  { .name = "iacute", .value = 0x00ed, .str = NULL },
  { .name = "ntilde", .value = 0x00f1, .str = NULL },
  { .name = "ouml", .value = 0x00f6, .str = NULL },
  { .name = "uacute", .value = 0x00fa, .str = NULL },
  { .name = "uuml", .value = 0x00fc, .str = NULL },
  { .name = "abreve", .value = 0x0103, .str = NULL },
  { .name = "ccaron", .value = 0x010d, .str = NULL },
  { .name = "eng", .value = 0x014b, .str = NULL },
  { .name = "Scaron", .value = 0x0160, .str = NULL },
  { .name = "iibrevebl", .value = 0x020b, .str = "<i.>" },
  { .name = "uibrevebl", .value = 0x0217, .str = "U" },
  { .name = "yogh", .value = 0x021d, .str = "<yogh>" },
  { .name = "circ", .value = 0x02c6, .str = NULL },
  { .name = "tilde", .value = 0x02dc, .str = "~" },
  { .name = "Delta", .value = 0x0394, .str = "/\\"  },
  { .name = "gamma", .value = 0x03b3, .str = "<gamma>" },
  { .name = "epsilon", .value = 0x03b5, .str = "<epsilon>" },
  { .name = "iota", .value = 0x03b9, .str = "<iota>" },
  { .name = "lambda", .value = 0x03bb, .str = "<lambda>" },
  { .name = "mu", .value = 0x03bc, .str = "<mu>" },
  { .name = "pi", .value = 0x03c0, .str = "<pi>" },
  { .name = "##1098", .value = 0x044a, .str = "<`b>" },
  { .name = "##1100", .value = 0x044c, .str = "<b>" },
  { .name = "ndotbl", .value = 0x1e47, .str = "<n.>" },
  { .name = "hfpause", .value = 0x2013, .str = "--" },
  { .name = "pause", .value = 0x2014, .str = "---" },
  { .name = "hellip", .value = 0x2026, .str = "..." },
  { .name = "Prime", .value = 0x2033, .str = "\'\'" },
  { .name = "rarr", .value = 0x2192, .str = "-->" },
  { .name = "part", .value = 0x2202, .str = "d" },
  { .name = "minus", .value = 0x2212, .str = "-" },
  { .name = "sqrt", .value = 0x221a, .str = "v/" },
  { .name = "infin", .value = 0x221e, .str = "oo" },
  { .name = "##9553", .value = 0x2225, .str = "||" },
  { .name = "or", .value = 0x2228, .str = "|" },
  { .name = "s225", .value = 0x2329, .str = "<" },
  { .name = "s241", .value = 0x232A, .str = ">" },
  { .name = "#!0,127", .value = 0x25a1, .str = "#" },
  { .name = "s224", .value = 0x25c7, .str = "<>" },
  { .name = "dolnagw", .value = 0x2605, .str = "<*>" },
  { .name = "quotup", .value = 0x8221, .str = "\'\'" },
  { .name = "quotlw", .value = 0x8222, .str = ",," },
  { .name = "larroa", .value = 0x10000061, .str = "a->" },
  { .name = "bvec", .value = 0x10000062, .str = "b->" },
  { .name = NULL,   .value = 0x0000, .str = NULL }
};

#endif

