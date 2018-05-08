#include <unistd.h>
#include <stdio.h>

int main() {
	int ret;

	ret = fork();
	if (ret != 0) {
		while(1) {
			printf("Parent... Child PID is %d\n", ret);
		}
	} else {
		while (1) {
			printf("Child...\n");
		}
	}
	return 0;
}
