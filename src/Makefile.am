include $(top_srcdir)/common.mk

noinst_PROGRAMS = template bootloader
template_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/arm9loaderhax.ld -nostartfiles -I$(CTRARM9)/include
template_LDFLAGS=$(AM_LDFLAGS) -lctr
template_SOURCES = main.c start.s
template_LDADD = -L$(CTRARM9)/lib -lctr9 -lctr

bootloader_CFLAGS=$(AM_CFLAGS) -T$(srcdir)/bootloader.ld -nostartfiles -I$(CTRARM9)/include
bootloader_LDFLAGS=$(AM_LDFLAGS) -lctr
bootloader_SOURCES = bootloader.c start.s
bootloader_LDADD = -L$(CTRARM9)/lib -lctr9 -lctr


EXTRA_DIST = arm9loaderhax.ld

all-local: template.bin bootloader.bin
clean-local:
	rm -f template.bin bootloader.bin

template.bin: template
	$(OBJCOPY) $(OCFLAGS) -O binary template template.bin

bootloader.bin: bootloader
	$(OBJCOPY) $(OCFLAGS) -O binary bootloader bootloader.bin

