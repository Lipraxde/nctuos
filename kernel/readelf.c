#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/x86.h>
#include <kernel/task.h>

#define PGSIZE		4096
#define SECTSIZE	512

static void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

static void
readsect(void *dst, uint32_t offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked
void
readseg(uint32_t pa, uint32_t count, uint32_t offset)
{
	uint32_t end_pa;

	end_pa = pa + count;

	// round down to page boundary
	pa &= ~(SECTSIZE - 1);

	offset = (offset / SECTSIZE);

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (pa < end_pa) {
		// Since we haven't enabled paging yet and we're using
		// an identity segment mapping (see boot.S), we can
		// use physical addresses directly.  This won't be the
		// case once JOS enables the MMU.
		readsect((uint8_t*) pa, offset++);
		pa += SECTSIZE;
	}
}

void
load_elf(struct Task *t, uint8_t *binary)
{
	// LAB 3: Your code here.
	struct Elf *elf = (struct Elf *)binary;
	struct Proghdr *eph, *ph = (struct Proghdr *) (binary + (elf->e_phoff));
	eph = ph + elf->e_phnum;
	lcr3(PADDR(t->pgdir));
	for (; ph < eph; ph++)
		if (ph->p_type == ELF_PROG_LOAD) {
			memmove((void *)ph->p_va, binary+ph->p_offset, ph->p_filesz);
			memset((void *)ph->p_va+ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
		}
}
