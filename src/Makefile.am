include $(top_srcdir)/common.mk

noinst_PROGRAMS = arm9loaderhax arm9launcher
arm9loaderhax_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/arm9loaderhax.ld -I$(prefix)/include -I$(top_srcdir)/ext
arm9loaderhax_LDFLAGS=$(AM_LDFLAGS) -L$(prefix)/lib
arm9loaderhax_SOURCES=loader.c a9l_config.h a9l_config.c
arm9loaderhax_LDADD=-lctr9 -lctr_core -lfreetype $(top_builddir)/ext/libjsmn.la

arm9launcher_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/bootloader.ld -I$(prefix)/include
arm9launcher_LDFLAGS=$(AM_LDFLAGS)
arm9launcher_SOURCES = arm9launcher.c a_start.s elf.c elf.h
arm9launcher_LDFLAGS=$(AM_LDFLAGS) -L$(prefix)/lib
arm9launcher_LDADD = -lctr9 -lctr_core -lctrelf -lfreetype

EXTRA_DIST = arm9loaderhax.ld bootloader.ld

all-local: arm9loaderhax.bin arm9launcher.bin
clean-local:
	rm -f arm9loaderhax.bin arm9launcher.bin

.PHONY: FORCE

FORCE:

$(top_builddir)/ext/libjsmn.la: FORCE
	(cd $(top_builddir)/ext/ && $(MAKE) $(AM_MAKEFLAGS) libjsmn.la )

arm9loaderhax.bin: arm9loaderhax
	$(OBJCOPY) $(OCFLAGS) -O binary arm9loaderhax arm9loaderhax.bin

arm9launcher.bin: arm9launcher
	$(OBJCOPY) $(OCFLAGS) -O binary arm9launcher arm9launcher.bin

