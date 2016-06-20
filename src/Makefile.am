include $(top_srcdir)/common.mk

noinst_PROGRAMS = arm9loaderhax arm9launcher
arm9loaderhax_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/arm9loaderhax.ld -nostartfiles -I$(CTRARM9)/include
arm9loaderhax_LDFLAGS=$(AM_LDFLAGS) -lctr
arm9loaderhax_SOURCES = main.c start.s
arm9loaderhax_LDADD = -L$(CTRARM9)/lib -lctr9 -lctr

arm9launcher_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/bootloader.ld -nostartfiles -I$(CTRARM9)/include
arm9launcher_LDFLAGS=$(AM_LDFLAGS) -lctr
arm9launcher_SOURCES = arm9launcher.c a_start.s
arm9launcher_LDADD = -L$(CTRARM9)/lib -lctr9 -lctr


EXTRA_DIST = arm9loaderhax.ld

all-local: arm9loaderhax.bin arm9launcher.bin
clean-local:
	rm -f arm9loaderhax.bin arm9launcher.bin

arm9loaderhax.bin: arm9loaderhax
	$(OBJCOPY) $(OCFLAGS) -O binary arm9loaderhax arm9loaderhax.bin

arm9launcher.bin: arm9launcher
	$(OBJCOPY) $(OCFLAGS) -O binary arm9launcher arm9launcher.bin

