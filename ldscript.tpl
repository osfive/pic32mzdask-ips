TARGET("elf32-tradlittlemips")
ENTRY(_start)

MEMORY
{
	flash        (rx) : ORIGIN = 0x9fc00000,  LENGTH = 64K-64
	devcfg        (r) : ORIGIN = 0x9fc0ffc0,  LENGTH = 16
	devsign       (r) : ORIGIN = 0x9fc0ffe0,  LENGTH = 16
	prog         (rx) : ORIGIN = 0x9d000000,  LENGTH = 2048K
	sram0      (rw!x) : ORIGIN = 0xa0000000,  LENGTH = 0x180
	exception   (rwx) : ORIGIN = 0xa0000180,  LENGTH = 0x200 /* MipsExceptionEnd - MipsException */
	sram1      (rw!x) : ORIGIN = 0xa0000380,  LENGTH = 256K
	sram2      (rw!x) : ORIGIN = 0xa0040380,  LENGTH = 256K - 0x380 /* malloc */
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

	/* Ensure _smem is associated with the next section */
	. = .;
	_smem = ABSOLUTE(.);
	.data : {
		_sdata = ABSOLUTE(.);
		*(.data*)
		_edata = ABSOLUTE(.);
	} > sram1 AT > flash

	.bss : {
		_sbss = ABSOLUTE(.);
		*(.bss COMMON)
		*(.sbss)
		_ebss = ABSOLUTE(.);
	} > sram1

	. = ALIGN(4);
	stack_top = . + 0x1000; /* 4kB of stack memory */

	.font : {
		. = ALIGN(4);
		_sfont = ABSOLUTE(.);
		INCLUDE "FONT_PATH";
		_efont = ABSOLUTE(.);
	} > prog
}
