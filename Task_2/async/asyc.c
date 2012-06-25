#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>

struct buf {
	char *content;
	size_t pos_end;
	size_t pos_begin;
	size_t size;
	size_t max_size;
};

int new_buf(struct buf *b) {
	b->max_size = 2;
	b->pos_begin = 0;
	b->pos_end = 0;
	b->size = 0;
	b->content = (char *)malloc(sizeof(char) * b->max_size);
	if(b->content == NULL)
		return -1;
	return 0;
}

int buf_add(struct buf *b, char c) {
	if(b->size == b->max_size) {
		(b->max_size) *= 2;
		char *tmp = b->content;
		b->content = (char *)realloc(b->content, sizeof(char) * b->max_size);
		if(b->content == NULL) {
			b->max_size /= 2;
			b->content = tmp;
			return -1;
		}
	}
	if(b->pos_end == b->max_size) {
		b->pos_end -= b->max_size;
	}
	b->content[b->pos_end] = c;
	++(b->size);
	++(b->pos_end);
	return 0;
}

int buf_pop(struct buf *b, char *c) {
	if(b->size == 0)
		return -1;
	if(b->pos_begin == b->max_size)
		b->pos_begin -= b->max_size;
	++(b->pos_begin);
	--(b->size);
	*c = b->content[b->pos_begin - 1];
	return 0;
}

void buf_clear(struct buf *b) {
	b->max_size = 0;
	b->size = 0;
	free(b->content);
}

void close_and_clear(int **pipes, int **sockets, long int n) {
	for(long int i = 0; i < n; ++i) {
		close(pipes[i][0]);
		close(pipes[i][1]);
		close(sockets[i][0]);
		close(sockets[i][1]);
		free(pipes[i]);
		free(sockets[i]);
	}
	free(pipes);
	free(sockets);
}

void clear(int **pipes, int **sockets, long int n) {
	for(long int i = 0; i < n; ++i) {
		free(pipes[i]);
		free(sockets[i]);
	}
	free(pipes);
	free(sockets);
}

