#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "userutils/userutils.h"

void print_mode(struct stat *statbuf) {
	mode_t m = statbuf->st_mode;
	putchar(m & 0400 ? 'r':'-');
	putchar(m & 0200 ? 'w':'-');
	putchar(m & 0100 ? 'x':'-');
	putchar(m & 040 ? 'r':'-');
	putchar(m & 020 ? 'w':'-');
	putchar(m & 010 ? 'x':'-');
	putchar(m & 04 ? 'r':'-');
	putchar(m & 02 ? 'w':'-');
	putchar(m & 01 ? 'x':'-');
	printf("   ");
	// Print 12 chars
}
void print_owner(struct stat *statbuf) {
	login_t *user;
	if ((user = user_get(statbuf->st_uid))) {
		printf("%-16s", user->username);
	} else {
		printf("%16d", statbuf->st_uid);
	}
	// Print 16 chars
}
void print_size(struct stat *statbuf) {
	size_t size = statbuf->st_size;
	if (size < (1<<10)) {
		printf("%-8d", statbuf->st_size);
	} else if (size < (1<<20)) {
		printf("%-7dK", statbuf->st_size << 10);
	} else if (size < (1<<30)) {
		printf("%-7dM", statbuf->st_size << 20);
	} else {
		printf("%-7dG", statbuf->st_size << 30);
	}
	printf("    ");
	// Print 12 chars
}


int main(int argc, char *argv[]) {
	int ok = 0, i;
	DIR *dir;
	struct dirent *entry;
	struct stat statbuf;

	if (argc == 1) {
		dir = opendir(".");
	} else {
		dir = opendir(argv[1]);
	}

	if (dir == NULL) {
		perror("opendir");
		return 1;
	}
	while((entry = readdir(dir))) {
		// if (stat(entry->filename, &statbuf) == -1) {
		// 	perror("stat");
		// 	// fprintf(stderr, "Disabling stat output...\n");
		// 	printf("%-40s","?????????");
		// 	use_stat = 0;
		// } else {
		// 	print_mode(&statbuf);
		// 	print_owner(&statbuf);
		// 	print_size(&statbuf);
		// }
		printf("%-40s", entry->filename);
	}
	printf("\n");
	closedir(dir);
	return 0;
}
