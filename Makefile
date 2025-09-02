# Minimal build for xv6 from repository root

# Object files (prefixed with xv6/)
OBJS = \
	xv6/drivers/console.o\
	xv6/processus/exec.o\
	xv6/fileSystem/bio.o\
	xv6/fileSystem/fs.o\
	xv6/fileSystem/file.o\
	xv6/fileSystem/log.o\
	xv6/fileSystem/pipe.o\
	xv6/drivers/ide.o\
	xv6/drivers/ioapic.o\
	xv6/memory/kalloc.o\
	xv6/drivers/kbd.o\
	xv6/drivers/lapic.o\
	xv6/mp/mp.o\
	xv6/main.o\
	xv6/drivers/picirq.o\
	xv6/processus/proc.o\
	xv6/synchronization/sleeplock.o\
	xv6/synchronization/spinlock.o\
	xv6/userLand/ulibGeneric.o\
	xv6/processus/swtch.o\
	xv6/systemCall/syscall.o\
	xv6/systemCall/sysfile.o\
	xv6/systemCall/sysproc.o\
	xv6/systemCall/trapasm.o\
	xv6/systemCall/trap.o\
	xv6/drivers/uart.o\
	xv6/systemCall/vectors.o\
	xv6/memory/vm.o

# Toolchain
TOOLPREFIX = x86_64-linux-gnu-
QEMU = qemu-system-i386
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer -g -I xv6/include -I xv6/mp -I xv6
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

# Documentation path (lives inside kernel tree)
DOCS_DIR := xv6/docs
DOCS_README := $(DOCS_DIR)/README

# Disk image
xv6/xv6.img: xv6/bootblock xv6/kernel
	dd if=/dev/zero of=$@ count=10000
	dd if=xv6/bootblock of=$@ conv=notrunc
	dd if=xv6/kernel of=$@ seek=1 conv=notrunc

## (Optional memfs image removed for minimal setup)

# Boot block
xv6/bootblock: xv6/bootstrap/bootasm.S xv6/bootstrap/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I xv6 -c xv6/bootstrap/bootmain.c -o xv6/bootstrap/bootmain.o
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I xv6 -c xv6/bootstrap/bootasm.S -o xv6/bootstrap/bootasm.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o xv6/bootblock.o xv6/bootstrap/bootasm.o xv6/bootstrap/bootmain.o
	$(OBJCOPY) -S -O binary -j .text xv6/bootblock.o xv6/bootblock
	python3 xv6/utility/sign.py xv6/bootblock

# AP start stub
xv6/entryother: xv6/bootstrap/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I xv6 -c xv6/bootstrap/entryother.S -o xv6/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o xv6/bootblockother.o xv6/entryother.o
	$(OBJCOPY) -S -O binary -j .text xv6/bootblockother.o xv6/entryother
	# (omit disassembly dump for minimal build)

# First user code image
xv6/initcode: xv6/bootstrap/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I xv6 -c xv6/bootstrap/initcode.S -o xv6/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o xv6/initcode.out xv6/initcode.o
	$(OBJCOPY) -S -O binary xv6/initcode.out xv6/initcode

# Interrupt vectors
xv6/systemCall/vectors.S: xv6/systemCall/vectors.py
	python3 xv6/systemCall/vectors.py > xv6/systemCall/vectors.S

# Kernel
xv6/kernel: $(OBJS) xv6/systemCall/vectors.S xv6/bootstrap/entry.o xv6/entryother xv6/initcode xv6/linker/kernel.ld
	# Symlink binaries into CWD for ld -b binary inputs
	ln -sf xv6/initcode initcode
	ln -sf xv6/entryother entryother
	$(LD) $(LDFLAGS) -T xv6/linker/kernel.ld -o $@ xv6/bootstrap/entry.o $(OBJS) -b binary initcode entryother
	rm -f initcode entryother

# Assembly objects
xv6/processus/swtch.o: xv6/processus/swtch.S
	$(CC) -m32 -gdwarf-2 -Wa,-divide -c -o $@ $<

# User library
ULIB = xv6/userLand/ulibGeneric.o xv6/userLand/ulibXv6.o xv6/userLand/usys.o xv6/userLand/printf.o xv6/userLand/umalloc.o

# User programs
xv6/_%: xv6/commands/%.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^

xv6/_forktest: xv6/commands/forktest.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ xv6/commands/forktest.o xv6/userLand/ulibGeneric.o xv6/userLand/usys.o

# mkfs tool
xv6/mkfs: xv6/fileSystem/mkfs.c xv6/fileSystem/fs.h
	gcc -Werror -Wall -o $@ xv6/fileSystem/mkfs.c

.PRECIOUS: %.o

# Names of user programs as seen inside fs
UPROGS_NAMES=\
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
	_zombie

# Paths to built user program binaries
UPROGS=$(addprefix xv6/,$(UPROGS_NAMES))

# Filesystem image (mkfs expects bare filenames; run from xv6/)
xv6/fs.img: xv6/mkfs $(DOCS_README) $(UPROGS)
	cd xv6 && ./mkfs ../xv6/fs.img docs/README $(UPROGS_NAMES)

# Auto-included dependencies
-include $(shell find xv6 -name '*.d' 2>/dev/null)

.PHONY: clean
clean:
	rm -f xv6/*.tex xv6/*.dvi xv6/*.idx xv6/*.aux xv6/*.log xv6/*.ind xv6/*.ilg xv6/*.sym \
		xv6/systemCall/vectors.S xv6/bootblock xv6/bootblock.o xv6/bootblockother.o \
		xv6/entryother xv6/entryother.o xv6/entryother.asm \
		xv6/initcode xv6/initcode.o xv6/initcode.out \
		xv6/kernel xv6/kernelmemfs xv6/xv6.img xv6/xv6memfs.img xv6/fs.img \
		xv6/mkfs xv6/.gdbinit xv6/_cat xv6/_echo xv6/_forktest xv6/_grep xv6/_init xv6/_kill xv6/_ln xv6/_ls xv6/_mkdir xv6/_rm xv6/_sh xv6/_stressfs xv6/_wc xv6/_zombie
	find xv6 -type f \( -name '*.o' -o -name '*.d' -o -name '*.asm' \) -delete 2>/dev/null || true

## (dev-links helper removed for minimal setup)

# GDB/QEMU integration
GDBPORT = 26000
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif
QEMUOPTS = -drive file=xv6/fs.img,index=1,media=disk,format=raw -drive file=xv6/xv6.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA)

qemu: xv6/fs.img xv6/xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

xv6/.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: xv6/fs.img xv6/xv6.img xv6/.gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

# Launch gdb using the template inside xv6/
.PHONY: gdb
gdb: xv6/.gdbinit
	gdb -x xv6/.gdbinit

# Convenience aliases for old targets
.PHONY: fs.img xv6.img kernel bootblock entryother initcode mkfs gdb
fs.img: xv6/fs.img ; @true
xv6.img: xv6/xv6.img ; @true
kernel: xv6/kernel ; @true
bootblock: xv6/bootblock ; @true
entryother: xv6/entryother ; @true
initcode: xv6/initcode ; @true
mkfs: xv6/mkfs ; @true
