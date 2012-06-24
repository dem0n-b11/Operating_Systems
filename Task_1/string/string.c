#include <stdio.h>
#include <stdlib.h>

struct dictionary {
	size_t count;
	size_t max_size;
	char **words;
};

struct word {
	size_t size;
	size_t max_size;
	char *lexeme;
};

void clear(struct dictionary *dict, struct word *w) {
	if(dict != NULL) {
		for(size_t i = 0; i < dict->count; ++i)
			free(dict->words[i]);
		free(dict->words);
	}
	if(w != NULL) {
		free(w->lexeme);
	}
}

int add_word(struct dictionary *dict, struct word *w) {
	if((dict == NULL) || (w == NULL)) {
		return 1;
	}
	++(dict->count);
	if(dict->count > dict->max_size) {
		dict->max_size *= 2;
		char **tmp = (char **)realloc(dict->words, dict->max_size * sizeof(char *));
		if(tmp == NULL) {
			return 1;
		}
		dict->words = tmp;
	}
	dict->words[dict->count - 1] = w->lexeme;
	return 0;
}

int new_word(struct word *w) {
	w->size = 1;
	w->lexeme = (char *)malloc(sizeof(char));
	w->max_size = 1;
	if(w->lexeme == NULL) {
		return 1;
	}
	w->lexeme[0] = 1;
	return 0;
}

int add_char(struct word *w, char c) {
	if(w == NULL) {
		return 1;
	}
	++(w->size);
	if(w->size >= w->max_size) {
		w->max_size *= 2;
		char *tmp = (char *)realloc(w->lexeme, w->max_size * sizeof(char));
		if(tmp == NULL) {
			return 1;
		}
		w->lexeme = tmp;
	}
	w->lexeme[w->size - 2] = c;
	w->lexeme[w->size - 1] = 0;
	return 0;
}

void print(struct dictionary *dict) {
	if(dict != NULL) {
		for(size_t i = 0; i < dict->count; ++i) {
			printf("\'%s\'\n", dict->words[i]);
		}
	}
}

int main() {
	struct dictionary dict;
	dict.count = 0; dict.max_size = 1; dict.words = (char **)malloc(sizeof(char *));
	if(dict.words == NULL) {
		fprintf(stderr, "Memory error!\n");
		return 1;
	}
	struct word w;
	if(new_word(&w) == 1) {
		clear(&dict, &w);
		fprintf(stderr, "Memory error!\n");
		return 1;
	}
	int qt = 0;
	int ch = 0;
	char c = 0;
	while(c != EOF) {
		ch = getchar();
		c = (char)ch;
		if(c == '\"') {
			if(qt == 1)
				qt = 0;
			else
				qt = 1;
		}
		else if((c == '\n') && (qt == 1)) {
			clear(&dict, &w);
			fprintf(stderr, "Input error!\n");
			return 1;
		}
		else if(qt == 1) {
			if(add_char(&w, c) == 1) {
				clear(&dict, &w);
				fprintf(stderr, "Memory error!\n");
				return 1;
			}
		}
		else if((c == '|') || (c == '&') || (c == ';')) {
			if(w.size != 1) {
				if((add_word(&dict, &w) == 1) || (new_word(&w) == 1) ||
						 (add_char(&w, c) == 1) || (add_word(&dict, &w) == 1) ||
						 		 (new_word(&w) == 1)) {
					clear(&dict, &w);
					fprintf(stderr, "Memory error!\n");
					return 1;
				}
			}
			else {
				if((add_char(&w, c) == 1) || (add_word(&dict, &w) == 1) ||
						 		 (new_word(&w) == 1)) {
					clear(&dict, &w);
					fprintf(stderr, "Memory error!\n");
					return 1;
				}
			}
		}
		else if(((c == ' ') || (c == '\n') || (c == '\t') || (ch == EOF)) && (w.size!=1)) {
			if((add_word(&dict, &w) == 1) || (new_word(&w) == 1)) {
				clear(&dict, &w);
				fprintf(stderr, "Memory error!\n");
				return 1;
			}
		}
		else if((c != ' ') && (c != '\t') && (c != EOF) && (c != '\n')) {
			if(add_char(&w, c) == 1) {
				clear(&dict, &w);
				fprintf(stderr, "Memory error!\n");
				return 1;
			}
		}
		if((c == '\n') || (ch == EOF)) {
			if((ch == EOF) && (dict.count != 0)) {
				printf("\n");
			}
			print(&dict);
			clear(&dict, &w);
			dict.count = 0;
			dict.max_size = 1;
			dict.words = (char **)malloc(sizeof(char *));
			if(dict.words == NULL) {
				clear(&dict, &w);
				fprintf(stderr, "Memory error!\n");
				return 1;
			}
			if(new_word(&w) == 1) { // Течет память?
				clear(&dict, &w);
				fprintf(stderr, "Memory error!\n");
				return 1;
			}
		}
	}
	clear(&dict, &w);
	return 0;
}
