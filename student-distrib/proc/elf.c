#include "elf.h"

#include "../fs/vfs.h"
#include "task.h"
#include "../../libc/include/sys/stat.h"
#include "../k_mem/kmalloc.h"

int elf_load_pheader(int fd, int offset, elf_pheader_t *ph) {
	int ret;

	if (offset <= 0) {
		// Do NOTHING if offset is invalid
		// This feature is used to workaround ECE 391 messing up with ELF
		// segment definition format
		return 0;
	}

	ret = syscall_lseek(fd, offset, SEEK_SET);
	if (ret < 0) {
		return ret;
	}
	ret = syscall_read(fd, (int)ph, sizeof(elf_pheader_t));
	if (ret != sizeof(elf_pheader_t)) {
		return -EIO;
	}
	return 0;
}

int elf_sanity(int fd) {
	elf_eheader_t eh;
	int ret;

	ret = syscall_read(fd, (int)&eh, sizeof(eh));
	if (ret < 0) {
		return ret;
	}
	if (ret != sizeof(eh)) {
		return -EIO;
	}
	
	// Sanity checks
	if (eh.magic[0] != '\x7f' || eh.magic[1] != 'E' ||
		eh.magic[2] != 'L' || eh.magic[3] != 'F') {
		return -ENOEXEC;
	}
	
	if (eh.arch != 1) {
		// Not 32 bits
		return -ENOEXEC;
	}
	if (eh.endianness != 1) {
		// Not LE
		return -ENOEXEC;
	}
	if (eh.machine != 3) {
		// Not x86
		return -ENOEXEC;
	}
	
	return 0;
}

