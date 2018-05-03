#include "userutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "sha256.h"

login_t *user_data = NULL;

int user_load() {
	struct stat fi;
	int fd;
	fd = open("/passwd", O_RDONLY, 0);
	if (fd == -1) {
		perror("open");
		return -1;
	}
	if (fstat(fd, &fi) == -1) {
		perror("stat");
		close(fd);
		return -1;
	}
	user_data = malloc(fi.st_size);
	if (!user_data) {
		printf("No memory\n");
		close(fd);
		exit(1);
		return -1;
	}
	if (read(fd, user_data, fi.st_size) == -1) {
		perror("read");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

login_t *user_find(char *username) {
	int i;
	if (!user_data)
		user_load();
	for (i = 1; i < user_data[0].uid; i++) {
		if (strcmp(user_data[i].username, username) == 0) {
			return user_data + i;
		}
	}
	return NULL;
}

login_t *user_get(uint32_t uid) {
	int i;
	if (!user_data)
		user_load();
	for (i = 1; i < user_data[0].uid; i++) {
		if (user_data[i].uid ==  uid) {
			return user_data + i;
		}
	}
	return NULL;
}

int user_check_password(login_t *user, char *password) {
	uint8_t buf[32];
	SHA256_CTX ctx;

	sha256_init(&ctx);
	sha256_update(&ctx, user->salt, 8);
	sha256_update(&ctx, (uint8_t*) password, strlen(password));
	sha256_final(&ctx, buf);

	return strncmp((char*)buf, (char*)user->password, 32);
}
