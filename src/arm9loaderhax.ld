OUTPUT_ARCH(arm)
ENTRY(_entry)
SECTIONS
{
	. = 0x23F00000;
	.text : {
		*(.text.start)
		*(.text)
		. = ALIGN(4);
	}
	.data : {
		*(.data)
		. = ALIGN(4);
	}

	.bss ALIGN(4) : {
		__bss_start = .;
		*(.bss COMMON)
		. = ALIGN(4);
	}
	__bss_end = .;
	
	.rodata : {
		*(.rodata)
		. = ALIGN(4);
	}

	.plt :
	{
		*(.plt)
	}

	__end__ = . ;/*0x20400000; */
}

