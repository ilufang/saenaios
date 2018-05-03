#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "userutils/userutils.h"

int main() {
	int i = 5;
	char username[33], password[33];
	login_t *login;
	int rtc_fd;
	char *argv[2];

	putchar('\x0c'); // Clear
	fflush(stdout);
	printf("Welcome to SaenaiOS\n\n");

	rtc_fd = open("/dev/rtc", O_RDONLY);

	while (i --> 0) {
		printf("login: ");
		fflush(stdout);
		if (scanf("%s", username) != 1) {
			perror("scanf");
			return 1;
		}
		printf("Password: ");
		fflush(stdout);
		if (scanf("%s", password) != 1) {
			perror("scanf");
			return 1;
		}
		login = user_find(username);
		if (login && user_check_password(login, password) == 0) {
			// Success
			if (setuid(login->uid) == -1) {
				perror("setuid");
			}
			if (chdir(login->home_dir) == -1) {
				perror("chdir");
			}
			argv[1] = login->login_shell;
			argv[2] = NULL;
			if (execve(login->login_shell, argv, NULL)) {
				perror("execve");
			}
			printf("Press enter to continue...");
			fflush(stdout);
			getchar();
			return 0;
		}
		if (rtc_fd > 0) {
			read(rtc_fd, username, 0);
			read(rtc_fd, username, 0);
			read(rtc_fd, username, 0);
			read(rtc_fd, username, 0);
		}
		printf("\nInvalid username or password.\n\n");
	}
	printf("Too many unsuccessful login attempts.\n");
	return 0;
}
