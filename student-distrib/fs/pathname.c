#include "pathname.h"

#include "../lib.h"
#include "../errno.h"

int path_cd(pathname_t path, const char *relpath) {
	int ptr;

	if (!path || !relpath || path[0] != '/') {
		return -EINVAL;
	}

	ptr = strlen(path);

	if (path[ptr-1] != '/') {
		// Enforce trailing separator
		path[ptr++] = '/';
		path[ptr] = '\0'; // In case of abnormal exit, maintain path validity
	}

	if (relpath[0] == '/') {
		// CD to absolute path
		ptr = 1;
		relpath++;
	}

	if (ptr + strlen(relpath) >= PATH_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	while (*relpath != '\0') {
		// Process next path element
		if (relpath[0] == '.') {
			if (relpath[1] == '/') {
				relpath += 2; // Skip "./"
				continue;
			} else if (relpath[1] == '\0') {
				break;
			} else if (relpath[1] == '.') {
				if (relpath[2] == '/') {
					relpath += 3; // Skip "../"
					if (ptr != 1) {
						// If not at root, pop 1 level
						for (ptr-=2; path[ptr] != '/'; ptr--);
						ptr++; // Move 1 right, after the '/'
					}
					continue;
				} else if (relpath[2] == '\0') {
					if (ptr != 1) {
						// If not at root, pop 1 level
						for (ptr-=2; path[ptr] != '/'; ptr--);
						ptr++; // Move 1 right, after the '/'
					}
					break;
				}
			}
		} else if (*relpath == '/') {
			// Skip consecutive '/'s
			relpath++;
			continue;
		} else {
			// Push next component
			for(; (*relpath != '/' && *relpath != '\0'); relpath++) {
				path[ptr++] = *relpath;
			}
			if (*relpath == '/'){
				relpath++;
				path[ptr++] = '/';
			}
		}
	}

	path[ptr] = '\0';

	return 0;
}

