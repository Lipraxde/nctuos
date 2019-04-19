
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
		kernel/task.c \
		kernel/timer.c \
		kernel/readelf.c \
		lib/printfmt.c \
		lib/readline.c \
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
	kernel/task.o \
	kernel/timer.o \
	kernel/readelf.o \
	lib/printfmt.o \
	lib/readline.o \
	lib/string.o

KERN_OBJS := $(patsubst %.S, $(OBJDIR)/%.o, $(KERN_OBJS))

$(OBJDIR)/kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -nostdinc -O0 -c -o $@ $<

$(OBJDIR)/kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -nostdinc -c -o $@ $<

$(OBJDIR)/kernel/system: $(KERN_OBJS)
	@echo + ld kernel/system
	$(LD) $(KERN_LDFLAGS) $(KERN_OBJS) $(GCC_LIB) -o $@
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym
