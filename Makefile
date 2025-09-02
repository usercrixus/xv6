# Object files path
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

# User library path
ULIB = \
	xv6/userLand/ulibGeneric.o \
	xv6/userLand/ulibXv6.o \
	xv6/userLand/usys.o \
	xv6/userLand/printf.o \
	xv6/userLand/umalloc.o

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

DOCS_README := xv6/docs/README

# Toolchain
TOOLPREFIX = x86_64-linux-gnu-
QEMU = qemu-system-i386
CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer -g -I xv6/include -I xv6/mp -I xv6
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

# Disk image
xv6/xv6.img: xv6/bootblock xv6/kernel
	dd if=/dev/zero of=$@ count=10000
	dd if=xv6/bootblock of=$@ conv=notrunc
	dd if=xv6/kernel of=$@ seek=1 conv=notrunc

# Boot block
xv6/bootblock: xv6/bootstrap/bootasm.S xv6/bootstrap/bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I xv6 -c xv6/bootstrap/bootmain.c -o xv6/bootstrap/bootmain.o
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I xv6 -c xv6/bootstrap/bootasm.S -o xv6/bootstrap/bootasm.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o xv6/bootblock.o xv6/bootstrap/bootasm.o xv6/bootstrap/bootmain.o
	$(OBJCOPY) -S -O binary -j .text xv6/bootblock.o xv6/bootblock
	python3 xv6/utility/sign.py xv6/bootblock

# Kernel
xv6/kernel: $(OBJS) xv6/systemCall/vectors.S xv6/bootstrap/entry.o xv6/entryother xv6/initcode xv6/linker/kernel.ld
	# Symlink binaries into CWD for ld -b binary inputs
	ln -sf xv6/initcode initcode
	ln -sf xv6/entryother entryother
	$(LD) $(LDFLAGS) -T xv6/linker/kernel.ld -o $@ xv6/bootstrap/entry.o $(OBJS) -b binary initcode entryother
	rm -f initcode entryother

# Interrupt vectors
xv6/systemCall/vectors.S: xv6/systemCall/vectors.py
	python3 xv6/systemCall/vectors.py > xv6/systemCall/vectors.S

# Kernel entry object
xv6/bootstrap/entry.o: xv6/bootstrap/entry.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I xv6 -c $< -o $@

# First user code image
xv6/initcode: xv6/bootstrap/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I xv6 -c xv6/bootstrap/initcode.S -o xv6/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o xv6/initcode.out xv6/initcode.o
	$(OBJCOPY) -S -O binary xv6/initcode.out xv6/initcode

# AP start stub
xv6/entryother: xv6/bootstrap/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I xv6 -c xv6/bootstrap/entryother.S -o xv6/entryother.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o xv6/bootblockother.o xv6/entryother.o
	$(OBJCOPY) -S -O binary -j .text xv6/bootblockother.o xv6/entryother

# Assembly objects
xv6/processus/swtch.o: xv6/processus/swtch.S
	$(CC) $(ASFLAGS) -c -o $@ $<

# User programs
xv6/_%: xv6/commands/%.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^

xv6/_forktest: xv6/commands/forktest.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ xv6/commands/forktest.o xv6/userLand/ulibGeneric.o xv6/userLand/usys.o

# mkfs tool
xv6/mkfs: xv6/fileSystem/mkfs.c xv6/fileSystem/fs.h
	gcc -Werror -Wall -o $@ xv6/fileSystem/mkfs.c

# Filesystem image (mkfs expects bare filenames; run from xv6/)
xv6/fs.img: xv6/mkfs $(DOCS_README) $(UPROGS)
	./xv6/mkfs $@ $(DOCS_README) $(UPROGS)

# Auto-included dependencies
-include $(shell find xv6 -name '*.d' 2>/dev/null)


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
gdb: xv6/.gdbinit
	gdb -x xv6/.gdbinit


clean:
	rm -f xv6/systemCall/vectors.S xv6/entryother xv6/initcode xv6/initcode.out xv6/kernel xv6/xv6.img xv6/mkfs \
	xv6/.gdbinit xv6/bootblock $(UPROGS)
	find xv6 -type f \( -name '*.o' -o -name '*.d' -o -name '*.asm' \) -delete 2>/dev/null || true


.PRECIOUS: %.o
