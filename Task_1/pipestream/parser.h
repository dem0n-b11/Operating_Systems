#ifndef _PARSER_H_
#define _PARSER_H_

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

void clear();
void clear(struct dictionary *dict, struct word *w);
void clear_word(struct word *w);
void clear_dict(struct dictionary *dict);
int split(char *str, struct dictionary *dict);
#endif
