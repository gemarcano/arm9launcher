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

