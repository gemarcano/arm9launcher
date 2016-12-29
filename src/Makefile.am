include $(top_srcdir)/common.mk

noinst_PROGRAMS = arm9loaderhax arm9loaderhax.bin
arm9loaderhax_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/arm9loaderhax.ld -I$(prefix)/include -I$(top_srcdir)/ext
arm9loaderhax_LDFLAGS=$(AM_LDFLAGS)
arm9loaderhax_SOURCES = loader.c a9l_config.h a9l_config.c
arm9loaderhax_LDADD = -L$(prefix)/lib -lctr9 -lctrelf -lfreetype $(top_builddir)/ext/libjsmn.la

EXTRA_DIST = arm9loaderhax.ld

.PHONY: FORCE

FORCE:

arm9loaderhax.bin: arm9loaderhax$(EXEEXT)
	cp $^ $@
	$(STRIP_BIN) $@


$(top_builddir)/ext/libjsmn.la: FORCE
	(cd $(top_builddir)/ext/ && $(MAKE) $(AM_MAKEFLAGS) libjsmn.la )

