#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "userutils.h"
#include "sha256.h"

void user_set_password(login_t *user, char *password) {
	SHA256_CTX ctx;
	int fd;

	fd = open("/dev/urandom", O_RDONLY);
	read(fd, user->salt, 8);
	close(fd);

	sha256_init(&ctx);
	sha256_update(&ctx, user->salt, 8);
	sha256_update(&ctx, (uint8_t*)password, strlen(password));
	sha256_final(&ctx, user->password);
}


int main() {
	int fd;
	login_t userlist[4];
	memset(userlist, 0, sizeof(userlist));
	userlist[0].uid = 4;
	strcpy(userlist[0].username, "SaenaiOS shadow passwd");
	printf("Sizeof user: %ld\n", sizeof(login_t));
	printf("Sizeof list: %ld\n", sizeof(userlist));
	userlist[1].uid = 0;
	userlist[1].gid = 0;
	strcpy(userlist[1].username, "root");
	strcpy(userlist[1].login_shell, "/sh");
	strcpy(userlist[1].home_dir, "/");
	user_set_password(userlist+1, "supersecret");

	userlist[2].uid = 1;
	userlist[2].gid = 1;
	strcpy(userlist[2].username, "megumi");
	strcpy(userlist[2].login_shell, "/sh");
	strcpy(userlist[2].home_dir, "/");
	user_set_password(userlist+2, "blessing_soft");

	userlist[3].uid = 2;
	userlist[3].gid = 2;
	strcpy(userlist[3].username, "user");
	strcpy(userlist[3].login_shell, "/shell");
	strcpy(userlist[3].home_dir, "/");
	user_set_password(userlist+3, "ece391");

	fd = open("passwd", O_CREAT | O_TRUNC | O_WRONLY, 0644);
	write(fd, userlist, sizeof(userlist));
	close(fd);
	return 0;
}
