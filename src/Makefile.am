include $(top_srcdir)/common.mk

noinst_PROGRAMS = arm9loaderhax
arm9loaderhax_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/arm9loaderhax.ld -nostartfiles -I$(CTRARM9)/include -I$(top_srcdir)/ext
arm9loaderhax_LDFLAGS=$(AM_LDFLAGS) -lctr
arm9loaderhax_SOURCES = loader.c start.s a9l_config.h a9l_config.c elf.c elf.h
arm9loaderhax_LDADD = -L$(CTRARM9)/lib -lctr9 -lctr -lctrelf $(top_builddir)/ext/libjsmn.la

EXTRA_DIST = arm9loaderhax.ld

.PHONY: FORCE

FORCE:

$(top_builddir)/ext/libjsmn.la: FORCE
	(cd $(top_builddir)/ext/ && $(MAKE) $(AM_MAKEFLAGS) libjsmn.la )

