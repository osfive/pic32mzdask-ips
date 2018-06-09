TARGET("elf32-tradlittlemips")
ENTRY(_start)

MEMORY
{
	flash        (rx) : ORIGIN = 0x9fc00000,  LENGTH = 64K-64
	devcfg        (r) : ORIGIN = 0x9fc0ffc0,  LENGTH = 16
	devsign       (r) : ORIGIN = 0x9fc0ffe0,  LENGTH = 16
	prog         (rx) : ORIGIN = 0x9d000000,  LENGTH = 2048K
	sram0      (rw!x) : ORIGIN = 0xa0000000,  LENGTH = 0x180
	exception   (rwx) : ORIGIN = 0xa0000180,  LENGTH = 360 /* MipsExceptionEnd - MipsException */
	sram       (rw!x) : ORIGIN = 0xa0000380,  LENGTH = 512K - 0x180 - 360
}

SECTIONS
{
	. = 0x9fc00000;
	.start . : {
		*start.o(.text)
	} > flash

	.text : {
		*(.text)
	} > flash

	.rodata : {
		*(.rodata)
	} > flash

	.devcfg : {
		*(.config3)
		*(.config2)
		*(.config1)
		*(.config0)
	} > devcfg

	.devsign : {
		*(.devsign)
	} > devsign

	.data : {
		*(.data)
	} > sram AT > flash

	.sdata : {
		_gp = .;
		*(.sdata)
	} > sram AT > flash

	.bss : {
		_sbss = ABSOLUTE(.);
		*(.bss COMMON)
		_ebss = ABSOLUTE(.);
	} > sram

	. = ALIGN(4);
	stack_top = . + 0x1000; /* 4kB of stack memory */

	.font : {
		. = ALIGN(4);
		_sfont = ABSOLUTE(.);
		INCLUDE "FONT_PATH";
		_efont = ABSOLUTE(.);
	} > prog
}