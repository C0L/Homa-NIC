ENTRY(_start)
SECTIONS
{
    . = 0x0;
    .text   :
    {
	*(.boot)
	*(.text)
    }
    . = 0x0100;
    .data   : { *(.data) }
    .bss    : { *(.bss)  }
    . = 0x4000;
}
