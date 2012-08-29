#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
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

#define MAX_USERS 100
#define MAX_NAME 20
#define MAX_MESSAGE 512

struct user {
	int sockfd;
	char name[MAX_NAME + 1];
	char message[MAX_MESSAGE];
	size_t size_mes;
};

void add_char(struct user *u, char c) {
	if((c > 31 && c < 127 && u->size_mes < MAX_MESSAGE - 1) || c == '\n')
		u->message[(u->size_mes)++] = c;
}

int main() {
	struct user users[MAX_USERS];
	struct sockaddr_in servaddr;
	int sockfd;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(6666);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd, MAX_USERS) < 0) {
		perror("listen");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	for(size_t i = 0; i < MAX_USERS; ++i) {
		users[i].sockfd = -1;
		users[i].size_mes = 0;
	}
	fd_set rfds;
	while(1) {
		FD_ZERO(&rfds);
		int highest = sockfd;
		FD_SET(sockfd, &rfds);
		for(size_t i = 0; i < MAX_USERS; ++i) {
			if(users[i].sockfd != -1) {
				if(highest < users[i].sockfd)
					highest = users[i].sockfd;
				FD_SET(users[i].sockfd, &rfds);
			}
		}
		int res = select(highest + 1, &rfds, NULL, NULL, NULL);
		if(res > 0) {
			if(FD_ISSET(sockfd, &rfds)) {
				char connected = 0;
				for(size_t i = 0; i < MAX_USERS; ++i) {
					if(users[i].sockfd == -1) {
						if((users[i].sockfd = accept(sockfd, NULL, NULL)) < 0) {
							users[i].sockfd = -1;
							connected = 2;
							break;
						}
						strncpy(users[i].name, "Guest", MAX_NAME +1);
						int flags = fcntl(users[i].sockfd, F_GETFL, 0);
						if(flags == -1) {
							perror("fcntl");
							close(users[i].sockfd);
							users[i].sockfd = -1;
							connected = 2;
							break;
						}
						if(fcntl(users[i].sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
							perror("fcntl");
							close(users[i].sockfd);
							users[i].sockfd = -1;
							connected = 2;
							break;
						}
						connected = 1;
						break;
					}
				}
				if(connected == 0) {
					int newsockfd;
					if((newsockfd = accept(sockfd, NULL, NULL)) < 0) {
						continue;
					}
					close(newsockfd);
				}
				if(connected == 1)
					printf("Connected new user. \n");
				continue;
			}
			for(size_t i = 0; i < MAX_USERS; ++i) {
				if(users[i].sockfd != -1 && FD_ISSET(users[i].sockfd, &rfds)) {
					char c = 0;
					if(c == 0) {
						char is_read_smth = 0;
						while(read(users[i].sockfd, &c, sizeof(char)) > 0) {
							is_read_smth = 1;
							add_char(&users[i], c);
							if(c == '\n') {
								if(strstr(users[i].message, "/nick") == users[i].message) {
									char *tmp = NULL;
									if((tmp = strstr(users[i].message, " ")) == NULL) {
										users[i].size_mes = 0;
										continue;
									}
									++tmp;
									for(size_t j = 0; j < MAX_NAME - 1 && tmp[j] != '\n'; ++j) {
										users[i].name[j] = tmp[j];
										users[i].name[j + 1] = 0;
									}
									users[i].size_mes = 0;
								} else if(strstr(users[i].message, "/quit") == users[i].message) {
									printf("User disconnected.\n");
									close(users[i].sockfd);
									users[i].sockfd = -1;
									users[i].size_mes = 0;
								} else if(strstr(users[i].message, "/sendmsg") == users[i].message) {
									char *nick = NULL;
									if((nick = strstr(users[i].message, " ")) == NULL) {
										users[i].size_mes = 0;
										continue;
									}
									++nick;
									char *mess = NULL;
									if((mess = strstr(nick, " ")) == NULL) {
										users[i].size_mes = 0;
										continue;
									}
									char nickname[MAX_NAME + 1];
									for(size_t i = 0; i < MAX_NAME && nick != mess; ++i) {
										nickname[i] = *nick;
										nickname[i + 1] = 0;
										++nick;
									}
									++mess;
									for(size_t k = 0; k < MAX_USERS; ++k) {
										if(users[k].sockfd != -1 && strcmp(users[k].name, nickname) == 0) {
											char tmp_c;
											if(write(users[k].sockfd, users[i].name, strlen(users[i].name)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
											tmp_c = ':';
											if(write(users[k].sockfd, &tmp_c, sizeof(char)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
											tmp_c = ' ';
											if(write(users[k].sockfd, &tmp_c, sizeof(char)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
											if(write(users[k].sockfd, mess, users[i].size_mes - (size_t)(mess - users[i].message)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
										}
									}
									users[i].size_mes = 0;
								} else {
									for(size_t k = 0; k < MAX_USERS; ++k) {
										if(users[k].sockfd != -1 && users[k].sockfd != users[i].sockfd) {
											char tmp_c;
											if(write(users[k].sockfd, users[i].name, strlen(users[i].name)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
											tmp_c = ':';
											if(write(users[k].sockfd, &tmp_c, sizeof(char)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
											tmp_c = ' ';
											if(write(users[k].sockfd, &tmp_c, sizeof(char)) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
											if(write(users[k].sockfd, users[i].message, users[i].size_mes) <= 0) {
												close(users[k].sockfd);
												users[k].sockfd = -1;
												users[k].size_mes = 0;
												printf("User disconnected.\n");
											}
											fsync(users[k].sockfd);
										}
									}
									users[i].size_mes = 0;
								}
							}
						}
						if(is_read_smth == 0) {
							close(users[i].sockfd);
							users[i].sockfd = -1;
							users[i].size_mes = 0;
							printf("User disconnected.\n");
						}
					}
				}
			}
		}
	}
	close(sockfd);
}