void close_proc(pid_t *pids, int count) {
	for(int i = 0; i < count; ++i) {
		if(pids[i] != -1) {
			kill(pids[i], SIGKILL);
		}
	}
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		fprintf(stderr, "Error: use \"%s\" with 2 arguments: first - name of file, second - count of child processes.\n", argv[0]);
		return 1;
	}
	char *endptr = NULL;
	long int n = strtol(argv[2], &endptr, 10);
	if((n == LONG_MAX || n == LONG_MIN) && errno == ERANGE) {
		perror("strtol");
		return 1;		
	}
	if(argv[2] == endptr) {
		fprintf(stderr, "Error: no digits were found.\n");
		return 1;
	}
	if(n <= 0) {
		fprintf(stderr, "Error: %ld isn't positive number.\n", n);
		return 1;
	}
	int **pipes = (int **)malloc(sizeof(int *) * n);
	if(pipes == NULL) {
		perror("malloc");
		return 1;
	}
	int **sockets = (int **)malloc(sizeof(int *) * n);
	if(sockets == NULL) {
		perror("malloc");
		free(pipes);
		return 1;
	}
	pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * n);
	if(pids == NULL) {
		free(pipes);
		free(sockets);
		return 1;
	}
	for(long int i = 0; i < n; ++i) {
		pids[i] = -1;
	}
	for(long int i = 0; i < n; ++i) {
		pipes[i] = (int *)malloc(sizeof(int) * 2);
		sockets[i] = (int *)malloc(sizeof(int) * 2);
		if(pipes[i] == NULL || sockets[i] == NULL ||
				pipe(pipes[i]) != 0 || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets[i]) != 0) {
			fprintf(stderr, "Error: create pipe or socketpair.\n");
			for(long int j = i; j > 0; --j) {
				close(pipes[j - 1][0]);
				close(pipes[j - 1][1]);
				close(sockets[j - 1][0]);
				close(sockets[j - 1][1]);
				free(pipes[j - 1]);
				free(sockets[j - 1]);
			}
			free(pids);
			free(pipes);
			free(sockets);
			return 1;
		}
		pids[i] = fork();
		if(pids[i] < 0) {
			close_and_clear(pipes, sockets, i + 1);
			close_proc(pids, n);
			free(pids);
			return 1;
		} else if(pids[i] == 0) {
			int pipe_in = pipes[i][0];
			close(pipes[i][1]);
			int soc_out = sockets[i][1];
			close(sockets[i][0]);
			clear(pipes, sockets, i + 1);
			free(pids);
			int rd_st = 0;
			int wr_st = 1;
			char c;
			while((rd_st = read(pipe_in, &c, 1)) > 0) {
				if(i == n - 1 && c != 0) {
					write(STDOUT_FILENO, &c, 1);
				}
				if((wr_st = write(soc_out, &c, 1)) <= 0)
					break;
			}
			if(rd_st < 0 || wr_st <= 0) {
				fprintf(stderr, "Error: can't read or write.\n");
				close(pipe_in);
				close(soc_out);
				return 1;
			}
			close(pipe_in);
			close(soc_out);
			return 0;
		} else {
			close(pipes[i][0]);
			close(sockets[i][1]);
		}
	}
	for(long int i = 0; i < n; ++i) {
		int flags = fcntl(pipes[i][1], F_GETFL, 0);
		if(flags == -1) {
			perror("fcntl");
			close_proc(pids, n);
			close_and_clear(pipes, sockets, n);
			free(pids);
			return 1;
		}
		if(fcntl(pipes[i][1], F_SETFL, flags | O_NONBLOCK) == -1) {
			perror("fcntl");
			close_proc(pids, n);
			close_and_clear(pipes, sockets, n);
			free(pids);
			return 1;
		}
		flags = fcntl(sockets[i][0], F_GETFL, 0);
		if(flags == -1) {
			perror("fcntl");
			close_proc(pids, n);
			close_and_clear(pipes, sockets, n);
			free(pids);
			return 1;
		}
		if(fcntl(sockets[i][0], F_SETFL, flags | O_NONBLOCK) == -1) {
			perror("fcntl");
			close_proc(pids, n);
			close_and_clear(pipes, sockets, n);
			free(pids);
			return 1;
		}
	}
	struct buf *bufs = (struct buf *)malloc(sizeof(struct buf) * n);
	if(bufs == 0) {
		perror("malloc");
		close_proc(pids, n);
		close_and_clear(pipes, sockets, n);
		free(pids);
		return 1;
	}
	for(long int i = 0; i < n; ++i) {
		if(new_buf(&bufs[i]) == -1) {
			fprintf(stderr, "Error: new buf.\n");
			for(long int j = i - 1; j >= 0; --j) {
				buf_clear(&bufs[j]);
			}
			free(bufs);
			close_proc(pids, n);
			close_and_clear(pipes, sockets, n);
			free(pids);
			return 1;
		}
	}
	int fd;
	if((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open");
		for(long int j = 0; j < n; ++j) {
			buf_clear(&bufs[j]);
		}
		free(bufs);
		close_proc(pids, n);
		close_and_clear(pipes, sockets, n);
		free(pids);
		return 1;
	}
	int tmp_rd_st;
	char tmp_c;
	while((tmp_rd_st = read(fd, &tmp_c, 1)) > 0) {
		if(buf_add(&bufs[0], tmp_c) == -1) {
			fprintf(stderr, "Error: buf_add.\n");
			for(long int j = 0; j < n; ++j) {
				buf_clear(&bufs[j]);
			}
			free(bufs);
			close_proc(pids, n);
			close_and_clear(pipes, sockets, n);
			free(pids);
			return 1;
		}
	}
	if(tmp_rd_st < 0 || buf_add(&bufs[0], 0)) {
		fprintf(stderr, "Error: read or buf_add.\n");
		for(long int j = 0; j < n; ++j) {
			buf_clear(&bufs[j]);
		}
		free(bufs);
		close_proc(pids, n);
		close_and_clear(pipes, sockets, n);
		free(pids);
		return 1;
	}
	fd_set rfds;
	fd_set wfds;
	long int count_of_proc = n;
	while(count_of_proc > 0) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		int highest = 0;
		for(long int i = 0; i < n; ++i) {
			FD_SET(pipes[i][1], &wfds);
			if(pipes[i][1] > highest)
				highest = pipes[i][1];
			FD_SET(sockets[i][0], &rfds);
			if(sockets[i][0] > highest)
				highest = sockets[i][0];
		}
		int res = select(highest + 1, &rfds, &wfds, NULL, NULL);
		if(res > 0) {
			for(long int i = 0; i < n; ++i) {
				if(FD_ISSET(sockets[i][0], &rfds)) {
					char c = 0;
					int rd_st = 0;
					if((rd_st = read(sockets[i][0], &c, 1)) < 0) {
						perror("read");
						for(long int i = 0; i < n; ++i) {
							buf_clear(&bufs[i]);
						}
						free(bufs);
						close_proc(pids, n);
						close_and_clear(pipes, sockets, n);
						free(pids);
						return 1;
					}
					if(rd_st == 0)
						continue;
					if(i != n - 1) {
						if(buf_add(&bufs[i + 1], c) == -1) {
							fprintf(stderr, "Error: buf_add.\n");
							for(long int i = 0; i < n; ++i) {
								buf_clear(&bufs[i]);
							}
							free(bufs);
							close_proc(pids, n);
							close_and_clear(pipes, sockets, n);
							free(pids);
							return 1;
						}
					}
					if(c == 0) {
						if(kill(pids[i], SIGKILL) == -1) {
							perror("kill");
							for(long int i = 0; i < n; ++i) {
								buf_clear(&bufs[i]);
							}
							free(bufs);
							close_proc(pids, n);
							close_and_clear(pipes, sockets, n);
							free(pids);
							return 1;

						}
					}
				}
			}
			for(long int i = 0; i < n; ++i) {
				if(FD_ISSET(pipes[i][1], &wfds) && pids[i] != -1) {
					char c = 0;
					if(buf_pop(&bufs[i], &c) == -1) {
						continue;
					}
					int wr_st = 0;
					if((wr_st = write(pipes[i][1], &c, 1)) <= 0) {
						perror("write");
						for(long int i = 0; i < n; ++i) {
							buf_clear(&bufs[i]);
						}
						free(bufs);
						close_proc(pids, n);
						close_and_clear(pipes, sockets, n);
						free(pids);
						return 1;
					}
				}
			}
		}
		for(int j = 0; j < n; ++j) {
			int w_stat;
			pid_t tmp_pid = waitpid(pids[j], &w_stat, WNOHANG);
			if(tmp_pid > 0) {
				if(WIFEXITED(w_stat) && WEXITSTATUS(w_stat))
				{
					fprintf(stderr, "Error: child process closed.\n");
					for(long int i = 0; i < n; ++i) {
						buf_clear(&bufs[i]);
					}
					free(bufs);
					close_proc(pids, n);
					close_and_clear(pipes, sockets, n);
					free(pids);
					return 1;
				}
				for(long int i = 0; i < n; ++i) {
					if(pids[i] == tmp_pid) {
						pids[i] = -1;
						break;
					}
				}
				--count_of_proc;
			}
		}
	}
	close_and_clear(pipes, sockets, n);
	for(long int i = 0; i < n; ++i) {
		buf_clear(&bufs[i]);
	}
	free(bufs);
	free(pids);
	return 0;
}
