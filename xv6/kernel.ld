/*
Simple linker script for the JOS kernel.
See the GNU ld 'info' manual ("info ld") to learn the syntax.
*/

/*
Specifies the output format as ELF (Executable and Linkable Format) for 32-bit x86 architecture.
*/
OUTPUT_FORMAT("elf32-i386")
/*
specifies the target architecture as i386.
*/
OUTPUT_ARCH(i386)
/*
specifies the entry point of the executable as the symbol _start. This is the starting point of program execution.
*/
ENTRY(_start)

/*
This block defines various sections and their attributes.

A section refers to a logical unit within the binary file that contains related data or code. Sections organize the 
different parts of a program, such as executable instructions, constant data, global variables, symbol tables, and 
debugging information. Each section has a specific purpose and characteristics.
*/
SECTIONS
{
	/*
	Sets the current address (.) to 0x80100000. 
	This is the base address where the kernel will be linked.
	Must be equal to KERNLINK
	*/
	. = 0x80100000;
	/*
	Defines the text section starting at virtual address 0x100000. It contains the executable code.
	*/
	.text : AT(0x100000) {
		/*
		includes all symbols and sections related to the text section.
		*/
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}
	/*
	defines the symbol etext and assigns it the value of the current address (.). It represents the end of the text section.
	*/
	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
	/*
	defines the read-only data section containing constant data.
	*/
	.rodata : {
		/*
		includes all symbols and sections related to the read-only data section.
		*/
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Include debugging information in kernel memory */
	.stab : {
		PROVIDE(__STAB_BEGIN__ = .);
		*(.stab);
		PROVIDE(__STAB_END__ = .);
	}

	.stabstr : {
		PROVIDE(__STABSTR_BEGIN__ = .);
		*(.stabstr);
		PROVIDE(__STABSTR_END__ = .);
	}

	/*
	Adjust the address for the data segment to the next page 
	The value 0x1000 (4096 in decimal) represents the size of a page.
	*/
	. = ALIGN(0x1000);

	/*
	Conventionally, Unix linkers provide pseudo-symbols
	etext, edata, and end, at the end of the text, data, and bss.
	For the kernel mapping, we need the address at the beginning
	of the data section, but that's not one of the conventional
	symbols, because the convention started before there was a
	read-only rodata section between text and data.
	*/
	/*
	defines the symbol data and assigns it the value of the current address. It represents the beginning of the data section.
	*/
	PROVIDE(data = .);
	/*
	defines the data section containing initialized global and static variables.
	*/
	.data : {
		/*
		includes all symbols and sections related to the data section.
		*/
		*(.data)
	}

	/*
	defines the symbol edata and assigns it the value of the current address. It represents the end of the data section.
	*/
	PROVIDE(edata = .);
	/*
	defines the bss section, which contains uninitialized global and static variables.
	*/
	.bss : {
		/*
		includes all symbols and sections related to the bss section.
		*/
		*(.bss)
	}

	/*
	defines the symbol end and assigns it the value of the current address. It represents the end of the program's memory
	*/
	PROVIDE(end = .);

	/*
	In the provided linker script snippet, /DISCARD/ is a special section that instructs the linker to discard or remove specific 
	sections from the final executable. The sections specified within the /DISCARD/ block will not be included in the output binary
	*/
	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}


/*
the linker script defines the virtual addresses of various sections in the final executable. 
The virtual addresses determine the layout and organization of the program's memory when it 
is loaded and executed in a virtual memory system.
By specifying the virtual addresses of different sections in the linker script, you can 
control the memory layout of the executable and ensure that each section is located at the 
desired virtual address when the program is loaded into memory.

Text Section: Also known as the code section, it contains the machine instructions that make 
up the program's executable code. This section is typically marked as read-only.

Data Section: This section holds initialized global and static variables that have a 
defined initial value. It includes both mutable and immutable data.

BSS Section: The Block Started by Symbol (BSS) section contains uninitialized global and 
static variables. These variables are initialized to zero or null values during program startup.

Symbol Table: This section stores information about the symbols defined and referenced within 
the program, such as functions, variables, and labels. It allows for symbol resolution and debugging.

Debugging Sections: These sections store additional information used for debugging, including 
source code line numbers, variable names, function names, and stack frame information.

Relocation Section: When the program is loaded into memory, addresses of symbols and references 
may need to be adjusted. Relocation sections contain information for the linker to perform these 
address adjustments.

These sections provide a structured way to organize and manage the different components of a p
rogram within the binary file, facilitating linking, loading, and debugging processes.
*/