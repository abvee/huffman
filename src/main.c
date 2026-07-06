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
	// printf("%s", buf.ptr); // not good for long inputs
	printf("%d\n", buf.len);
	printf("%d\n", buf.capacity);

	/*
	count of all the unique characters
	*/
	unsigned int char_count = 0;
	for (int i = 0; i < buf.len - 1; i++) {
		if (!hash_map[buf.ptr[i]]) char_count++;
		hash_map[buf.ptr[i]] += 1;
	}

	struct character *tree = malloc(sizeof(struct character) * char_count - 1);
	unsigned int top = 0;

	// Now put in p_queue
	for (int i = 0; i < HM_LEN; i++) {
		if (hash_map[i]) {
			tree[top++] = (struct character) {i, hash_map[i], NULL, NULL};
			pq_enqueue(tree + top - 1);
		}
	}
	// Note that top ends up as the length of the hashmap

	// debug
	pq_print();
	/*
	for (int i = 0; i < top; i++)
		printf("%p %p\n", tree[i].link.left, tree[i].link.right);
	*/

	/*
	construct tree

	We use the priority queue to keep popping the least frequent nodes and
	enqueue the parent nodes

	we do that till the queue is empty, at which point we get the root node as
	the final node
	*/
	struct character *current_nodes[2];
	while ((current_nodes[0] = pq_dequeue()) && (current_nodes[1] = pq_dequeue())) {
		tree[top] = (struct character) {
			'\0',
			current_nodes[0]->count + current_nodes[1]->count,
			{current_nodes[0], current_nodes[1]}
		};
		pq_enqueue(&tree[top++]);
	}
	printf("%d\n%d\n", top, char_count);

	/*
	current_nodes[0] is the root node

	We can start encoding or we can start doing whatever we want here
	*/
	// print_tree(current_nodes[0]);
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
