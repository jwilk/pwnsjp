M_VERSION = 0.355

M_BUILD_HEADERS = yes	# yes | no
M_DEBUG = no			# yes | no
M_PROFILE = no			# yes | no
M_COMPILER = gcc		# gcc | icc

FAKEROOT := $(shell which fakeroot | head -1 2>/dev/null)
CTAGS := $(shell which ctags | head -1 2>/dev/null)

include Makefile.conf

CC = $(strip ${M_COMPILER})
CFLAGS = \
	$(CFLAGS_opt) $(CFLAGS_wrn) $(CFLAGS_std) $(CFLAGS_dbg)

CFLAGS_opt := -O3 -fstrict-aliasing
CFLAGS_ld  := -lz -lncursesw
CFLAGS_dbg := -DNDEBUG
STRIP = strip -s
ifeq ($(strip ${M_DEBUG}),yes)
	CFLAGS_opt := -O0
	CFLAGS_dbg := -g
	STRIP = @true
endif
ifeq ($(strip ${M_COMPILER}),gcc)
	CFLAGS_opt += -finline-limit=1200
	CFLAGS_wrn := -W -Wall -Winline
	CFLAGS_std := --std=gnu99
	ifeq ($(strip ${M_PROFILE}),yes)
		CFLAGS_dbg += -pg -DNDEBUG
		STRIP = @true
	endif
endif
ifeq ($(strip ${M_COMPILER}),icc)
	CFLAGS_opt += -static-libcxa -ansi_alias
	CFLAGS_wrn := -w
	CFLAGS_std := -c99
	ifeq ($(strip ${M_PROFILE}),yes)
		CFLAGS_dbg += -prof_gen -DNDEBUG
		STRIP = @true
	else
		CFLAGS_opt += -ipo
	endif
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

.PHONY: all
all: pwnsjp pwnsjpi tags

.PHONY: clean
clean:
	rm -f pwnsjp pwnsjpi *.da *.il *.dyn *.s *.o *.out pwnsjp-*.tar* tags

tags: $(CFILES) $(HFILES)
ifneq ($(CTAGS),)
	@echo 'ctags *.c *.h'
	@ctags ${^}
else
	@touch ${@}
endif

include Makefile.dep

pwnsjpi: pwnsjp
	ln -sf pwnsjp pwnsjpi

pwnsjp: $(OFILES)
	@echo CFLAGS = $(CFLAGS)
	@echo $(CC) '(...)' $(CFLAGS_ld) ${^} -o ${@}
	@$(CC) $(CFLAGS) $(CFLAGS_ld) ${^} -o ${@}
	$(STRIP) ${@}

$(OFILES): %.o: %.c
	@echo $(CC) '(...)' -c ${<} -o ${@}
	@$(CC) $(CFLAGS) $(CFLAGS_def) -c ${<} -o ${@}

ifeq ($(strip ${M_BUILD_HEADERS}),yes)
include Makefile.hdr
endif

.PHONY: stats
stats:
	@echo $(shell cat ${SOURCEFILES} script/* | wc -l) lines.
	@echo $(shell cat ${SOURCEFILES} script/* | wc -c) bytes.

.PHONY: ui-test
ui-test: pwnsjpi
	LC_ALL=pl_PL ./pwnsjpi

.PHONY: test
test: pwnsjp
	./pwnsjp --debug '^ab(neg|om)'

.PHONY: dist
dist: $(HFILES)
	$(FAKEROOT) tar -chjf pwnsjp-$(M_VERSION).tar.bz2 $(DISTFILES)

# vim:ts=4
