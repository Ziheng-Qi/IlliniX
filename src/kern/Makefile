TOOLPREFIX=riscv64-unknown-elf-
QEMU=qemu-system-riscv64

CC=$(TOOLPREFIX)gcc
AS=$(TOOLPREFIX)as
LD=$(TOOLPREFIX)ld
OBJCOPY=$(TOOLPREFIX)objcopy
OBJDUMP=$(TOOLPREFIX)objdump

CORE_OBJS = \
	start.o \
	halt.o \
	string.o \
	trapasm.o \
	intr.o \
	io.o \
	device.o \
	vioblk.o \
	virtio.o \
	console.o \
	plic.o \
	timer.o \
	thread.o \
	thrasm.o \
	ezheap.o \
	uart.o \
	kfs.o \
	elf.o \
	# Add the names of new objects here

CORE_OBJS_NO_VIO = \
	start.o \
	halt.o \
	string.o \
	trapasm.o \
	intr.o \
	io.o \
	device.o \
	console.o \
	plic.o \
	thrasm.o \
	thread.o \
	uart.o \
	timer.o \
	ezheap.o \
	kfs.o \
	elf.o \
	# Add the names of new objects here

CFLAGS = -Wall -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-pie -no-pie -march=rv64g -mabi=lp64d
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -fno-asynchronous-unwind-tables
CFLAGS += -I. -DDEBUG -DTRACE

QEMUOPTS = -global virtio-mmio.force-legacy=false
QEMUOPTS += -machine virt -bios none -kernel $< -m 8M -nographic
QEMUOPTS += -serial mon:stdio
QEMUOPTS += -drive file=kfs.raw,id=blk0,if=none,format=raw
QEMUOPTS += -device virtio-blk-device,drive=blk0
QEMUOPTS += -serial pty
test_fs.elf: $(CORE_OBJS_NO_VIOBLK) main_tests.o companion.o
	$(LD) -T kernel.ld -o $@ $^
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)

all: shell.elf test_fs.elf vioblk.elf 

shell.elf: $(CORE_OBJS) main_shell.o companion.o
	$(LD) -T kernel.ld -o $@ $^

vioblk.elf: $(CORE_OBJS) main_test_vio.o
	$(LD) -T kernel.ld -o $@ $^

test_fs.elf: $(CORE_OBJS_NO_VIO) main_tests.o companion.o
	$(LD) -T kernel.ld -o $@ $^

run-test_fs: test_fs.elf
	$(QEMU) $(QEMUOPTS)

debug-test_fs: test_fs.elf
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

run-shell: shell.elf
	$(QEMU) $(QEMUOPTS)

debug-shell: shell.elf
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

debug-vioblk: vioblk.elf
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

# You might want to add more here

clean:
	if [ -f companion.o ]; then cp companion.o companion.o.save; fi
	rm -rf *.o *.elf *.asm
	if [ -f companion.o.save ]; then mv companion.o.save companion.o; fi

companion.o:
	if [ -f ../user/trek ]; then sh ./mkcomp.sh ../user/trek; fi
	if ! [ -f ../user/trek ]; then sh ./mkcomp.sh; fi

