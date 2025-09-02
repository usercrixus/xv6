# The OBJS variable is a list of object files that need to be 
# compiled and linked together to create the final xv6 kernel 
# image. Each line in the list represents an individual object 
# file. The \ character at the end of each line indicates that
# the list continues on the next line.
#
# In the rule kernel: $(OBJS) entry.o entryother initcode kernel.ld, 
# all the $(OBJS) files are dependencies for the target kernel. Each 
# $(OBJS) file is in the form filename.o, indicating an object file. 
# The makefile implicitly assumes that each object file (filename.o) 
# depends on a corresponding C source file (filename.c) and includes 
# the necessary rules to compile the C source files into object files.
#
# For example, if there is an object file foo.o, make will look for a 
# corresponding C source file named foo.c and compile it into foo.o 
# using the implicit rule for compiling C source files. The implicit 
# rule invokes the C compiler ($(CC)) with the specified compiler flags 
# ($(CFLAGS)) to generate the object file.
OBJS = \
	console.o\
	./processus/exec.o\
	./fileSystem/bio.o\
	./fileSystem/fs.o\
	./fileSystem/file.o\
	./fileSystem/log.o\
	./fileSystem/pipe.o\
	./drivers/ide.o\
	./drivers/ioapic.o\
	./memory/kalloc.o\
	./drivers/kbd.o\
	./drivers/lapic.o\
	mp.o\
	main.o\
	./drivers/picirq.o\
	./processus/proc.o\
	./synchronization/sleeplock.o\
	./synchronization/spinlock.o\
	./userLand/ulibGeneric.o\
	swtch.o\
	./systemCall/syscall.o\
	./systemCall/sysfile.o\
	./systemCall/sysproc.o\
	./systemCall/trapasm.o\
	./systemCall/trap.o\
	./drivers/uart.o\
	./systemCall/vectors.o\
	./memory/vm.o\

# Cross-compiling
TOOLPREFIX = x86_64-linux-gnu-

# If the makefile can't find QEMU, specify its path here
QEMU = qemu-system-i386

# sets the value of the CC variable, which specifies the C 
# compiler to use. The $(TOOLPREFIX) is a variable that holds 
# the prefix for the toolchain, as defined earlier. In this 
# case, it prefixes the gcc command, indicating that the 
# cross-compiler should be used.
CC = $(TOOLPREFIX)gcc
# sets the value of the AS variable, which specifies the 
# assembler to use. Similarly, it uses the $(TOOLPREFIX) to 
# prefix the gas command for the cross-assembler.
AS = $(TOOLPREFIX)as # Here unused..
# sets the value of the LD variable, which specifies the 
# linker to use. It also uses the $(TOOLPREFIX) to prefix 
# the ld command for the cross-linker.
LD = $(TOOLPREFIX)ld
# sets the value of the OBJCOPY variable, which specifies 
# the object file copier. Again, the $(TOOLPREFIX) is used 
# to prefix the objcopy command.
OBJCOPY = $(TOOLPREFIX)objcopy
# sets the value of the OBJDUMP variable, which specifies 
# the program for displaying information about object files.
OBJDUMP = $(TOOLPREFIX)objdump
# sets the value of the CFLAGS variable, which holds the 
# compiler flags for C code. These flags control various 
# aspects of the compilation process, such as optimization 
# level, warning options, and debugging symbols.
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer -g
# appends additional flags to CFLAGS using the += operator. 
# It checks if the compiler supports the -fno-stack-protector 
# flag by invoking $(CC) (the C compiler) with some options 
# and checking the result. If the flag is supported, it is 
# added to CFLAGS.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# sets the value of the ASFLAGS variable, which holds the 
# assembler flags. These flags specify options for the assembly 
# process, such as generating 32-bit code, debugging information,
# and division support.
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
# appends linker flags to LDFLAGS using the += operator. It invokes 
# $(LD) (the linker) with some options to extract the target 
# architecture from the linker's version information and adds it 
# to LDFLAGS. The target architecture is determined by searching 
# for the string "elf_i386" in the linker's version output.
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1) # in my case : elf_i386

# Make cmd and dependencies.
xv6.img: bootblock kernel
# creates a new disk image file called xv6.img filled with zeros. 
# The count parameter specifies the number of blocks to be created, 
# and in this case, it is set to 10000.
	dd if=/dev/zero of=xv6.img count=10000
# copies the contents of the bootblock file into the xv6.img disk 
# image file. The conv=notrunc option ensures that the existing 
# contents of xv6.img are not truncated before writing.
	dd if=bootblock of=xv6.img conv=notrunc
# appends the contents of the kernel file to the xv6.img disk image file. 
# The seek=1 option instructs dd to skip the first block of xv6.img 
# before writing the kernel, effectively placing the kernel after the 
# bootblock in the disk image.
	dd if=kernel of=xv6.img seek=1 conv=notrunc

