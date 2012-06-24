#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "parser.h"

void close_proc(pid_t *pids, int count) {
	for(int i = 0; i < count; ++i) {
		if(pids[i] != -1) {
			kill(pids[i], SIGKILL);
		}
	}
}

int main(int argc, char *argv[]) {
	struct dictionary d;
	int **pipes = (int **)malloc(sizeof(int *) * (argc - 1));
	if(pipes == NULL) {
		fprintf(stderr, "Error memory.\n");
		return 1;
	}
	pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * (argc - 1));
	if(pids == NULL) {
		fprintf(stderr, "Error memory.\n");
		free(pipes);
		return 1;
	}
	for(int i = 0; i < (argc - 1); ++i) {
		pids[i] = -1;
	}
	for(int i = 0; i < argc - 1; ++i) {
		pipes[i] = (int *)malloc(sizeof(int) * 2);
		if(pipes[i] == NULL) {
			for(int j = i - 1; j >= 0; --j) {
				free(pipes[j]);
			}
			free(pipes);
			free(pids);
			fprintf(stderr, "Error memory.\n");
			return 1;
		}
	}
	for(int i = 0; i < argc - 1; ++i) {
		if(pipe(pipes[i]) != 0) {
			fprintf(stderr, "Error pipe.");
			for(int j = i - 1; j >= 0; --j) {
				close(pipes[j][0]);
				close(pipes[j][1]);
			}
			close_proc(pids, argc - 1);
			for(int j = 0; j < argc - 1; ++j) {
				free(pipes[j]);
			}
			free(pids);
			free(pipes);
			return 1;
		}
		if(split(argv[i + 1], &d) != 0) {
			fprintf(stderr, "Error split.\n");
			close_proc(pids, argc - 1);
			clear_dict(&d);
			for(int j = i; j >= 0; --j) {
				close(pipes[j][0]);
				close(pipes[j][1]);
			}
			for(int j = 0; j < argc - 1; ++j) {
				free(pipes[j]);
			}
			free(pids);
			free(pipes);
			return 1;
		}
		if(i > 1) {
			close(pipes[i - 1][1]);
			close(pipes[i - 2][0]);
		} else if(i > 0) {
			close(pipes[i - 1][1]);
		}
		pids[i] = fork();
		if(pids[i] < 0) {
			clear_dict(&d);
			for(int j = i; j >= 0; --j) {
				close(pipes[j][0]);
				close(pipes[j][1]);
			}
			for(int j = 0; j < argc - 1; ++j) {
				free(pipes[j]);
			}
			free(pids);
			free(pipes);
			fprintf(stderr, "Error fork.\n");
			close_proc(pids, argc - 1);
			return 1;
		} else if(pids[i] == 0) {
			if(i == 0) {
				int pipe_out = pipes[i][1];
				close(pipes[i][0]);
				if(i != argc - 2) {
					dup2(pipe_out, STDOUT_FILENO);
				} else {
					close(pipe_out);
				}
			} else {
				int pipe_out = pipes[i][1];
				close(pipes[i][0]);
				int pipe_in = pipes[i - 1][0];
				if(i != argc - 2) {
					dup2(pipe_out, STDOUT_FILENO);
				} else {
					close(pipe_out);
				}
				dup2(pipe_in, STDIN_FILENO);
			}
			if(execvp(d.words[0], d.words) == -1) {
				clear_dict(&d);
				for(int z = 0; z < argc - 1; ++z) {
					free(pipes[z]);
				}
				free(pids);
				free(pipes);
				return 1;
			}
		}
		for(int j = 0; j <= i; ++j) {
			int w_stat;
			pid_t tmp_pid = waitpid(pids[j], &w_stat, WNOHANG);
			if(tmp_pid != 0) {
				if(WEXITSTATUS(w_stat)) {
					close_proc(pids, argc - 1);
					fprintf(stderr, "Error child process.\n");
					for(int j = i; j >= 0; --j) {
						close(pipes[j][0]);
						close(pipes[j][1]);
					}
					for(int z = 0; z < argc - 1; ++z) {
						free(pipes[z]);
					}
					free(pids);
					free(pipes);
					clear_dict(&d);
					return 1;
				}
			}
		}
		clear_dict(&d);
	}
	if(argc > 1) {
		close(pipes[argc - 2][1]);
	}
	if(argc > 2) {
		close(pipes[argc - 3][1]);
	}
	while(1) {
		int status;
		pid_t pid = wait(&status);
		if(!(pid > 0)) {
			break;
		}
		if(WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
			for(int i = 0; i < argc - 1; ++i) {
				if (pids[i] == pid) {
					pids[i] = -1;
					break;
				}
			}
		} else {
			for(int i = 0; i < argc - 1; ++i) {
				if (pids[i] == pid) {
					pids[i] = -1;
					break;
				}
			}
			fprintf(stderr, "Error child process.\n");
			close_proc(pids, argc - 1);
			for(int j = 0; j < argc - 1; ++j) {
				free(pipes[j]);
			}
			free(pids);
			free(pipes);
			return 1;
		}
	}
	for(int j = 0; j < argc - 1; ++j) {
		free(pipes[j]);
	}
	free(pids);
	free(pipes);
	return 0;
}
