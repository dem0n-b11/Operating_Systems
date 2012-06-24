#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define TIME_ENDL 24

struct string {
	char *str;
	size_t size;
};

int new_str(struct string *s) {
	s->size = 1;
	s->str = (char *)malloc(sizeof(char) * s->size);
	if(s->str == NULL)
		return 1;
	s->str[s->size - 1] = 0;
	return 0;
}

int str_add(struct string *to, struct string *from) {
	size_t end = to->size;
	to->size = to->size + from->size - 1;
	char *tmp  = (char *)realloc(to->str, sizeof(char) * to->size);
	if(tmp == NULL)
		return 1;
	to->str = tmp;
	for(size_t i = end-1; i < to->size; ++i) {
		to->str[i] = from->str[i - end + 1];
	}
	to->str[to->size - 1] = 0;
	return 0;
}

int make_str(struct string *to, char *from, int dir) {
	size_t i = 0;
	while(from[i] != 0)
		++i;
	to->size = i + 1 + dir;
	to->str = (char *)malloc(sizeof(char) * to->size);
	if(to->str == NULL) {
		return 1;
	}
	i = 0;
	while(from[i] != 0) {
		to->str[i] = from[i];
		++i;
	}
	if(dir)
		to->str[to->size - 2] = '/';
	to->str[to->size - 1] = 0;
	return 0;
}

void clear(struct string *s) {
	free(s->str);
}

struct dirs {
	char **dir;
	size_t size;
	size_t max_size;
};

int new_dirs(struct dirs *d) {
	d->size = 0;
	d->max_size = 8;
	d->dir = (char **)malloc(sizeof(char *) * d->max_size);
	if(d->dir == NULL) {
		return 1;
	}
	return 0;
}

int add_dir(struct dirs *d, char *dir) {
	if(d->size == d->max_size) {
		d->max_size *= 2;
		char **tmp = (char **)realloc(d->dir, sizeof(char *) * d->max_size);
		if(tmp == NULL)
			return 1;
		d->dir = tmp;
	}
	++(d->size);
	d->dir[d->size-1] = dir;
	return 0;
}

void clear_dirs(struct dirs *d) {
	for(size_t i = 0; i < d->size; ++i)
		free(d->dir[i]);
	free(d->dir);
}

int mode(char *s_mode, unsigned int md, size_t size) {
	if(size < 11)
		return 1;
	s_mode[0] = '-';
	if(S_IRUSR & md) {
		s_mode[1] = 'r';
	} else {
		s_mode[1] = '-';
	}
	if(S_IWUSR & md) {
		s_mode[2] = 'w';
	} else {
		s_mode[2] = '-';
	}
	if(S_IXUSR & md) {
		s_mode[3] = 'x';
	} else {
		s_mode[3] = '-';
	}
	if(S_IRGRP & md) {
		s_mode[4] = 'r';
	} else {
		s_mode[4] = '-';
	}
	if(S_IWGRP & md) {
		s_mode[5] = 'w';
	} else {
		s_mode[5] = '-';
	}
	if(S_IXGRP & md) {
		s_mode[6] = 'x';
	} else {
		s_mode[6] = '-';
	}
	if(S_IROTH & md) {
		s_mode[7] = 'r';
	} else {
		s_mode[7] = '-';
	}
	if(S_IWOTH & md) {
		s_mode[8] = 'w';
	} else {
		s_mode[8] = '-';
	}
	if(S_IXOTH & md) {
		s_mode[9] = 'x';
	} else {
		s_mode[9] = '-';
	}
	s_mode[10] = 0;
	return 0;
}

void ls(char *s_dir) {
	struct stat main_info;
	if(lstat(s_dir, &main_info) == -1) {
		fprintf(stderr, "File not found.\n");
		return;
	}
	if(S_IFDIR & main_info.st_mode) {
		DIR *dir;
		dir = opendir(s_dir);
		if(dir == NULL) {
			fprintf(stderr, "Error with opening directory %s.\n", s_dir);
			return;
		}
		struct stat info;
		struct dirent *file;
		struct dirs drs;
		if(new_dirs(&drs) != 0) {
			fprintf(stderr, "Memory error.\n");
			return;
		}
		printf("directory --- %s\n", s_dir);
		while((file = readdir(dir)) != NULL) {
			if((strcmp(file->d_name, "..")) && 
					(strcmp(file->d_name, "."))) {
				struct string path;
				struct string f_name;
				char s_mode[11];
				if((make_str(&path, s_dir, 1)) || (make_str(&f_name, file->d_name, 0)) || (str_add(&path, &f_name)) ||
						(lstat(path.str, &info) != 0) || (mode(s_mode, info.st_mode, 11) != 0)) {
					clear(&path);
					clear(&f_name);
					continue;
				}
				char *tm = ctime(&info.st_mtime);
				*(tm + TIME_ENDL) = 0;
				if(S_IFDIR & info.st_mode) {
					if(add_dir(&drs, path.str)!=0) {
						clear(&path);
						clear(&f_name);
						continue;
					}
					clear(&f_name);
				} else {
					clear(&path);
					clear(&f_name);
				}
				printf("%s %u %u %s %s\n", s_mode, info.st_uid,
						info.st_gid, tm, file->d_name);
			}
		}
		printf("\n");
		for(size_t i = 0; i < drs.size; ++i)
			ls(drs.dir[i]);
		closedir(dir);
		clear_dirs(&drs);
	} else {
		char s_mode[11];
		if(mode(s_mode, main_info.st_mode, 11) != 0) {
			exit(1);
		}
		char *tm = ctime(&main_info.st_mtime);
		*(tm + 24) = 0;
		printf("file --- %s\n", s_dir);
		printf("%s %u %u %s\n", s_mode, main_info.st_uid,
				main_info.st_gid, tm);
	}
}

int main(int argc, char *argv[]) {
	if(argc == 1) {
		ls(".");
	} else {
		ls(argv[1]);
	}
	return 0;
}
