#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/x86.h>

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
static void
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

struct Elf *
load_elf(uint32_t pa, uint32_t offset)
{
	struct Elf *ehdr = (struct Elf *)pa;
	struct Proghdr *ph, *eph;

	// read 1st page off disk
	readseg(pa, PGSIZE, offset);

	// is this a valid ELF?
	if (ehdr->e_magic != ELF_MAGIC)
		panic("Can not found ELF file");

	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *) ((uint8_t *) ehdr + ehdr->e_phoff);
	eph = ph + ehdr->e_phnum;
	for (; ph < eph; ph++) {
		if (ph->p_type != ELF_PROG_LOAD)
			continue;
		// p_pa is the load address of this segment (as well
		// as the physical address)
		readseg(ph->p_pa, ph->p_filesz, offset + ph->p_offset);
		char *p = (char *)(ph->p_pa + ph->p_filesz);
		for (; p < ph->p_pa + ph->p_memsz; ++p)
			*p = 0;
	}

	// call the entry point from the ELF header
	// note: does not return!
	// ((void (*)(void)) (ehdr->e_entry))();
	return ehdr;

bad:
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8E00);
	while (1)
		/* do nothing */;
}
