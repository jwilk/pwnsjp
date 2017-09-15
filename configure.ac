dnl | Copyright © 2012-2017 Jakub Wilk
dnl |
dnl | Permission is hereby granted, free of charge, to any person obtaining a
dnl | copy of this software and associated documentation files (the
dnl | “Software”), to deal in the Software without restriction, including
dnl | without limitation the rights to use, copy, modify, merge, publish,
dnl | distribute, sublicense, and/or sell copies of the Software, and to
dnl | permit persons to whom the Software is furnished to do so, subject to
dnl | the following conditions:
dnl |
dnl | The above copyright notice and this permission notice shall be included
dnl | in all copies or substantial portions of the Software.
dnl |
dnl | THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
dnl | OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl | MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
dnl | IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
dnl | CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
dnl | TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
dnl | SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_PREREQ([2.61])
AC_INIT([pwnsjp], [0.9.1], [jwilk@jwilk.net])

AC_PROG_CC_C99
AC_C_BIGENDIAN
AX_CFLAGS_WARN_ALL

PKG_CHECK_MODULES(
    [NCURSESW],
    [ncursesw],
    [],
    [NCURSESW_CFLAGS=-I/usr/include/ncursesw; NCURSESW_LIBS=-lncursesw]
)
PKG_CHECK_MODULES(
    [ZLIB],
    [zlib],
    [],
    [ZLIB_CFLAGS=; ZLIB_LIBS=-lz]
)

AC_ARG_WITH([dictdir],
    [AS_HELP_STRING(
        [--with-dictdir=DIR],
        [directory with dictionary files @<:@default=DATADIR/pwnsjp@:>@]
    )],
    [dictdir="${with_dictdir%/}"],
    [dictdir="\${datadir}/pwnsjp"],
)
AC_SUBST([dictdir])

AC_CHECK_PROGS([XSLTPROC], [xsltproc])
if test -n "$XSLTPROC"; then
    AC_CACHE_CHECK([for DocBook XSL stylesheet], [_cv_docbook_xsl], [
        _cv_docbook_xsl=http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl
        AC_RUN_LOG([$XSLTPROC --nonet "$_cv_docbook_xsl"]) || _cv_docbook_xsl=no
    ])
    if test "$_cv_docbook_xsl" = no; then
        _cv_docbook_xsl=
        AC_MSG_WARN([DocBook XSL stylesheets are needed to build the documentation])
    fi
    AC_SUBST([DOCBOOK_XSL], ["$_cv_docbook_xsl"])
else
    AC_MSG_WARN([xsltproc is needed to build the documentation])
fi

AC_CONFIG_HEADER([autoconf.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([

The following flags will be used to link to external libraries:
NCURSESW_CFLAGS = $NCURSESW_CFLAGS
NCURSESW_LIBS = $NCURSESW_LIBS
ZLIB_CFLAGS = $ZLIB_CFLAGS
ZLIB_LIBS = $ZLIB_LIBS
])

dnl vim: ts=4 sts=4 sw=4 et
