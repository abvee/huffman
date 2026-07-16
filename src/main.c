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
	char *ptr;
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

	unsigned long bits = encode(buf.ptr, buf.len);
	free(buf.ptr);
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
