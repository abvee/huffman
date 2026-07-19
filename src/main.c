#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>

#include "common.h"
#include "encoder.c"

struct {
	byte *ptr;
	unsigned int capacity;
	unsigned int len;
} buf = {NULL, 0, 0}; // general purpose buffer

void read_input();

int main(int argc, char *argv[]) {
	buf.capacity = MAX;
	memset(hash_map, 0, HM_LEN * sizeof *hash_map);

	// handles all the buffer stuff
	read_input();

	// debug
	// printf("%s", buf.ptr); // not good for long inputs
	printf("Input length: %d\n", buf.len);
	printf("Buffer capacity: %d\n", buf.capacity);

	encode(buf.ptr, buf.len);
	free(buf.ptr);
	return 0;
}

// read input into buf
void read_input() {
	buf.ptr = malloc(sizeof *buf.ptr * buf.capacity);

	int ch = 0; // getc(stdin) returns an integer, EOF == -1
	while (ch != EOF) {
		while (buf.len < buf.capacity && (ch = getc(stdin)) != EOF)
			buf.ptr[buf.len++] = ch;

		if (buf.len >= buf.capacity  && ch != EOF)
			buf.ptr = realloc(buf.ptr, sizeof *buf.ptr * (buf.capacity *= 2));
	}
}
