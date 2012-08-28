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
	struct sockaddr_in servaddr;
	int sockfd;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd, 5) < 0) {
		perror("listen");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	while(1) {
		int newsockfd;
		if((newsockfd = accept(sockfd, NULL, NULL)) < 0) {
			perror("accept");
			close(sockfd);
			exit(EXIT_FAILURE);
		}
		while(1) {
			size_t el_i, el_j, n;
			if(read(newsockfd, &n, sizeof(size_t)) <= 0) {
				perror("read");
				close(newsockfd);
				close(sockfd);
				exit(EXIT_FAILURE);
			}
			if(n == 0)
				break;
			if(read(newsockfd, &el_i, sizeof(size_t)) <= 0 || read(newsockfd, &el_j, sizeof(size_t)) <= 0) {
				perror("read");
				close(newsockfd);
				close(sockfd);
				exit(EXIT_FAILURE);
			}
			int res = 0;
			for(size_t i = 0; i < n; ++i) {
				int e1, e2;
				if(read(newsockfd, &e1, sizeof(int)) <= 0 || read(newsockfd, &e2, sizeof(int)) <= 0) {
					perror("read");
					close(newsockfd);
					close(sockfd);
					exit(EXIT_FAILURE);
				}
				res += e1 * e2;
			}
			if(write(newsockfd, &el_i, sizeof(size_t)) <= 0 || write(newsockfd, &el_j, sizeof(size_t)) <= 0 || write(newsockfd, &res, sizeof(int)) <= 0) {
				perror("write");
				close(newsockfd);
				close(sockfd);
				exit(EXIT_FAILURE);
			}
		}
		close(newsockfd);
	}
}
