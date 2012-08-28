#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

void clear_matrix(int **mat, size_t n) {
	for(size_t i = 0; i < n; ++i) {
		free(mat[i]);
	}
	free(mat);
}

int main() {
	printf("Enter the number of calculators: ");
	size_t calcs;
	if(scanf("%lu", &calcs) < 1) {
		perror("scanf");
		exit(EXIT_FAILURE);
	}
	printf("Enter n and m: ");
	size_t n, m;
	if(scanf("%lu %lu", &n, &m) < 2) {
		perror("scanf");
		exit(EXIT_FAILURE);
	}
	printf("Enter the first matrix:\n");
	int **first = (int **)malloc(n * sizeof(int *));
	if(first == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for(size_t i = 0; i < n; ++i) {
		first[i] = (int *)malloc(m * sizeof(int));
		if(first[i] == NULL) {
			perror("malloc");
			for(size_t j = i; j > 0; --j) {
				free(first[j - 1]);
			}
			free(first);
			exit(EXIT_FAILURE);
		}
		for(size_t j = 0; j < m; ++j) {
			scanf("%d", &first[i][j]);
		}
	}
	printf("Enter the second matrix:\n");
	int **second = (int **)malloc(m * sizeof(int *));
	if(second == NULL) {
		perror("malloc");
		clear_matrix(first, n);
		exit(EXIT_FAILURE);
	}
	for(size_t i = 0; i < m; ++i) {
		second[i] = (int *)malloc(n * sizeof(int));
		if(second[i] == NULL) {
			perror("malloc");
			clear_matrix(first, n);
			for(size_t j = i; j > 0; --j) {
				free(second[j - 1]);
			}
			free(second);
			exit(EXIT_FAILURE);
		}
		for(size_t j = 0; j < n; ++j) {
			scanf("%d", &second[i][j]);
		}
	}
	int *socks = (int *)malloc(calcs * sizeof(int));
	if(socks == NULL) {
		perror("malloc");
		clear_matrix(first, n);
		clear_matrix(second, m);
		exit(EXIT_FAILURE);
	}
	FILE *fd = fopen("workers.txt", "r");
	if(fd < 0) {
		perror("fopen");
		clear_matrix(first, n);
		clear_matrix(second, m);
		free(socks);
		exit(EXIT_FAILURE);
	}
	size_t element_i = 0;
	size_t element_j = 0;
	for(size_t i = 0; i < calcs && (element_i != n); ++i) {
		char ipaddr[20];
		char port_str[10];
		if(fscanf(fd, "%s %s", ipaddr, port_str) < 2) {
			perror("scanf");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		char *endptr = NULL;
		long int port = strtol(port_str, &endptr, 10);
		if((port == LONG_MAX || port == LONG_MIN) && errno == ERANGE) {
			perror("strtol");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		if(port_str == endptr) {
			fprintf(stderr, "Error: no digits were found.\n");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		if(port <= 0 || port > 65535) {
			fprintf(stderr, "Error: %ld isn't a number of port.\n", port);
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		struct sockaddr_in servaddr;
		if((socks[i] = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(port);
		if(inet_aton(ipaddr, &servaddr.sin_addr) == 0) {
			fprintf(stderr, "Invalid IP.\n");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i + 1; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		if(connect(socks[i], (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
			perror("connect");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i + 1; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		if(write(socks[i], &m, sizeof(size_t)) <= 0 || write(socks[i], &element_i, sizeof(size_t)) <= 0 || write(socks[i], &element_j, sizeof(size_t)) <= 0) {
			perror("write");
			clear_matrix(first, n);
			clear_matrix(second, m);
			for(size_t j = i + 1; j > 0; --j) {
				close(socks[j - 1]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
		for(size_t j = 0; j < m; ++j) {
			if(write(socks[i], &first[element_i][j], sizeof(int)) <= 0 || write(socks[i], &second[j][element_j], sizeof(int)) <= 0) {
				perror("write");
				clear_matrix(first, n);
				clear_matrix(second, m);
				for(size_t j = i + 1; j > 0; --j) {
					close(socks[j - 1]);
				}
				free(socks);
				exit(EXIT_FAILURE);
			}
		}
		++element_j;
		if(element_j == n) {
			++element_i;
			element_j = 0;
		}
	}
	int **ans = (int **)malloc(n * sizeof(int *));
	if(ans == NULL) {
		perror("malloc");
		clear_matrix(first, n);
		clear_matrix(second, m);
		for(size_t j = 0 ; j < calcs; ++j) {
			close(socks[j]);
		}
		free(socks);
		exit(EXIT_FAILURE);
	}
	for(size_t i = 0; i < n; ++i) {
		ans[i] = (int *)malloc(n * sizeof(int));
		if(ans[i] == NULL) {
			perror("malloc");
			clear_matrix(first, n);
			clear_matrix(second, m);
			clear_matrix(ans, i - 1);
			for(size_t j = 0; j < calcs; ++j) {
				close(socks[j]);
			}
			free(socks);
			exit(EXIT_FAILURE);
		}
	}
	fd_set rfds;
	size_t count_ans = n * n;
	while(count_ans > 0) {
		FD_ZERO(&rfds);
		int highest = 0;
		for(size_t i = 0; i < calcs; ++i) {
			if(socks[i] > highest)
				highest = socks[i];
			FD_SET(socks[i], &rfds);
		}
		int res = select(highest + 1, &rfds, NULL, NULL, NULL);
		if(res > 0) {
			for(size_t i = 0; i < calcs; ++i) {
				if(FD_ISSET(socks[i], &rfds)) {
					size_t el_i;
					size_t el_j;
					int result;
					if(read(socks[i], &el_i, sizeof(size_t)) <= 0 || read(socks[i], &el_j, sizeof(size_t)) <= 0 || read(socks[i], &result, sizeof(int)) <= 0) {
						perror("read");
						clear_matrix(first, n);
						clear_matrix(second, m);
						clear_matrix(ans, n);
						for(size_t j = 0; j < calcs; ++j) {
							close(socks[j]);
						}
						free(socks);
						exit(EXIT_FAILURE);
					}
					ans[el_i][el_j] = result;
					--count_ans;
					if(element_i == n)
						continue;
					if(write(socks[i], &m, sizeof(size_t)) <= 0 || write(socks[i], &element_i, sizeof(size_t)) <= 0 || write(socks[i], &element_j, sizeof(size_t)) <= 0) {
						perror("write");
						clear_matrix(first, n);
						clear_matrix(second, m);
						clear_matrix(ans, n);
						for(size_t j = 0; j < calcs; ++j) {
							close(socks[j]);
						}
						free(socks);
						exit(EXIT_FAILURE);
					}
					for(size_t j = 0; j < m; ++j) {
						if(write(socks[i], &first[element_i][j], sizeof(int)) <= 0 || write(socks[i], &second[j][element_j], sizeof(int)) <= 0) {
							perror("write");
							clear_matrix(first, n);
							clear_matrix(second, m);
							for(size_t j = 0; j < calcs; ++j) {
								close(socks[j]);
							}
							clear_matrix(ans, n);
							free(socks);
							exit(EXIT_FAILURE);
						}
					}
					++element_j;
					if(element_j == n) {
						++element_i;
						element_j = 0;
					}
				}
			}
		}
	}
	printf("Result:\n");
	for(size_t i = 0; i < n; ++i) {
		for(size_t j = 0; j < n; ++j) {
			printf("%d\t", ans[i][j]);
		}
		printf("\n");
	}
	clear_matrix(first, n);
	clear_matrix(second, m);
	clear_matrix(ans, n);
	size_t fake_m = 0;
	for(size_t j = 0; j < calcs; ++j) {
		write(socks[j], &fake_m, sizeof(size_t));
		close(socks[j]);
	}
	free(socks);
	exit(EXIT_SUCCESS);
}
