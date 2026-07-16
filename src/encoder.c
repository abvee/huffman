#include <stdio.h>
#include "common.h"
#include "p-queue.c"

void print_tree(struct character *current_node, unsigned int lvl);
static inline void bit_lenghts(
	struct character *current_node,
	unsigned int n_bits,
	unsigned long *op_len
);
static inline struct character *build_tree(struct character *tree);
static inline void gen_canon_codes();

/*
giant fuck off hash map for every possible character, and similar strategy for
the encodings
*/
static unsigned int hash_map[HM_LEN];

static struct {
	uint64_t bits[HM_LEN / (sizeof(uint64_t) * 8)];
	uint8_t n_bits;
} encodings[HM_LEN];

enum { TYPE_LEN_BITS = sizeof encodings[0].bits[0] * 8 };

unsigned long encode(char *buf, unsigned int buf_len) {
	/*
	Build the huffman tree, generate canonical encodings and output length in
	bits
	*/
	unsigned long op_len = 0;

	/*
	count of all the unique characters
	*/
	unsigned int char_count = 0;
	for (int i = 0; i < buf_len - 1; i++) {
		if (!hash_map[buf[i]]) char_count++;
		hash_map[buf[i]] += 1;
	}

	// full binary tree needs 2n - 1 nodes (n - 1 internal, n external)
	struct character *tree = malloc(sizeof*tree * (2 * char_count - 1));
	struct character *root = build_tree(tree);

	printf("Tree top: %d\nUnique character count: %d\n", 2 * char_count - 1, char_count);
	print_tree(root, 0);

	// Get bit lengths from tree
	pq_reset();
	bit_lenghts(root, 0, &op_len);
	pq_print();

	// write canon codes to encodings[]
	gen_canon_codes();
	// serialize the encodings and write to file
	/*
	Serialization will have to be in reverse order of the array, with last
	viable index written first.

	Since each uint64_t is also little endian encoded, I can't just write the
	bytes directly, inside each unint64_t I would also need to go last to first.
	I expect to get standard library or OS or processor help with this ?
	*/
	free(tree);
	return op_len; // placeholder
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

/*
Calculate bit lengths needed for canonical code book generation.
Also do frequency of symbols * bit length for total output length in bits
*/
inline void bit_lenghts(
	struct character *current_node,
	unsigned int n_bits,
	unsigned long *op_len
) {
	/*
	Assume a full binary tree
	*/

	// check leaf
	if (!current_node->link.left) {
		n_bits = n_bits?n_bits:n_bits+1; // handle 0 entropy edge case
		*op_len += current_node->count * n_bits;
		current_node->count = n_bits;
		pq_enqueue(current_node);
		return;
	}

	bit_lenghts(current_node->link.left, n_bits + 1, op_len);
	bit_lenghts(current_node->link.right, n_bits + 1, op_len);
}

static inline struct character *build_tree(struct character *tree) {
	unsigned int top = 0;

	// Now put in p_queue
	for (int i = 0; i < HM_LEN; i++) {
		if (hash_map[i]) {
			tree[top++] = (struct character) {i, hash_map[i], {NULL, NULL}};
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
	return current_nodes[0];
}

// generate canonical codes and fill in encodings[]
static inline void gen_canon_codes() {
	struct character *prev = pq_dequeue();
	encodings[prev->c].n_bits = prev->count;

	printf("%c(%d): %d ->\t", prev->c, prev->c, encodings[prev->c].n_bits);
	for (int i = 0; i < sizeof encodings[prev->c].bits / sizeof encodings[prev->c].bits[0]; i++) {
		encodings[prev->c].bits[i] = 0;
		printf("0x%016lx ", encodings[prev->c].bits[i]);
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
			for (int i = current_byte; i && index > current_byte; index--, i--) {
				encodings[current->c].bits[index] = encodings[current->c].bits[i];
				encodings[current->c].bits[i] = 0;
			}
		}

		/*
		now we handle the remaining shift % 64 bits of shifting
		Note that if shift & 64 is 0, then TYPE_LEN_BITS - shift is UB (can't
		shift 64 bits)
		*/
		shift &= TYPE_LEN_BITS - 1;
		if (shift) for (int i = 1; i < HM_LEN / TYPE_LEN_BITS; i++) {
			encodings[current->c].bits[i] =
				(encodings[current->c].bits[i] << shift)
				|
				(encodings[current->c].bits[i - 1] >> (TYPE_LEN_BITS - shift));
		}
		encodings[current->c].bits[0] <<= shift;

		// debug print
		if (current->c > ' ' && current->c <= '~')
			printf("%c(%d): %d ->\t", current->c, current->c, encodings[current->c].n_bits);
		else
			printf("(%d): %d ->\t", current->c, encodings[current->c].n_bits);
		for (int i = 0; i < sizeof encodings[current->c].bits / sizeof encodings[current->c].bits[0]; i++)
			printf("0x%016lx ", encodings[current->c].bits[i]);
		printf("\n");

		prev = current;
	}
}
