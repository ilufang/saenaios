#include <stdio.h>
#include <unistd.h>

int main() {
	char buf[1024];
	if (!getcwd(buf, 1023)) {
		perror("getcwd");
		return 1;
	} else {
		printf("%s\n", buf);
	}
	return 0;
}
