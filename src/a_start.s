@*******************************************************************************
@* Copyright (C) 2016 Gabriel Marcano
@*
@* Refer to the COPYING.txt file at the top of the project directory. If that is
@* missing, this file is licensed under the GPL version 2.0 or later.
@*
@******************************************************************************/

.arm
.align 4

.global _entry

.section .text.start, "x"

_entry:
	adr r2, main_offset
	ldr r3, [r2]
	add r2, r3, r2
	bx r2

main_offset:
.word main-.

