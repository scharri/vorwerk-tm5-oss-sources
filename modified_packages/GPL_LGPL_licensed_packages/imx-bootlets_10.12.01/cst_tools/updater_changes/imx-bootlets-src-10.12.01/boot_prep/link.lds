OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
/* Image loaded at address 0x10 to avoid input_ivt at 0 */
	. = 0x00000010;
	. = ALIGN(4);
	.text : { *(.text) }
	.data : { *(.data) }
/* Reserve this area to store HAB related data such as CSF commands, certificates and signatures */
	. = ALIGN(4);
	__hab_data = .;
	. = . + 0x2000;		/* 8kB */
	__hab_data_end = .;
/* Place the __hab_data memory region before .bss region to avoid being over written at runtime and to keep the binary as small as possible */
	. = ALIGN(4);
	.bss  : { *(.bss) }
/*	. = 0x0000C400;		*//* 49kB - top of bootlet memory */
}
