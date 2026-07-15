#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>

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
struct {
	uint64_t bits[HM_LEN / (sizeof(uint64_t) * 8)];
	uint8_t n_bits;
} encodings[HM_LEN];
enum {
	TYPE_LEN_BITS = sizeof encodings[0].bits[0] * 8
};

void read_input();
void print_tree(struct character *current_node, unsigned int lvl);
void bit_lenghts(struct character *current_node, unsigned int n_bits);

int main(int argc, char *argv[]) {
	buf.capacity = MAX;
	memset(hash_map, 0, HM_LEN);

	// handles all the buffer stuff
	read_input();
	// debug
	// printf("%s", buf.ptr); // not good for long inputs
	printf("Input length: %d\n", buf.len);
	printf("Buffer capacity: %d\n", buf.capacity);

	/*
	count of all the unique characters
	*/
	unsigned int char_count = 0;
	for (int i = 0; i < buf.len - 1; i++) {
		if (!hash_map[buf.ptr[i]]) char_count++;
		hash_map[buf.ptr[i]] += 1;
	}

	// full binary tree needs 2n - 1 nodes (n - 1 internal, n external)
	struct character *tree = malloc(sizeof*tree * (2 * char_count - 1));
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
	construct tree

	We use the priority queue to keep popping the least frequent nodes and
	enqueue the parent nodes

	we do that till the queue is empty, at which point we get the root node as
	the final node
	*/
	struct character *current_nodes[2];

	/*
	We don't compare both current_nodes[0] for NULL because in the end, there's
	only the root node left in the queue.

	In that situation, current_nodes[1] = dequeue() will give us NULL

	Using this:
	current_nodes[0] = pq_dequeue() && current_nodes[1] = pq_dequeue()

	Is a waste because we always break on the current_nodes[1] = pq_dequeue(),
	the first part of the expression does nothing
	*/
	for (
		current_nodes[0] = pq_dequeue();
		current_nodes[1] = pq_dequeue();
		current_nodes[0] = pq_dequeue()
	) {
		tree[top] = (struct character) {
			'\0',
			current_nodes[0]->count + current_nodes[1]->count,
			{current_nodes[0], current_nodes[1]}
		};
		pq_enqueue(&tree[top++]);
	}
	printf("Tree top: %d\nUnique character count: %d\n", top, char_count);

	// current_nodes[0] is the root node
	print_tree(current_nodes[0], 0);

	// Get bit lengths
	pq_reset();
	bit_lenghts(current_nodes[0], 0);
	pq_print();

	// generate canonical codes
	struct character *prev = pq_dequeue();
	encodings[prev->c].n_bits = prev->count;

	printf("%c(%d): %d -> ", prev->c, prev->c, encodings[prev->c].n_bits);
	for (int i = 0; i < sizeof encodings[prev->c].bits / sizeof encodings[prev->c].bits[0]; i++) {
		encodings[prev->c].bits[i] = 0;
		printf("0x%016llx ", encodings[prev->c].bits[i]);
	}
	printf("\n");


	for (struct character *current; current = pq_dequeue();) {
		encodings[current->c].n_bits = current->count;

		// copy previous bits over
		memcpy(
			encodings[current->c].bits,
			encodings[prev->c].bits,
			sizeof encodings[current->c].bits
		);


		// add 1
		for (int i = 0; !++encodings[current->c].bits[i++];);

		// shift bits
		unsigned int shift = encodings[current->c].n_bits - encodings[prev->c].n_bits;

		/*
		Okay, so basically if shift > 64, we have a problem. Shifting by > 64
		bits means we lose the everything in that array cell.

		So, we compute where we'll end up if we were to shift there, copy the
		bytes over then shift by (shift % TYPE_LEN_BITS)
		*/
		{
			unsigned int current_byte = encodings[current->c].n_bits;
			unsigned int index = (encodings[current->c].n_bits + shift) / TYPE_LEN_BITS;
			for (int i = current_byte; index > current_byte; index--, i--) {
				encodings[current->c].bits[index] = encodings[current->c].bits[i];
				encodings[current->c].bits[i] = 0;
			}
		}

		// TODO: see if if (shift) here helps performance...
		shift &= TYPE_LEN_BITS - 1;
		for (int i = 1; i < HM_LEN / TYPE_LEN_BITS; i++) {
			encodings[current->c].bits[i] =
				(encodings[current->c].bits[i] << shift)
				|
				(encodings[current->c].bits[i - 1] >> (TYPE_LEN_BITS - shift - 1));
		}
		encodings[current->c].bits[0] <<= shift;

		// debug print
		if (current->c > ' ' && current->c <= '~')
			printf("%c(%d): %d -> ", current->c, current->c, encodings[current->c].n_bits);
		else
			printf("(%d): %d -> ", current->c, encodings[current->c].n_bits);
		for (int i = 0; i < sizeof encodings[current->c].bits / sizeof encodings[current->c].bits[0]; i++)
			printf("0x%016llx ", encodings[current->c].bits[i]);
		printf("\n");

		prev = current;
	}

	free(buf.ptr);
	free(tree);
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

void print_tree(struct character *current_node, unsigned int lvl) {
	if (!current_node) return;
	// print at correct depth
	for (int i = 0; i < (int) lvl - 1; i++)
		printf("|\t");
	if (lvl) printf("|----");

	if (isalnum(current_node->c))
		printf("%c(%d):%u\n", current_node->c, current_node->c, current_node->count);
	else
		printf("(%d):%d\n", current_node->c, current_node->count);

	print_tree(current_node->link.left, lvl + 1);
	print_tree(current_node->link.right, lvl + 1);
}

void bit_lenghts(struct character *current_node, unsigned int n_bits) {
	/*
	Assume a full binary tree
	*/

	// check leaf
	if (!current_node->link.left) {
		current_node->count = n_bits;
		pq_enqueue(current_node);
		return;
	}

	bit_lenghts(current_node->link.left, n_bits + 1);
	bit_lenghts(current_node->link.right, n_bits + 1);
}
