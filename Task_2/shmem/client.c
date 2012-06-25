#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>

#define SHM_NAME "/my_sh_mem"
#define SEM_OK_NAME "/my_sem"
#define SEM_SERV_NAME "/serv_sem"
#define SEM_CL_NAME "/cl_sem"
#define MY_FIFO_DIR "/tmp/OS_FIVT_T2_P3/"
#define MY_TMP_FIFO_FILE "/tmp/OS_FIVT_T2_P3/client_tmp_fifo"

int main(int argc, char *argv[]) {
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
			rmdir(MY_FIFO_DIR);
			perror("lockf");
			return 1;
		}
		fprintf(stderr, "Error: already run!\n");
		return 1;
	}
	if(argc != 2) {
		fprintf(stderr, "Error: use '%s' with one argument\n", argv[0]);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	int shm;
	if((shm = shm_open(SHM_NAME, O_RDWR, 0777)) == -1) {
		perror("shm_open");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	char *pointer = (char *)mmap(NULL, 1, PROT_WRITE, MAP_SHARED, shm, 0);
	if(*(int *)pointer == -1) {
		perror("mmap");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	sem_t *sem_serv;
	if((sem_serv = sem_open(SEM_SERV_NAME, 0)) == SEM_FAILED) {
		perror("sem_open(SEM_SERV_NAME)");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	sem_t *sem_cl;
	if((sem_cl = sem_open(SEM_CL_NAME, 0)) == SEM_FAILED) {		
		perror("sem_open(SEM_CL_NAME)");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	sem_t *sem_ok;
	if((sem_ok = sem_open(SEM_OK_NAME, 0)) == SEM_FAILED) {
		perror("sem_open(SEM_OK_NAME)");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	int fd;
	if((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	char c;
	int read_st = 0;
	while((read_st = read(fd, &c, 1)) > 0) {
		if(sem_wait(sem_cl) != 0) {
			perror("sem_wait(sem_cl)");
			close(fifo_tmp);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
			return 1;
		}
		*pointer = c;
		if(sem_post(sem_serv) != 0) {
			perror("sem_post(sem_serv)");
			close(fifo_tmp);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
			return 1;
		}
	}
	if(read_st != 0) {
		perror("read");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	sem_post(sem_serv);
	sem_post(sem_ok);
	close(fifo_tmp);
	unlink(MY_TMP_FIFO_FILE);
	rmdir(MY_FIFO_DIR);
	return 0;
}