# The "kernelmemfs" in the xv6 operating system refers to a modified 
# version of the kernel that uses an in-memory file system (memfs) 
# instead of a disk-based file system. In the original xv6 implementation, 
# the kernel relies on a disk image (fs.img) for file system operations. 
# However, the kernelmemfs variant allows the entire file system to 
# reside in memory.
xv6memfs.img: bootblock kernelmemfs
	dd if=/dev/zero of=xv6memfs.img count=10000
	dd if=bootblock of=xv6memfs.img conv=notrunc
# appends the contents of the "kernelmemfs" file to the "xv6memfs.img" 
# disk image, starting from the second block (seek=1). Again, the 
# "conv=notrunc" option is used to prevent overwriting existing data.
	dd if=kernelmemfs of=xv6memfs.img seek=1 conv=notrunc

# responsible for creating the boot block of the xv6 operating system. 
# The boot block is the first sector of the disk image and contains 
# the initial bootloader code.
bootblock:./bootstrap/bootasm.S ./bootstrap/bootmain.c
# Compilation of bootmain.c: create bootmain.o
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c ./bootstrap/bootmain.c -o ./bootstrap/bootmain.o
# Compilation of bootasm.S: create bootasm.o
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c ./bootstrap/bootasm.S -o ./bootstrap/bootasm.o
# Linking of bootblock: The $(LD) linker is used to link the object 
# files (bootasm.o, bootmain.o) into an executable file (bootblock.o).
# The -N option tells the linker not to page-align sections, and 
# the -e start option sets the entry point of the executable to the symbol start.
# The -Ttext 0x7C00 option specifies the virtual address at which the text 
# section should be placed.
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o ./bootstrap/bootasm.o ./bootstrap/bootmain.o
# not essential for building or running the xv6 operating system. It is 
# used for studying and examining the generated machine code of the 
# bootblock object file.
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
# The sign.py script is executed to "sign" the bootblock binary file.
# The purpose of signing is to modify the last two bytes of the bootblock 
# to contain a special signature. The signed bootblock file is the final output.
	python3 ./utility/sign.py bootblock

entryother: entryother.S
# Compilation of entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c entryother.S
# it reorganizes the sections and headers of the entryother.o object file.
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
# uses the objcopy tool to extract the .text section from the bootblockother.o 
# object file and create a binary file named entryother. The -S flag preserves 
# symbolic information, and the -O binary flag specifies the output format as 
# binary. The -j .text flag specifies that only the .text section should be extracted.
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
# not essential for building or running the xv6 operating system. It is 
# used for studying and examining the generated machine code of the 
# bootblockother object file.
	$(OBJDUMP) -S bootblockother.o > entryother.asm

# compiles the initcode.S assembly source file into an object file (initcode.o)
initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
# links the object file (initcode.o) using the linker ($(LD)) with specific 
# flags and options. It sets the entry point of the program to the symbol 
# start (-e start) and specifies the virtual address at which the text 
# section should be placed (-Ttext 0). The resulting linked output is 
# saved as initcode.out.
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
# copies (-O binary) the text and data sections from initcode.out and 
# creates a raw binary file named initcode.
	$(OBJCOPY) -S -O binary initcode.out initcode

# responsible for creating the final kernel binary
kernel: $(OBJS) systemCall/vectors.S ./bootstrap/entry.o entryother initcode kernel.ld
# The linker takes multiple input files: entry.o, $(OBJS) (the list of object files), 
# initcode, and entryother. These files contain the compiled code and data necessary 
# for building the kernel. The -b binary flag indicates that initcode and entryother 
# should be treated as binary files during the linking process.
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel ./bootstrap/entry.o $(OBJS) -b binary initcode entryother

# Create interupt handler
systemCall/vectors.S: ./systemCall/vectors.py
	python3 ./systemCall/vectors.py > ./systemCall/vectors.S

# user lib
ULIB = ./userLand/ulibGeneric.o ./userLand/ulibXv6.o ./userLand/usys.o ./userLand/printf.o ./userLand/umalloc.o
# This line specifies the target pattern _%. This means that any target that matches 
# this pattern can be considered for this rule. The %` is a wildcard that matches 
# any string.
_%: commands/%.o $(ULIB)
# $@ is replaced with the target name, which is the target binary file being built.
# $^ is replaced with the list of prerequisites, which are the object files and the 
# files specified in the ULIB variable.
# If the target is _cat, then in the rule _cat: cat.o $(ULIB), the variables $@, $^, 
# and $* will have the following values:
#   $@: _cat
#   $^: cat.o ulib.o usys.o printf.o umalloc.o
#   $*: cat
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^

_forktest: commands/forktest.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o _forktest commands/forktest.o ./userLand/ulibGeneric.o ./userLand/usys.o

