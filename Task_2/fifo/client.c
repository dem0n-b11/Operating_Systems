#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define MY_FIFO_FILE "/tmp/OS_FIVT_T2_P1/fifo_file"
#define MY_TMP_FIFO_FILE "/tmp/OS_FIVT_T2_P1/fifo_client_tmp_file"

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Error: Use \"%s\" with one argument.\n", argv[0]);
		return 1;
	}
	if(mkfifo(MY_TMP_FIFO_FILE, 0777) != 0) {
		if(errno != EEXIST) {
			perror("MY_TMP_FIFO_FILE mkfifo");
			return 1;
		}
	}
	int fifo_tmp;
	if((fifo_tmp = open(MY_TMP_FIFO_FILE, O_RDWR)) == -1) {
		perror("MY_TMP_FIFO_FILE open");
		return 1;
	}
	if(lockf(fifo_tmp, F_TLOCK, (off_t)0) != 0) {
		if(errno != EAGAIN) {
			perror("lockf");
			return 1;
		}
		fprintf(stderr, "Error: already run!\n");
		return 1;
	}
	int fd;
	if((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("FILE open");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		return 1;
	}
	int fifo_fd;
	if((fifo_fd = open(MY_FIFO_FILE, O_WRONLY | O_NONBLOCK)) == -1) {
		perror("MY_FIFO_FILE open");
		close(fd);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		return 1;
	}
	char c = 0;
	int read_st = 0;
	int write_st = 0;
	while((read_st = read(fd, &c, 1)) > 0) {
		if((write_st = write(fifo_fd, &c, 1)) <= 0)
			break;
	}
	if(read_st < 0) {
		perror("read");
		close(fd);
		close(fifo_tmp);
		close(fifo_fd);
		unlink(MY_TMP_FIFO_FILE);
		return 1;
	}
	if(write_st < 0) {
		perror("write");
		close(fd);
		close(fifo_tmp);
		close(fifo_fd);
		unlink(MY_TMP_FIFO_FILE);
		return 1;
	}
	close(fifo_fd);
	close(fd);
	close(fifo_tmp);
	unlink(MY_TMP_FIFO_FILE);
	return 0;
}
