#include "elf.h"

#include "../fs/vfs.h"
#include "task.h"
#include "../../libc/include/sys/stat.h"

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

int elf_load(int fd)  {
	elf_eheader_t eh;
	elf_pheader_t ph;
	task_t *proc;
	int i, ret, idx, idx0;
	uint32_t addr, align_off;
	task_ptentry_t *ptent;
	stat_t file_stat;
	proc = task_list + task_current_pid();

	ret = syscall_read(fd, (int)&eh, sizeof(eh));
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

	if (eh.abi != 0x03 || eh.abi_ver != 0x91) {
		// Not properly x-compiled, just assume it comes from ece391
		// Only 1 segment is present by presumption. Do NOT load PH from ELF
		eh.phoff = 0;
		eh.phentsize = 0;
		eh.phnum = 1;
		// PH is instead hardcoded as follow
		ph.type = 1;
		ph.offset = 0;
		ph.vaddr = 0x08048000; // Default load addr
		syscall_fstat(fd, (int)(&file_stat), 0);
		ph.filesz = (uint32_t)file_stat.st_size;
		ph.flags = 7; // rwx all granted
		ph.align = 0x1000; // 4kb aligned
	}

	// Set entry point
	proc->regs.eip = eh.entry;

	// Prepare to create new memory pages
	for (idx = 0; idx < TASK_MAX_PAGE_MAPS; idx++) {
		if (!(proc->pages[idx].pt_flags & PAGE_TAB_ENT_PRESENT))
			break;
	}
	if (idx == TASK_MAX_PAGE_MAPS) {
		// Very bad!!!
		return -ENOMEM;
	}

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
			 addr < ph.vaddr+ph.filesz;
			 addr += ph.align) {
			if (idx == TASK_MAX_PAGE_MAPS) {
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
		ret = syscall_read(fd, ph.vaddr, ph.filesz);
		if (ret != (int)ph.filesz) {
			return -EIO;
		}
		if (!(ph.flags & 2)) {
				// Page is readonly
			for (i = idx0 ; i < idx; i++) {
				ptent = proc->pages + i;
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
	}
	page_flush_tlb();

	return 0;
}
