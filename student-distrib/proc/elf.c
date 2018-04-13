#include "elf.h"

#include "../fs/vfs.h"
#include "task.h"

int elf_load_pheader(int fd, int offset, elf_pheader_t *ph) {
	int ret;

	if (offset <= 0) {
		// Do NOTHING if offset is invalid
		// This feature is used to workaround ECE 391 messing up with ELF
		// segment definition format
		return 0;
	}

	ret = syscall_lseek(fd, offset);
	if (ret < 0) {
		return ret;
	}
	ret = syscall_read(fd, (int)ph, sizeof(elf_pheader_t));
	if (ret != sizeof(elf_pheader_t)) {
		return -EIO;
	}

}

int elf_load(int fd)  {
	elf_eheader_t eh;
	elf_pheader_t ph;
	task_t *proc;
	int i, ret, idx;

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
		return -ENOEXEC
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
		ph.vaddr = 0x08048000;
		ph.filesz = -1; // TODO get file size
		ph.flags = 7; // rwx all granted
		ph.align = 0x1000; // 4kb aligned
	}


	// Prepare to create new memory pages
	for (idx = 0; idx < TASK_MAX_PAGE_MAPS; idx++) {
		if (!(proc->pages[i].pt_flags & PAGE_TAB_ENT_PRESENT))
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
		// TODO: setup all pages on page table
		ret = syscall_lseek(fd, eh.offset);
		if (ret < 0) {
			return ret;
		}
		ret = syscall_read(fd, ph.vaddr, ph.filesz);
		if (ret != ph.filesz) {
			return -EIO;
		}
		// TODO: set segment permission flags to all pages
	}

	// Setup stack segment
	// TODO: allocate 4MB page from 0xbfc00000 - 0xc0000000 for stack

	return 0;
}
