#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

struct msgbuf {
    long mtype;
    char *mtext;
};

void close_proc(pid_t *pids, int count) {
	for(int i = 0; i < count; ++i) {
		if(pids[i] != -1) {
			kill(pids[i], SIGKILL);
		}
	}
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Error: use %s with one argument.\n", argv[0]);
		return 1;
	}
	char *endptr = NULL;
	long int n = strtol(argv[1], &endptr, 10);
	if((n == LONG_MAX || n == LONG_MIN) && errno == ERANGE) {
		perror("strtol");
		return 1;		
	}
	if(argv[1] == endptr) {
		fprintf(stderr, "Error: no digits were found.\n");
		return 1;
	}
	if(n <= 0) {
		fprintf(stderr, "Error: %ld is'n positive number.\n", n);
		return 1;
	}
	pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * n);
	if(pids == NULL) {
		perror("malloc");
		return 1;
	}
	for(int i = 0; i < n; ++i)
		pids[i] = -1;
	key_t key = 11;
	int mfd = msgget(key, IPC_CREAT | 0666);
	if(mfd == -1) {
		perror("msgget");
		free(pids);
		return 1;
	}
	struct msgbuf msg;
	msg.mtype = 1;
	msg.mtext = NULL;
	if(msgsnd(mfd, &msg, 0, 0) == -1) {
		perror("msgsnd");
		free(pids);
		msgctl(mfd, IPC_RMID, NULL);
		return 1;
	}
	long int i;
	for(i = 1; i <= n; ++i) {
		pids[i - 1] = fork();
		if(pids[i - 1] < 0) {
			perror("fork");
			msgctl(mfd, IPC_RMID, NULL);
			close_proc(pids, n);
			free(pids);
			return 1;
		} else if(pids[i - 1] == 0) {
			free(pids);
			struct msgbuf tmp_msg;
			if(msgrcv(mfd, &tmp_msg, 0, i, 0) < 0) {
				perror("msgrcv");
				return 1;
			}
			printf("%ld ", i);
			fflush(stdout);
			if(i < n) {
				tmp_msg.mtype = i + 1;
				if(msgsnd(mfd, &tmp_msg, 0, 0) == -1) {
					perror("msgsnd");
					return 1;
				}
			}
			return 0;
		}
		for(int j = 1; j <= i; ++j) {
			int w_stat;
			pid_t tmp_pid = waitpid(pids[j - 1], &w_stat, WNOHANG);
			if(tmp_pid != 0) {
				if(WEXITSTATUS(w_stat)) {
					close_proc(pids, n);
					fprintf(stderr, "Error child process.\n");
					free(pids);
					msgctl(mfd, IPC_RMID, NULL);
					return 1;
				}
			}
		}
	}
	while(1) {
		int status;
		pid_t pid = wait(&status);
		if(!(pid > 0)) {
			break;
		}
		if(status == 0) {
			for(int i = 0; i < n; ++i) {
				if (pids[i] == pid) {
					pids[i] = -1;
					break;
				}
			}
		} else {
			fprintf(stderr, "Error child process.\n");
			close_proc(pids, n);
			free(pids);
			msgctl(mfd, IPC_RMID, NULL);
			return 1;
			break;
		}
	}
	printf("\n");
	free(pids);
	msgctl(mfd, IPC_RMID, NULL);
	return 0;
}
