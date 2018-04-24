/**
 *	@file proc/elf.h
 *
 *	ELF utilities. Parse the format of ELFs.
 */
#ifndef PROC_ELF_H
#define PROC_ELF_H

#include "../types.h"

/**
 *	ELF header. Contains information about the layout of the ELF file
 */
typedef struct elf_eheader_s {
	char magic[4]; ///< The ELF magic header. Should be 0x7F, 'E', 'L', 'F'
	uint8_t arch; ///< Should be 1 for 32 bits
	uint8_t endianness; ///< Should be 1 for LE
	uint8_t version1; ///< ELF version. Ignored
	uint8_t abi; ///< Operating system ABI. 0x03 for non mp3 x-compiled ELF
	uint8_t abi_ver; ///< ABI version. 0x91 for non mp3 x-compiled ELF (0x0391)
	uint8_t padding[7]; ///< Unused padding
	uint16_t type; ///< Unused
	uint16_t machine; ///< Target ISA. Should be 3 for x86
	uint32_t version2; ///< Unused
	uint32_t entry; ///< Entry point address
	uint32_t phoff; ///< Program header offset
	uint32_t shoff; ///< Section header offset
	uint32_t flags; ///< Unused
	uint16_t ehsize; ///< Size of elf header. Should be 52
	uint16_t phentsize; ///< Size of program header entry
	uint16_t phnum; ///< Number of program header entries
	uint16_t shentsize; ///< Size of section header entry
	uint16_t shnum; ///< Number of section header entries
	uint16_t shstrndx; ///< Index of the section header entry with section names
} __attribute__((__packed__)) elf_eheader_t;

/**
 *	Program header. Contains information about segments to be loaded
 */
typedef struct elf_pheader_s {
	uint32_t type; ///< Segment type. Should be 1 for program contents
	uint32_t offset; ///< File offset of segment in ELF file
	uint32_t vaddr; ///< Virtual address of segment
	uint32_t paddr; ///< Physical address of segment. Ignored
	uint32_t filesz; ///< Segment size
	uint32_t memsz; ///< Segment size in memory. Ignored
	uint32_t flags; ///< Permission bits
	uint32_t align; ///< Segment alignment information. Should be 4KB or 4MB
} __attribute__((__packed__)) elf_pheader_t;

/**
 *	Load ELF segments from a file into current process
 *
 *	@param fd: the file descriptor of the ELF file opened for reading
 *	@return 0 on success, or the negative of an errno on failure.
 */
int elf_load(int fd);

#endif
