AC_PREREQ([2.61])
AC_INIT([pwnsjp], [0.8.1], [jwilk@jwilk.net])

AC_PROG_CC_C99
AC_C_BIGENDIAN
AX_CFLAGS_WARN_ALL

PKG_CHECK_MODULES([NCURSESW], [ncursesw])
PKG_CHECK_MODULES([ZLIB], [zlib])

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

dnl vim: ts=4 sw=4 et
