AC_PREREQ([2.59])
AC_INIT([pwnsjp], [0.700], [jwilk@jwilk.net])

AC_PROG_CC_C99

AX_WITH_CURSES
if test "$ax_cv_ncursesw" != yes; then
    AC_MSG_ERROR([the ncursesw library is required])
fi

PKG_CHECK_MODULES([ZLIB], [zlib])

AC_ARG_WITH([dictdir],
    [AS_HELP_STRING
        [--with-dictdir=DIR],
        [directory with dictionary files @<:@default=DATADIR/pwnsjp@:>@]
    )],
    [dictdir="$with_dictdir"],
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
