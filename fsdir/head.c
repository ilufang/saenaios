#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
	char buf[256];
	int fd, ret;
	if (argc > 1) {
		fd = open(argv[1], O_RDONLY);
		if (fd == -1) {
			perror("open");
		}
	} else {
		fd = 0;
	}
	ret = read(fd, buf, 256);
	write(1, buf, ret);
	return 0;
}