# responsible for compiling and linking the mkfs.c source file, which is a program 
# used to create a file system image.
mkfs: ./fileSystem/mkfs.c ./fileSystem/fs.h
	gcc -Werror -Wall -o mkfs ./fileSystem/mkfs.c

# Keeping all object files as precious using the .PRECIOUS target prevents make 
# from deleting them after the first build. This can be useful in situations 
# where the object files are expensive to build or where you want to retain them 
# for debugging or analysis purposes.
#
# By marking the object files as precious, make will consider them important 
# and won't remove them, even if they are intermediate files. This ensures that 
# subsequent builds can skip the compilation step for object files that haven't 
# changed, saving time and resources.
.PRECIOUS: %.o

# a variable that holds a list of user programs (executable files) in the form 
# of _program1, _program2, _program3, and so on. Each program is prefixed with 
# an underscore symbol to differentiate it from other files or directories.
UPROGS=\
	_cat\
	_echo\
	_forktest\
	_grep\
	_init\
	_kill\
	_ln\
	_ls\
	_mkdir\
	_rm\
	_sh\
	_stressfs\
	_wc\
	_zombie\
#	_usertests

# This command runs the mkfs program, passing three arguments: fs.img, README, 
# and $(UPROGS). The purpose of this command is to generate the fs.img file, 
# which represents the file system image containing the operating system's 
# file system structure and the user programs specified in $(UPROGS).
#
# By including $(UPROGS) as a prerequisite, the makefile ensures that any 
# changes to the user programs or their dependencies will trigger the rebuild 
# of the fs.img file. This guarantees that the file system image remains up 
# to date with the latest versions of the user programs.
fs.img: mkfs README $(UPROGS)
	./mkfs fs.img README $(UPROGS)

# The -include directive in the makefile includes the specified files as makefile 
# fragments. In this case, *.d is a wildcard pattern that matches all files with 
# the .d extension. This directive is typically used to include dependency files 
# generated by the compiler, which contain information about the dependencies of 
# the source files.
#
# By including the dependency files (*.d), the makefile ensures that the 
# dependencies are properly tracked and the target files are rebuilt when their 
# dependencies change. The dependencies are specified in the form of rules that 
# describe the relationships between the target files and their prerequisites 
# (source files, headers, etc.). Including the dependency files ensures that 
# these rules are applied and the build process is correctly managed.
#
# the dependency files (*.d files) are typically auto-generated by the compiler
# or build system.
# Include all generated dependency files across subdirectories.
-include $(shell find . -name '*.d' 2>/dev/null)

# responsible for removing generated files and cleaning up the build artifacts. 
# It specifies a list of files and directories to be deleted using the rm command.
.PHONY: clean
clean:
	# Remove top-level known artifacts
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg *.sym \
		./systemCall/vectors.S bootblock bootblock.o bootblockother.o \
		entryother entryother.o entryother.asm \
		initcode initcode.o initcode.out \
		kernel kernelmemfs xv6.img xv6memfs.img fs.img \
		mkfs .gdbinit $(UPROGS)
	# Remove objects, deps, and .asm files recursively (portable; works with /bin/sh)
	find . -type f \( -name '*.o' -o -name '*.d' -o -name '*.asm' \) -delete 2>/dev/null || true

# setting up options and variables related to running the QEMU emulator and 
# debugging with GDB (GNU Debugger) for the xv6 operating system.
#
# This line generates a unique GDB port number by using the id -u command 
# to retrieve the current user's user ID, then performing a modulo 
# operation and adding an offset. This ensures that each user gets a 
# unique port number for debugging.
GDBPORT = 26000
# This line checks if the QEMU emulator supports the -gdb option by invoking 
# $(QEMU) -help and searching for the existence of the -gdb flag in the 
# output. If supported, it sets QEMUGDB to -gdb tcp::$(GDBPORT), indicating
# that GDB should connect to QEMU using TCP and the generated GDB port. 
# If not supported, it sets QEMUGDB to -s -p $(GDBPORT), indicating that
# GDB should connect to QEMU using a socket and the generated GDB port.
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
# This block checks if the CPUS variable is not defined. If not defined, 
# it sets CPUS to 2, indicating that the QEMU emulator should be run with two CPUs.
CPUS := 2
endif
# This line sets up the options for running QEMU. It specifies the disk image files 
# to be used (fs.img and xv6.img), the number of CPUs ($(CPUS)), the amount of 
# memory (-m 512), and any additional QEMU options ($(QEMUEXTRA)).
# Here index=1 and index=0 refer to the ide channel
QEMUOPTS = -drive file=fs.img,index=1,media=disk,format=raw -drive file=xv6.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA)

# -serial mon:stdio: Redirects the serial port to the standard input/output of the QEMU 
# that mean, the UART port of qemu is linked to our terminal
qemu: fs.img xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)
