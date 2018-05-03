#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "userutil.h"

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
		printf("%s\n", entry->filename);
	}
	closedir(dir);
	return 0;
}
