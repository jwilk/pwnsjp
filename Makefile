M_VERSION = $(shell date '+0.%Y%m%d')

M_BUILD_HEADERS = yes	# yes | no
M_DEBUG = no			# yes | no
M_PROFILE = no			# yes | no
M_COMPILER = gcc		# gcc | icc

include Makefile.conf

CC = $(strip ${M_COMPILER})
CFLAGS = \
	$(CFLAGS_opt) $(CFLAGS_wrn) $(CFLAGS_std) $(CFLAGS_dbg)

CFLAGS_opt := -O3
CFLAGS_ld  := -lz -lncurses
CFLAGS_dbg :=
STRIP = strip -s
ifeq ($(strip ${M_DEBUG}),yes)
	CFLAGS_opt := -O0
	CFLAGS_dbg += -g
	STRIP = true
endif
ifeq ($(strip ${M_COMPILER}),gcc)
	CFLAGS_wrn := -W -Wall
	CFLAGS_std := --std=gnu99
	ifeq ($(strip ${M_PROFILE}),yes)
		CFLAGS_dbg += -pg
		STRIP = true
	endif
endif
ifeq ($(strip ${M_COMPILER}),icc)
	CFLAGS_wrn :=
	CFLAGS_std := -c99
endif

CFLAGS_def = -DK_VERSION="\"$(strip ${M_VERSION})\""
ifdef K_DATA_PATH
	CFLAGS_def += -DK_DATA_PATH="\"$(strip {K_DATA_PATH})\""
endif
ifeq ($(strip ${K_VALIDATE_DATAFILE}),yes)
	CFLAGS_def += -DK_VALIDATE_DATAFILE
endif

HFILES = \
	cmap-cp1250.h cmap-iso8859-16.h cmap-iso8859-2.h cmap-usascii.h \
	entity.h entity-hash.h \
	validate.h byteorder.h \
	config.h terminfo.h unicode.h \
	common.h

CFILES = config.c pwnsjp.c terminfo.c unicode.c
OFILES = $(CFILES:.c=.o)

DISTFILES = Makefile Makefile.conf $(HFILES) $(CFILES) script/ data/

all: pwnsjp

clean:
	rm -f pwnsjp *.o gmon.out a.out

test: pwnsjp
	./pwnsjp --debug 'ab(neg|om)'

distclean:
	rm -f pwnsjp-*.tar.*

dist: distclean $(HFILES)
	fakeroot tar -chjf pwnsjp-$(M_VERSION).tar.bz2 $(DISTFILES)

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) $(CFLAGS_def) -c ${<} -o ${@}

pwnsjp: $(OFILES)
	$(CC) $(CFLAGS) $(CFLAGS_ld) ${^} -o ${@}
	$(STRIP) ${@}

ifeq ($(strip ${M_BUILD_HEADERS}),yes)

cmap-cp1250.h: data/cmap-cp1250.gz script/cmap-cp1250.h.in
	zcat ${<} | script/cmap-cp1250.h.in > ${@}

cmap-iso8859-2.h: data/cmap-iso8859-2.gz script/cmap-iso8859-n.h.in
	zcat ${<} | script/cmap-iso8859-n.h.in 2 > ${@}

cmap-iso8859-16.h: data/cmap-iso8859-16.gz script/cmap-iso8859-n.h.in
	zcat ${<} | script/cmap-iso8859-n.h.in 16 > ${@}

cmap-usascii.h: script/cmap-usascii.h.in
	script/cmap-usascii.h.in > ${@}

entity.h: data/entities.dat script/entity.h.in
	script/entity.h.in < ${<} > ${@}

entity-hash.h: data/entities.dat script/entity-hash.h.in
	script/entity-hash.h.in < ${<} > ${@}
	
endif
	
.PHONY: all clean test dist distclean

# vim:ts=4
