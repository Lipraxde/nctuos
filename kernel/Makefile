
GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

KERN_LDFLAGS = -T kernel/kern.ld

KERN_SRCFILES := kernel/entry.S \
		kernel/main.c \
		kernel/picirq.c \
		kernel/kbd.c \
		kernel/screen.c \
		kernel/printf.c \
		kernel/mem.c \
		kernel/entrypgdir.c \
		kernel/assert.c \
		kernel/kclock.c \
		kernel/sched.c \
		kernel/syscall.c \
		kernel/task.c \
		kernel/timer.c \
		kernel/readelf.c \
		kernel/spinlock.c \
		kernel/lapic.c \
		kernel/mpentry.c \
		kernel/mpconfig.c \
		lib/printfmt.c \
		lib/string.c

KERN_OBJS = kernel/entry.o \
	kernel/main.o \
	kernel/picirq.o \
	kernel/kbd.o \
	kernel/screen.o \
	kernel/trap.o \
	kernel/trap_entry.o \
	kernel/printf.o \
	kernel/mem.o \
	kernel/entrypgdir.o \
	kernel/assert.o \
	kernel/kclock.o \
	kernel/sched.o \
	kernel/syscall.o \
	kernel/task.o \
	kernel/timer.o \
	kernel/readelf.o \
	kernel/spinlock.o \
	kernel/lapic.o \
	kernel/mpentry.o \
	kernel/mpconfig.o \
	kernel/drv/disk.o \
	kernel/fs/fat/ff.o \
	kernel/fs/diskio.o \
	kernel/fs/fs_syscall.o \
	kernel/fs/fs_ops.o \
	kernel/fs/fs.o \
	kernel/fs/fs_test.o \
	lib/printfmt.o \
	lib/string.o

KERN_OBJS := $(patsubst %.S, $(OBJDIR)/%.o, $(KERN_OBJS))

$(OBJDIR)/kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -nostdinc -O0 -c -o $@ $<

$(OBJDIR)/kernel/drv/%.o: kernel/drv/%.c
	$(CC) $(CFLAGS) -O0 -c -o $@ $<

$(OBJDIR)/kernel/fs/%.o: kernel/fs/%.c
	$(CC) $(CFLAGS) -O0 -c -o $@ $<

$(OBJDIR)/kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -nostdinc -c -o $@ $<

$(OBJDIR)/kernel/system: $(KERN_OBJS)
	@echo + ld kernel/system
	$(LD) $(KERN_LDFLAGS) $(KERN_OBJS) $(GCC_LIB) -o $@
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym
