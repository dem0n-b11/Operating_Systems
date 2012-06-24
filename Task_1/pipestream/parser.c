#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

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

void clear_word(struct word *w) {
	if(w != NULL) {
		free(w->lexeme);
	}
}

void clear_dict(struct dictionary *dict) {
	if(dict != NULL) {
		for(size_t i = 0; i < dict->count; ++i)
			free(dict->words[i]);
		free(dict->words);
	}
}


int add_word(struct dictionary *dict, struct word *w) {
	if((dict == NULL) || (w == NULL)) {
		return 1;
	}
	++(dict->count);
	if(dict->count >= dict->max_size) {
		dict->max_size *= 2;
		char **tmp = (char **)realloc(dict->words, dict->max_size * sizeof(char *));
		if(tmp == NULL) {
			return 1;
		}
		dict->words = tmp;
	}
	dict->words[dict->count - 1] = w->lexeme;
	dict->words[dict->count] = NULL;
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

int split(char *str, struct dictionary *dict) {
	dict->count = 0; dict->max_size = 1; dict->words = (char **)malloc(sizeof(char *));
	dict->words[0] = NULL;
	if(dict->words == NULL) {
		return 1;
	}
	struct word w;
	if(new_word(&w) == 1) {
		clear(dict, &w);
		fprintf(stderr, "Memory error!\n");
		return 1;
	}
	int qt = 0;
	char c = 1;
	size_t k = 0;
	while(c != 0) {
		c = str[k];
		++k;
		if((c == '\"') || (c == '\'')) {
			if(qt == 1)
				qt = 0;
			else
				qt = 1;
		}
		else if((c == '\n') && (qt == 1)) {
			clear(dict, &w);
			return 1;
		}
		else if(qt == 1) {
			if(add_char(&w, c) == 1) {
				clear(dict, &w);
				return 1;
			}
		}
		else if((c == '|') || (c == '&') || (c == ';')) {
			if(w.size != 1) {
				if((add_word(dict, &w) == 1) || (new_word(&w) == 1) ||
						 (add_char(&w, c) == 1) || (add_word(dict, &w) == 1) ||
						 		 (new_word(&w) == 1)) {
					clear(dict, &w);
					return 1;
				}
			}
			else {
				if((add_char(&w, c) == 1) || (add_word(dict, &w) == 1) ||
						 		 (new_word(&w) == 1)) {
					clear(dict, &w);
					return 1;
				}
			}
		}
		else if(((c == ' ') || (c == '\t') || (c == 0)) && (w.size!=1)) {
			if((add_word(dict, &w) == 1) || (new_word(&w) == 1)) {
				clear(dict, &w);
				return 1;
			}
		}
		else if((c != ' ') && (c != '\t') && (c != EOF) && (c != '\n')) {
			if(add_char(&w, c) == 1) {
				clear(dict, &w);
				return 1;
			}
		}
	}
	clear_word(&w);
	return 0;
}
