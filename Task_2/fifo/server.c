#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define MY_FIFO_DIR "/tmp/OS_FIVT_T2_P1/"
#define MY_FIFO_FILE "/tmp/OS_FIVT_T2_P1/fifo_file"
#define MY_TMP_FIFO_FILE "/tmp/OS_FIVT_T2_P1/fifo_serv_tmp_file"

int main() {
	if(mkdir(MY_FIFO_DIR, 0777) != 0) {
		if(errno != EEXIST) {
			perror("mkdir");
			return 1;
		}
	}
	if(mkfifo(MY_TMP_FIFO_FILE, 0777) != 0) {
		if(errno != EEXIST) {
			rmdir(MY_FIFO_DIR);
			perror("MY_TMP_FIFO_FILE mkfifo");
			return 1;
		}
	}
	int fifo_tmp;
	if((fifo_tmp = open(MY_TMP_FIFO_FILE, O_RDWR)) == -1) {
		rmdir(MY_FIFO_DIR);
		perror("MY_TMP_FIFO_FILE open");
		return 1;
	}
	if(lockf(fifo_tmp, F_TLOCK, (off_t)0) != 0) {
		if(errno != EAGAIN) {
			close(fifo_tmp);
			rmdir(MY_FIFO_DIR);
			perror("lockf");
			return 1;
		}
		fprintf(stderr, "Error: already run!\n");
		return 1;
	}
	if(mkfifo(MY_FIFO_FILE, 0777) != 0) {
		if(errno != EEXIST) {
			close(fifo_tmp);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
			perror("MY_FIFO_FILE mkfifo");
			return 1;
		}
	}
	int fifo_fd;
	if((fifo_fd = open(MY_FIFO_FILE, O_RDONLY)) == -1) {
		close(fifo_tmp);
		unlink(MY_FIFO_FILE);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		perror("MY_FIFO_FILE open");
		return 1;
	}
	char c;
	while(1) { 
		int read_st = 0;
		if((read_st = read(fifo_fd, &c, 1)) == 0) {
			close(fifo_fd);
			close(fifo_tmp);
			unlink(MY_FIFO_FILE);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
			return 0;
		} else if(read_st < 0) {
			close(fifo_fd);
			close(fifo_tmp);
			unlink(MY_FIFO_FILE);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
		}
		printf("%c", c);
	}
	return 0;
}
