# Makefile for the simple kernel.
CC	=gcc
AS	=as
LD	=ld
OBJCOPY = objcopy
OBJDUMP = objdump
NM = nm

CFLAGS = -m32 -Wall -O -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin 

# Add debug symbol
CFLAGS += -g

CFLAGS += -I.

OBJDIR = .

all: kernel.img

$(OBJDIR)/kernel.img: boot/boot kernel/system user/userprog
	dd if=/dev/zero of=$(OBJDIR)/kernel.img count=10000 2>/dev/null
	dd if=$(OBJDIR)/boot/boot of=$(OBJDIR)/kernel.img conv=notrunc 2>/dev/null
	dd if=$(OBJDIR)/kernel/system of=$(OBJDIR)/kernel.img seek=1 conv=notrunc 2>/dev/null
	dd if=$(OBJDIR)/user/userprog of=$(OBJDIR)/kernel.img seek=500 conv=notrunc 2>/dev/null # XXX: Notice size of system

include boot/Makefile
include kernel/Makefile
include lib/Makefile
include user/Makefile

clean:
	rm -rf $(OBJDIR)/boot/*.o $(OBJDIR)/boot/boot.out $(OBJDIR)/boot/boot $(OBJDIR)/boot/boot.asm
	rm -rf $(OBJDIR)/kernel/*.o $(OBJDIR)/kernel/system* kernel.*
	rm -rf $(OBJDIR)/lib/*.o ${OBJDIR}/lib/libnctuos.a
	rm -rf $(OBJDIR)/user/*.o ${OBJDIR}/user/userprog*

qemu: $(OBJDIR)/kernel.img
	qemu-system-i386 -hda kernel.img -monitor stdio

debug: $(OBJDIR)/kernel.img
	qemu-system-i386 -hda kernel.img -monitor stdio -s -S
