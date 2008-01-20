VERSION = $(shell sed -n -e '1 s/.*(\([0-9.]*\)).*/\1/p' < debian/changelog)

M_BUILD_HEADERS = yes	# yes | no
M_DEBUG = no			# yes | no
M_PROFILE = no			# yes | no
M_COMPILER = gcc		# gcc | icc

FAKEROOT := $(shell command -v fakeroot | head -1 2>/dev/null)
CTAGS := $(shell command -v ctags | head -1 2>/dev/null)

include Makefile.conf

CC = $(strip $(M_COMPILER))
CFLAGS = \
	$(CFLAGS_opt) $(CFLAGS_wrn) $(CFLAGS_std) $(CFLAGS_dbg)

CFLAGS_def :=
CFLAGS_opt := -O3 -fstrict-aliasing
CFLAGS_ld  := -lz -lncursesw
CFLAGS_dbg := -DNDEBUG
STRIP = strip -s
ifeq ($(strip $(M_DEBUG)),yes)
	CFLAGS_opt := -O0
	CFLAGS_dbg := -g
	STRIP = @true
else
ifeq ($(CC),gcc)
	CFLAGS_opt += -fomit-frame-pointer
endif
endif
ifeq ($(CC),gcc)
	CFLAGS_opt += -finline-limit=1200
	CFLAGS_wrn := -W -Wall -Winline
	CFLAGS_std := --std=gnu99
	ifeq ($(strip $(M_PROFILE)),yes)
		CFLAGS_dbg += -pg -DNDEBUG
		STRIP = @true
	endif
endif
ifeq ($(CC),icc)
	CFLAGS_opt += -static-libcxa -ansi_alias
	CFLAGS_wrn := -w
	CFLAGS_std := -c99
	ifeq ($(strip $(M_PROFILE)),yes)
		CFLAGS_dbg += -prof_gen -DNDEBUG
		STRIP = @true
	else
		CFLAGS_opt += -ipo
	endif
	CFLAGS_def += -DICC
endif

CFLAGS_def += -DK_VERSION='"$(strip $(M_VERSION))"'
ifdef K_DATA_PATH
	CFLAGS_def += -DK_DATA_PATH="\"$(strip $(K_DATA_PATH))\""
else
	K_DATA_PATH := slo.win
endif

HFILES := $(wildcard *.h)
CFILES := $(wildcard *.c)
OFILES := $(CFILES:.c=.o)
MAKEFILES := Makefile $(wildcard Makefile.*)
SOURCEFILES := $(CFILES) $(HFILES) $(MAKEFILES)

DISTFILES := README COPYING $(SOURCEFILES) script/ data/

.PHONY: all
all: pwnsjp pwnsjpi tags

.PHONY: clean
clean:
	rm -f pwnsjp pwnsjpi core core.[0-9]* *.da *.il *.dyn *.s *.o *.out pwnsjp-*.tar* tags doc/*.[0-9]

tags: $(CFILES) $(HFILES)
ifneq ($(CTAGS),)
	@echo 'ctags *.c *.h'
	@ctags $(^)
else
	@touch $(@)
endif

include Makefile.dep

pwnsjpi: pwnsjp
	ln -sf pwnsjp pwnsjpi

pwnsjp: $(OFILES)
	@echo CFLAGS = $(CFLAGS)
	@echo $(CC) '(...)' $(^) -o ${@} $(CFLAGS_ld) 
	@$(CC) $(CFLAGS) $(^) -o ${@} $(CFLAGS_ld) 
	$(STRIP) ${@}

$(OFILES): %.o: %.c
	@echo $(CC) '(...)' -c $(<) -o $(@)
	@$(CC) $(CFLAGS) $(CFLAGS_def) -c $(<) -o $(@)

ifeq ($(strip $(M_BUILD_HEADERS)),yes)
include Makefile.hdr
endif

DB2MAN = /usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl
XSLTPROC = xsltproc

%.1: %.xml $(BD2MAN)
	sed -e '1,/^$$/ { s!\(slo\.win\)!$(strip ${K_DATA_PATH})/\1!; s!0\.devel!${VERSION}! }' $(<) | $(XSLTPROC) --output $(@) $(DB2MAN)  -

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

# vim:ts=4
