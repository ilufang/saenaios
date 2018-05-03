#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "userutils/userutils.h"

#define BUFSIZE 1024
#define MAX_STOP_JOBS	16

pid_t *proc_stop;
char *username;
char prompt = '>';

void wait_child() {
	int pid, i, ret;

	pid = wait(&ret);
	if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
		fprintf(stderr, "Program exited with status %d\n", WEXITSTATUS(ret));
	}
	if (WIFSIGNALED(ret)) {
		fprintf(stderr, "Program killed by signal %d\n", WTERMSIG(ret));
	}
	if (WIFSTOPPED(ret)) {
		for (i = 0; i < MAX_STOP_JOBS; i++) {
			if (proc_stop[i] == 0) {
				proc_stop[i] = pid;
				printf("[%d]   Stopped                 %d\n", i, pid);
				break;
			}
		}
	}
}

int main ()
{
	int cnt, ret, argc, is_space, i;
	char *buf;
	char *argv[16], *cptr, c, *redir_in, *redir_out, *redir_err;
	pid_t pid;
	login_t *login;

	buf = malloc(BUFSIZE + 1);
	proc_stop = calloc(MAX_STOP_JOBS, sizeof(pid_t));

	ret = getuid();
	if (ret == -1) {
		perror("getuid");
		username = "";
	} else {
		login = user_get(ret);
		username = login->username;
		if (ret == 0) {
			prompt = '#';
		} else {
			prompt = '$';
		}
	}

	while (1) {
		// Print prompt
		if (getcwd(buf, BUFSIZE) == NULL) {
			printf("%s%c ", username, prompt);
		} else {
			printf("%s %s%c ", buf, username, prompt);
		}
		fflush(stdout);

		// Read command
		cnt = read(0, buf, BUFSIZE-1);
		if (cnt == -1) {
			printf("Stdin exhausted. Bye.\n");
			return 0;
		}
		if (cnt > 0 && '\n' == buf[cnt - 1])
			cnt--;
		buf[cnt] = '\0';
		if ('\0' == buf[0])
			continue;
		// Parse arguments
		argc = 0;
		is_space = 1;
		cptr = buf;
		redir_in = NULL;
		redir_out = NULL;
		redir_err = NULL;
		while ((c = *cptr++)) {
			if (is_space) {
				if (c == ' ' || c == '\t') {
					continue;
				}
				is_space = 0;
				if (c == '<') {
					redir_in = cptr;
					continue;
				}
				if (c == '>') {
					redir_out = cptr;
					continue;
				}
				if (c == '2' && *cptr == '>') {
					redir_err = cptr++;
					continue;
				}
				argv[argc++] = cptr-1;
			} else {
				if (c == ' ' || c == '\t') {
					is_space = 1;
					cptr[-1] = '\0';
				}
			}
		}
		argv[argc] = NULL;
		if (strcmp(argv[0], "exit") == 0) {
			return 0;
		}
		if (strcmp(argv[0], "cd") == 0) {
			ret = chdir(argv[1]);
			if (ret == -1) {
				perror("chdir");
			}
			continue;
		}
		if (strcmp(argv[0], "jobs") == 0) {
			for (i = 0; i < MAX_STOP_JOBS; i++) {
				if (proc_stop[i]) {
					printf("[%d]   Stopped                 %d\n", i, proc_stop[i]);
				}
			}
			continue;
		}
		if (strcmp(argv[0], "fg") == 0) {
			if (argc < 1) {
				printf("Usage: fg [job-id]\n");
			} else {
				if (sscanf(argv[1], "%d", &pid) != 1) {
					printf("fg: no such job\n");
				} else if (pid > 16 || pid < 0) {
					printf("fg: %d: no such job\n", pid);
				} else if (proc_stop[pid] == 0) {
					printf("fg: %d: no such job\n", pid);
				} else {
					kill(pid, SIGCONT);
					wait_child();
				}
			}
			continue;
		}
		ret = fork();
		if (ret == -1) {
			continue;
		}
		if (ret != 0) {
			// parent
			wait_child();
		} else {
			// child
			if (redir_in) {
				if (close(0) == -1) {
					perror("close original stdin");
					return 1;
				}
				if (open(redir_in, O_RDONLY, 0) != 0) {
					perror("open new stdin");
					return 1;
				}
			}
			if (redir_out) {
				if (close(1) == -1) {
					perror("close original stdout");
					return 1;
				}
				if (open(redir_out, O_WRONLY | O_CREAT, 0) != 1) {
					perror("open new stdout");
					return 1;
				}
			}
			if (redir_err) {
				if (close(2) == -1) {
					perror("close original stderr");
					return 1;
				}
				if (open(redir_err, O_WRONLY | O_CREAT, 0) != 2) {
					perror("open new stderr");
					return 1;
				}
			}
			cptr = NULL;
			ret = fork();
			if (ret == -1) {
				perror("fork");
				return 1;
			}
			if (ret != 0) {
				wait(&ret);
				return 0;
			} else {
				ret = execve(argv[0], argv, &cptr);
				if (ret < 0) {
					perror("execve");
					return 1;
				}
			}
		}
	}
}
