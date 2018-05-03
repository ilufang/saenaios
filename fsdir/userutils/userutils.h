#ifndef USERUTIL_H
#define USERUTIL_H

#include <stdlib.h>
#include <stdint.h>

typedef struct s_login {
	uint32_t uid;
	uint32_t gid;
	uint8_t salt[8];
	char padding[48];
	char username[32];
	uint8_t password[32];
	char login_shell[64];
	char home_dir[64];
} __attribute__((__packed__)) login_t;

login_t *user_find(char *username);

login_t *user_get(uint32_t uid);

int user_check_password(login_t *user, char *password);

#endif
