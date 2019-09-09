OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start_code)
SECTIONS
{
	. = 0xA0010000;

	. = ALIGN(4);
	__text_start__ = .;
	.text : { *(.text) }
	__text_end__ = .;

	. = ALIGN(4);
	__data_start__ = .;
	.rodata : { *(.rodata) }
	. = ALIGN(4);
	.data : { *(.data) }
	__data_end__ = .;

	. = ALIGN(4);
	.got : { *(.got) }

	. = ALIGN(4);
	__bss_start__ = .;
	.bss : { *(.bss) }
	__bss_end__ = .;
}
