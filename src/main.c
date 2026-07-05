#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "p-queue.c"

struct {
	signed char *ptr;
	unsigned int capacity;
	unsigned int len;
} buf = {NULL, 0, 0}; // general purpose buffer
/*
giant fuck off hash map for every possible character
*/
unsigned int hash_map[HM_LEN];

void read_input();

int main(int argc, char *argv[]) {
	buf.capacity = MAX;
	memset(hash_map, 0, HM_LEN);

	// handles all the buffer stuff
	read_input();
	// debug
	printf("%s", buf.ptr);
	printf("%d\n", buf.len);
	printf("%d\n", buf.capacity);

	/*
	count of all the unique characters
	*/
	for (int i = 0; i < buf.len; i++)
		hash_map[buf.ptr[i]]+=1;
	// Now put in p_queue
	for (int i = 0; i < HM_LEN; i++)
		if (hash_map[i]) pq_enqueue((struct character) {i, hash_map[i]});
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
