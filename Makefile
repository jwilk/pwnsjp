M_VERSION = 0.340

M_BUILD_HEADERS = yes	# yes | no
M_DEBUG = yes			# yes | no
M_PROFILE = no			# yes | no
M_COMPILER = gcc		# gcc | icc

FAKEROOT := $(shell which fakeroot | head -1)
CTAGS := $(shell which ctags | head -1)

include Makefile.conf

CC = $(strip ${M_COMPILER})
CFLAGS = \
	$(CFLAGS_opt) $(CFLAGS_wrn) $(CFLAGS_std) $(CFLAGS_dbg)

CFLAGS_opt := -O3 -fstrict-aliasing -finline-limit=1200
CFLAGS_ld  := -lz -lncursesw
CFLAGS_dbg := -DNDEBUG
STRIP = strip -s
ifeq ($(strip ${M_DEBUG}),yes)
	CFLAGS_opt := -O0
	CFLAGS_dbg := -g
	STRIP = @true
endif
ifeq ($(strip ${M_COMPILER}),gcc)
	CFLAGS_wrn := -W -Wall -Winline
	CFLAGS_std := --std=gnu99
	ifeq ($(strip ${M_PROFILE}),yes)
		CFLAGS_dbg += -pg -DNDEBUG
		STRIP = true
	endif
endif
ifeq ($(strip ${M_COMPILER}),icc)
	CFLAGS_wrn := -w
	CFLAGS_std := -c99
endif

CFLAGS_def =
CFLAGS_def += -DK_VERSION='"$(strip ${M_VERSION})"'
ifdef K_DATA_PATH
	CFLAGS_def += -DK_DATA_PATH="\"$(strip {K_DATA_PATH})\""
endif
ifeq ($(strip ${K_VALIDATE_DATAFILE}),yes)
	CFLAGS_def += -DK_VALIDATE_DATAFILE
endif

HFILES = $(wildcard *.h)
CFILES = $(wildcard *.c)
OFILES = $(CFILES:.c=.o)
MAKEFILES = Makefile $(wildcard Makefile.*)
SOURCEFILES = $(CFILES) $(HFILES) $(MAKEFILES)

DISTFILES = README $(SOURCEFILES) script/ data/

all: pwnsjp pwnsjpi tags

tags: $(CFILES) $(HFILES)
ifneq ($(CTAGS),)
	@echo 'ctags *.c *.h'
	@ctags ${^}
else
	@touch ${@}
endif
		
clean:
	rm -f pwnsjp pwnsjpi *.da *.s *.o gmon.out a.out pwnsjp-*.tar* tags

ui-test: pwnsjpi
	LC_ALL=pl_PL ./pwnsjpi

test: pwnsjp
	./pwnsjp --debug '^ab(neg|om)'

dist: $(HFILES)
	$(FAKEROOT) tar -chjf pwnsjp-$(M_VERSION).tar.bz2 $(DISTFILES)

include Makefile.dep

%.o:
	@echo $(CC) '(...)' -c ${<} -o ${@}
	@$(CC) $(CFLAGS) $(CFLAGS_def) -c ${<} -o ${@}

pwnsjpi: pwnsjp
	ln -sf pwnsjp pwnsjpi

pwnsjp: $(OFILES)
	@echo $(CC) '(...)' $(CFLAGS_ld) ${^} -o ${@}
	@$(CC) $(CFLAGS) $(CFLAGS_ld) ${^} -o ${@}
	$(STRIP) ${@}

ifeq ($(strip ${M_BUILD_HEADERS}),yes)
include Makefile.hdr
endif


stats:
	@echo $(shell cat ${SOURCEFILES} script/* | wc -l) lines.
	@echo $(shell cat ${SOURCEFILES} script/* | wc -c) bytes.

.PHONY: all clean test ui-test dist stats

# vim:ts=4
