# Based on xv6-riscv Makefile; see LICENSE.

TOOLPREFIX=riscv64-unknown-elf-
QEMU=qemu-system-riscv64

CC=$(TOOLPREFIX)gcc
AS=$(TOOLPREFIX)as
LD=$(TOOLPREFIX)ld
OBJCOPY=$(TOOLPREFIX)objcopy
OBJDUMP=$(TOOLPREFIX)objdump

ULIB_OBJS = \
	start.o \
	string.o \
	syscall.o


ALL_TARGETS = \
	bin/trek \
	bin/rule30 \
	bin/init0 \
	bin/init1 \
	bin/init2 \
	bin/init3 \
	bin/init4 \
	bin/init5 \
	bin/init_trek_rule30 \
	bin/init_fib_rule30 \
	bin/init_fib_fib \
	bin/fib


CFLAGS = -Wall -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-pie -no-pie -march=rv64g -mabi=lp64d
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -fno-asynchronous-unwind-tables
CFLAGS += -I.

all: $(ALL_TARGETS)

bin/trek: $(ULIB_OBJS) trek.o
	$(LD) -T user.ld -o $@ $^

bin/rule30: $(ULIB_OBJS) rule30.o
	$(LD) -T user.ld -o $@ $^
	
bin/troll: $(ULIB_OBJS) troll.o
	$(LD) -T user.ld -o $@ $^

bin/init0: $(ULIB_OBJS) init0.o
	$(LD) -T user.ld -o $@ $^

bin/init1: $(ULIB_OBJS) init1.o
	$(LD) -T user.ld -o $@ $^

bin/init2: $(ULIB_OBJS) init2.o
	$(LD) -T user.ld -o $@ $^

bin/init3: $(ULIB_OBJS) init3.o
	$(LD) -T user.ld -o $@ $^

bin/init4: $(ULIB_OBJS) init4.o
	$(LD) -T user.ld -o $@ $^

bin/init5: $(ULIB_OBJS) init5.o
	$(LD) -T user.ld -o $@ $^

bin/fib: $(ULIB_OBJS) fib.o
	$(LD) -T user.ld -o $@ $^

bin/init_trek_rule30: $(ULIB_OBJS) init_trek_rule30.o
	$(LD) -T user.ld -o $@ $^

bin/init_fib_rule30: $(ULIB_OBJS) init_fib_rule30.o
	$(LD) -T user.ld -o $@ $^

bin/init_fib_fib: $(ULIB_OBJS) init_fib_fib.o
	$(LD) -T user.ld -o $@ $^


clean:
	rm -rf *.o *.elf *.asm $(ALL_TARGETS)
