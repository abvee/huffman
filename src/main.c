#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

enum {MAX = 128};

struct {
	signed char *ptr;
	unsigned int capacity;
	unsigned int len;
} buf = {NULL, 0, 0}; // general purpose buffer

void read_input();

int main(int argc, char *argv[]) {
	buf.capacity = MAX;

	// handles all the buffer stuff
	read_input();

	/*
	count of all the unique characters
	*/
	return 0;
}

// read input into buf
void read_input() {
	buf.ptr = malloc(sizeof *buf.ptr * buf.capacity);
	do {
		while (buf.len < buf.capacity && (buf.ptr[buf.len++] = getc(stdin)) != EOF);

		if (buf.len >= buf.capacity && buf.ptr[buf.len - 1] != EOF)
			buf.ptr = realloc(buf.ptr, sizeof *buf.ptr * (buf.capacity *= 2));
	} while (buf.ptr[buf.len - 1] != EOF);
	buf.ptr[buf.len - 1] = '\0';
}
