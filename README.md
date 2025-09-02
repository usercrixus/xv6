## xv6 (42 projects) — Workspace Guide

A minimal xv6-based OS workspace structured to keep code under `xv6/` and editor/debug tooling under `.vscode/`. This README gives a quick tour, build/run steps, and debugging tips both in VS Code and from the terminal.

## Layout

- `xv6/`: all kernel, userland, and build artifacts
  - `bootstrap/`: bootloader and early entry code
  - `commands/`: user programs (`cat`, `ls`, `sh`, ...)
  - `drivers/`: console, disk, APIC, UART
  - `fileSystem/`: mkfs tool and FS implementation
  - `include/`, `type/`: headers and shared types
  - `linker/`: linker scripts
  - `memory/`, `processus/`, `systemCall/`, `synchronization/`, `mp/`: core subsystems
  - `docs/README`: text resource included in the root of the xv6 filesystem image
  - `main.c`: kernel entry point
- `.vscode/`: minimal IDE setup
  - `launch.json`: one debug config that launches QEMU in gdb mode and attaches
  - `tasks.json`: background task to start QEMU (`make qemu-gdb`)
  - `settings.json`: light language hints (C associations, clang-format fallback)
- `Makefile`: top-level build that drives everything in `xv6/`
- `.gdbinit.tmpl`: template used to generate `xv6/.gdbinit` (kept inside xv6)

## Prerequisites

Install a 32-bit capable cross toolchain and QEMU:

- `qemu-system-i386`
- `gdb`
- `x86_64-linux-gnu-gcc`, `as`, `ld`, `objcopy`, `objdump`

On Debian/Ubuntu, roughly:

```
sudo apt install qemu-system-x86 gdb gcc binutils
```

Note: the Makefile expects `x86_64-linux-gnu-` prefixed binutils; many hosts have those by default. If not, adjust `TOOLPREFIX` in `Makefile`.

## Build & Run

- Build everything:
  - `make`
- Run in QEMU (normal mode):
  - `make qemu`

This produces:
- `xv6/xv6.img`: disk image with bootblock + kernel
- `xv6/fs.img`: filesystem image containing user programs and `README`

## Debugging

### In VS Code

- Choose the debug configuration: “xv6: QEMU”
  - It runs `make qemu-gdb` in the background and attaches GDB to QEMU at port 26000
  - Symbols are loaded from `xv6/kernel`
- Optional tweaks:
  - Set `stopAtEntry` in `.vscode/launch.json` to `false` to avoid halting at entry

### From the terminal

- Start QEMU in gdb mode (paused):
  - `make qemu-gdb`
- Attach GDB (from repo root):
  - `make gdb`
  - or `gdb -x xv6/.gdbinit`

The `.gdbinit` is generated inside `xv6/` from `.gdbinit.tmpl` and sets architecture, connects, and loads symbols.

## Common Make Targets

- `make`: builds kernel, userland, and tools
- `make qemu`: runs kernel with fs image
- `make qemu-gdb`: runs QEMU waiting for GDB
- `make gdb`: launches GDB attached to QEMU
- `make clean`: removes build artifacts

## Notes

- All source and generated binaries stay under `xv6/` to keep the repo root clean.
- The mkfs tool strips directory components so `docs/README` appears as `/README` inside the xv6 filesystem.

Happy hacking!