int elf_load(int fd)  {
	elf_eheader_t eh;
	elf_pheader_t ph;
	task_t *proc;
	int i, j, ret, idx = 0, idx0;
	uint32_t addr, align_off, brk = 0;
	task_ptentry_t *ptent;
	// stat_t file_stat;
	proc = task_list + task_current_pid();

	ret = syscall_read(fd, (int)&eh, sizeof(eh));
	if (ret < 0) {
		return ret;
	}
	if (ret != sizeof(eh)) {
		return -EIO;
	}

	// Sanity checks
	if (eh.magic[0] != '\x7f' || eh.magic[1] != 'E' ||
		eh.magic[2] != 'L' || eh.magic[3] != 'F') {
		return -ENOEXEC;
	}

	if (eh.arch != 1) {
		// Not 32 bits
		return -ENOEXEC;
	}
	if (eh.endianness != 1) {
		// Not LE
		return -ENOEXEC;
	}
	if (eh.machine != 3) {
		// Not x86
		return -ENOEXEC;
	}

	// Set entry point
	proc->regs.eip = eh.entry;

	// Probe total memory usage
	proc->page_limit = 0;
	for (i = 0; i < eh.phnum; i++) {
		ret = elf_load_pheader(fd, eh.phoff + i*eh.phentsize, &ph);
		if (ret < 0) {
			return ret;
		}
		if (ph.align == 4<<10) {
			proc->page_limit += ph.memsz / (4<<10) + 1;
		}
	}
	proc->page_limit += 16; // For stacks and heaps
	
	ptent = kmalloc(proc->page_limit * sizeof(task_ptentry_t));
	if (!ptent) {
		return -ENOMEM;
	}
	
	// Copy existing pages
	if (proc->pages) {
		for (idx = 0; idx < 16; idx++) {
			if (!(proc->pages[idx].pt_flags & PAGE_DIR_ENT_PRESENT))
				break;
			ptent[idx] = proc->pages[idx];
		}
	}
	proc->pages = ptent;

	// Read all segments
	for (i = 0; i < eh.phnum; i++) {
		ret = elf_load_pheader(fd, eh.phoff + i*eh.phentsize, &ph);
		if (ret < 0) {
			return ret;
		}
		if (ph.type != 1) {
			// Ignore non PT_LOAD segments
			continue;
		}
		if (ph.align != (4<<10) && ph.align != (4<<20)) {
			// Invalid align field
			return -ENOEXEC;
		}
		// Setup all pages on page table
		align_off = ph.vaddr & (ph.align-1);
		if ((ph.offset & (ph.align-1)) != align_off) {
			// Bad alignment
			return -ENOEXEC;
		}
		idx0 = idx;
		for (addr = ph.vaddr - align_off;
			 addr < ph.vaddr+ph.memsz;
			 addr += ph.align) {
			if (idx == proc->page_limit) {
				// No more pages may be allocated for this process
				return -ENOMEM;
			}
			ptent = proc->pages + idx;
			// Flags are the same for page directories and page tables
			ptent->pt_flags = PAGE_DIR_ENT_USER;
			ptent->paddr = 0;
			if (ph.align == (4<<10)) {
				// Allocate 4KB pages
				ret = page_alloc_4KB((int *)&(ptent->paddr));
			} else {
				// Allocate 4MB pages
				ret = page_alloc_4MB((int *)&(ptent->paddr));
				ptent->pt_flags |= PAGE_DIR_ENT_4MB;
			}
			if (ret != 0) {
				// Page allocation failed. Probably ENOMEM
				return ret;
			}
			ptent->pt_flags |= PAGE_DIR_ENT_RDWR;
			ptent->vaddr = addr;
			ptent->pt_flags |= PAGE_DIR_ENT_PRESENT;
			ptent->priv_flags = 0;

			if (ptent->pt_flags & PAGE_DIR_ENT_4MB) {
				// Add 4MB page table entry
				ret = page_dir_add_4MB_entry(ptent->vaddr, ptent->paddr, ptent->pt_flags);
			} else {
				// Add 4KB page table entry
				ret = page_tab_add_entry(ptent->vaddr, ptent->paddr, ptent->pt_flags);
			}
			if (ret != 0) {
				printf("Mapping failed. %d\n", ret);
			}
			idx++;
		}
		page_flush_tlb();
		ret = syscall_lseek(fd, ph.offset, SEEK_SET);
		if (ret < 0) {
			return ret;
		}
		if (ph.memsz < ph.filesz) {
			// Copy partial file into memory
			ret = syscall_read(fd, ph.vaddr, ph.memsz);
			if (ret >= 0) {
				ret -= ph.memsz;
			} else {
				return ret;
			}
		} else {
			// Copy full file into memory
			ret = syscall_read(fd, ph.vaddr, ph.filesz);
			if (ret >= 0) {
				ret -= ph.filesz;
			} else {
				return ret;
			}
			if (ph.memsz > ph.filesz) {
				// fill the rest with zeros
				memset((uint8_t *)(ph.vaddr+ph.filesz), 0, ph.memsz-ph.filesz);
			}
		}
		if (ret != 0) {
			return -EIO;
		}
		if (!(ph.flags & 2)) {
				// Page is readonly
			for (j = idx0 ; j < idx; j++) {
				ptent = proc->pages + j;
				ptent->pt_flags &= ~PAGE_DIR_ENT_RDWR;
				if (ptent->pt_flags & PAGE_DIR_ENT_4MB) {
					// Reload 4MB page table entry
					ret = page_dir_delete_entry(ptent->vaddr);
					if (ret != 0) {
						printf("DeMapping failed. %d\n", ret);
					}
					ret = page_dir_add_4MB_entry(ptent->vaddr, ptent->paddr, ptent->pt_flags);
					if (ret != 0) {
						printf("ReMapping failed. %d\n", ret);
					}
				} else {
					// Reload 4KB page table entry
					ret = page_tab_delete_entry(ptent->vaddr);
					if (ret != 0) {
						printf("DeMapping failed. %d\n", ret);
					}
					ret = page_tab_add_entry(ptent->vaddr, ptent->paddr, ptent->pt_flags);
					if (ret != 0) {
						printf("ReMapping failed. %d\n", ret);
					}
				}
			}
		}
		if (ph.vaddr + ph.memsz > brk) {
			brk = ph.vaddr + ph.memsz;
		}
	}
	page_flush_tlb();
	proc->heap.start = proc->heap.prog_break = (brk & ~((4<<20)-1)) + (4<<20);
	return 0;
}
