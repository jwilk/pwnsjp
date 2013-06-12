AC_PREREQ([2.61])
AC_INIT([pwnsjp], [0.8.1], [jwilk@jwilk.net])

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
if test -z "$XSLTPROC"; then
    AC_MSG_WARN([xsltproc is needed to build the documentation])
fi

AC_CONFIG_HEADER([setup.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([

The following flags will be used to link to external libraries:
NCURSESW_CFLAGS = $NCURSESW_CFLAGS
NCURSESW_LIBS = $NCURSESW_LIBS
ZLIB_CFLAGS = $ZLIB_CFLAGS
ZLIB_LIBS = $ZLIB_LIBS
])

dnl vim: ts=4 sw=4 et
