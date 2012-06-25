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
#define MY_TMP_FIFO_FILE "/tmp/OS_FIVT_T2_P3/server_tmp_fifo"

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
			rmdir(MY_FIFO_DIR);
			perror("lockf");
			return 1;
		}
		fprintf(stderr, "Error: already run!\n");
		return 1;
	}
	int shm;
	if((shm = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666)) == -1) {
		perror("shm_open");
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		return 1;
	}
	if(ftruncate(shm, 1) == -1) {
		shm_unlink(SHM_NAME);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		perror("ftruncate");
		return 1;
	}
	char *point = (char *)mmap(NULL, 1, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if(*(int *)point == -1) {
		shm_unlink(SHM_NAME);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		perror("mmap");
		return 1;
	}
	sem_t *sem_ok;
	if((sem_ok = sem_open(SEM_OK_NAME, O_CREAT, 0666, 0)) == SEM_FAILED) {
		shm_unlink(SHM_NAME);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		perror("sem_open(SEM_OK_NAME)");
		return 1;
	}
	while(sem_trywait(sem_ok) == 0) {}
	sem_t *sem_serv;
	if((sem_serv = sem_open(SEM_SERV_NAME, O_CREAT, 0666, 0)) == SEM_FAILED) {
		shm_unlink(SHM_NAME);
		sem_unlink(SEM_OK_NAME);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		perror("sem_open(SEM_SERV_NAME)");
		return 1;
	}
	while(sem_trywait(sem_serv) == 0) {}
	sem_t *sem_cl;
	if((sem_cl = sem_open(SEM_CL_NAME, O_CREAT, 0666, 0)) == SEM_FAILED) {
		shm_unlink(SHM_NAME);
		sem_unlink(SEM_OK_NAME);
		sem_unlink(SEM_SERV_NAME);
		close(fifo_tmp);
		unlink(MY_TMP_FIFO_FILE);
		rmdir(MY_FIFO_DIR);
		perror("sem_open(SEM_CL_NAME)");
		return 1;
	}
	while(sem_trywait(sem_cl) == 0) {}
	sem_post(sem_cl);
	while(sem_wait(sem_serv) == 0) {
		if(sem_trywait(sem_ok) == -1) {
			if(errno != EAGAIN) {
				sem_unlink(SEM_SERV_NAME);
				sem_unlink(SEM_OK_NAME);
				sem_unlink(SEM_CL_NAME);
				shm_unlink(SHM_NAME);
				close(fifo_tmp);
				unlink(MY_TMP_FIFO_FILE);
				rmdir(MY_FIFO_DIR);
				perror("sem_trywait(sem_ok)");
				return 1;
			}
		} else {
			sem_unlink(SEM_CL_NAME);
			sem_unlink(SEM_SERV_NAME);
			sem_unlink(SEM_OK_NAME);
			shm_unlink(SHM_NAME);
			close(fifo_tmp);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
			return 0;
		}
		printf("%c", *point);
		if(sem_post(sem_cl) != 0) {
			perror("sem_post");
			sem_unlink(SEM_CL_NAME);
			sem_unlink(SEM_SERV_NAME);
			sem_unlink(SEM_OK_NAME);
			shm_unlink(SHM_NAME);
			close(fifo_tmp);
			unlink(MY_TMP_FIFO_FILE);
			rmdir(MY_FIFO_DIR);
			return 1;
		}
	}
	sem_unlink(SEM_CL_NAME);
	sem_unlink(SEM_SERV_NAME);
	sem_unlink(SEM_OK_NAME);
	shm_unlink(SHM_NAME);
	close(fifo_tmp);
	unlink(MY_TMP_FIFO_FILE);
	rmdir(MY_FIFO_DIR);
	return 1;
}
