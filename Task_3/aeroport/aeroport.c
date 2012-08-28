#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#define SEM_NAME "/sem_name%lu"

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Use this program with the argument 'port'.\n");
		exit(EXIT_FAILURE);
	}
	char *endptr = NULL;
	long int port = strtol(argv[1], &endptr, 10);
	if((port == LONG_MAX || port == LONG_MIN) && errno == ERANGE) {
		perror("strtol");
		exit(EXIT_FAILURE);
	}
	if(argv[1] == endptr) {
		fprintf(stderr, "Error: no digits were found.\n");
		exit(EXIT_FAILURE);
	}
	if(port <= 0 || port > 65535) {
		fprintf(stderr, "Error: %ld isn't a number of port.\n", port);
		exit(EXIT_FAILURE);
	}
	printf("Enter the count of landing planes: ");
	size_t n;
	if(scanf("%lu", &n) < 1) {
		perror("scanf");
		exit(EXIT_FAILURE);
	}
	printf("Enter the number of departing planes: ");
	size_t m;
	if(scanf("%lu", &m) < 1) {
		perror("scanf");
		exit(EXIT_FAILURE);
	}
	printf("Enter the number of strips: ");
	size_t k;
	if(scanf("%lu", &k) < 1) {
		perror("scanf");
		exit(EXIT_FAILURE);
	}
	sem_t **sems;
	sems = (sem_t **)malloc(sizeof(sem_t *) * k);
	if(sems == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for(size_t i = 0; i < k; ++i) {
		char tmp[25];
		if(snprintf(tmp, 25, SEM_NAME, i) <= 0) {
			perror("snprintf");
			free(sems);
			exit(EXIT_FAILURE);
		}
		if((sems[i] = sem_open(tmp, O_CREAT, 0666, 0)) == SEM_FAILED) {
			perror("sem_open");
			free(sems);
			exit(EXIT_FAILURE);
		}
		while(sem_trywait(sems[i]) == 0) {}
		if(errno != EAGAIN) {
			perror("sem_trywait");
			for(size_t j = i; j > 0; --j) {
				snprintf(tmp, 25, SEM_NAME, j);
				sem_unlink(tmp);
			}
			free(sems);
			exit(EXIT_FAILURE);
		}
		if(sem_post(sems[i]) != 0) {
			perror("sem_open");
			for(size_t j = i; j > 0; --j) {
				snprintf(tmp, 25, SEM_NAME, j);
				sem_unlink(tmp);
			}
			free(sems);
			exit(EXIT_FAILURE);
		}
	}
	struct sockaddr_in servaddr;
	int sockfd, newsockfd;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		for(size_t j = 0; j < k; ++j) {
			char tmp[25];
			snprintf(tmp, 25, SEM_NAME, j);
			sem_unlink(tmp);
		}
		free(sems);
		exit(EXIT_FAILURE);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		for(size_t j = 0; j < k; ++j) {
			char tmp[25];
			snprintf(tmp, 25, SEM_NAME, j);
			sem_unlink(tmp);
		}
		free(sems);
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd, n + m + 1) < 0) {
		perror("listen");
		for(size_t j = 0; j < k; ++j) {
			char tmp[25];
			snprintf(tmp, 25, SEM_NAME, j);
			sem_unlink(tmp);
		}
		free(sems);
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	size_t num_of_next_strip = 0;
	while(1) {
		if((newsockfd = accept(sockfd, NULL, NULL)) < 0) {
			perror("accept");
			for(size_t j = 0; j < k; ++j) {
				char tmp[25];
				snprintf(tmp, 25, SEM_NAME, j);
				sem_unlink(tmp);
			}
			free(sems);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		char status;
		if(read(newsockfd, &status, 1) <= 0) {
			perror("read");
			for(size_t j = 0; j < k; ++j) {
				char tmp[25];
				snprintf(tmp, 25, SEM_NAME, j);
				sem_unlink(tmp);
			}
			free(sems);
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		if(status == 0) {
			printf("We received a request for landing.\n");
		} else {
			printf("We received a request for departure.\n");
		}
		int pid = fork();
		if(pid < 0) {
			perror("fork");
			for(size_t j = 0; j < k; ++j) {
				char tmp[25];
				snprintf(tmp, 25, SEM_NAME, j);
				sem_unlink(tmp);
			}
			free(sems);
			close(sockfd);
			exit(EXIT_FAILURE);
		} else if(pid == 0) {
			sem_t *my_sem = sems[num_of_next_strip];
			free(sems);
			close(sockfd);
			char c = 0;
			if(write(newsockfd, &c, 1) <= 0) {
				perror("write");
				exit(EXIT_FAILURE);
			}
			if(sem_wait(my_sem) != 0) {
				perror("sem_wait");
				exit(EXIT_FAILURE);
			}
			printf("Stip number %lu is busy.\n", num_of_next_strip + 1);
			++num_of_next_strip;
			if(write(newsockfd, &num_of_next_strip, sizeof(size_t)) <= 0) {
				close(newsockfd);
				fprintf(stderr, "We can not establish a connection.\n");
				exit(EXIT_FAILURE);
			}
			--num_of_next_strip;
			char ansv;
			if(read(newsockfd, &ansv, 1) <= 0) {
				close(newsockfd);
				fprintf(stderr, "We can not establish a connection.\n");
				exit(EXIT_FAILURE);
			}
			if(ansv == 0)
				printf("Strip number %lu is free.\n", num_of_next_strip + 1);
			close(newsockfd);
			if(sem_post(my_sem) != 0) {
				close(newsockfd);
				fprintf(stderr, "The accident on the strip %lu.\n", num_of_next_strip + 1);
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS);
		}
		++num_of_next_strip;
		if(num_of_next_strip == k)
			num_of_next_strip = 0;
		close(newsockfd);
	}
}
