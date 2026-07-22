#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "p-queue.c"

static void print_tree(struct character *current_node, unsigned int lvl);
static inline void bit_lenghts(
	struct character *current_node,
	unsigned int n_bits,
	unsigned long *op_len
);
static inline struct character *build_tree(struct character *tree);
static inline void gen_canon_codes(
	struct character *current,
	struct character *prev
);
static inline unsigned int count_unique_characters(
	byte *buf,
	unsigned int buf_len
);

static uint serialize(
	byte *in_buffer,
	unsigned int in_buf_len,
	byte *op_buffer
);


/*
giant fuck off hash map for every possible character, and similar strategy for
the encodings
*/
static unsigned int hash_map[HM_LEN];

static struct {
	uint64_t bits[HM_LEN / (sizeof(uint64_t) * 8)];
	uint8_t n_bits;
} encodings[HM_LEN];

enum { TYPE_LEN_BITS = sizeof *(*encodings).bits * 8 };

void encode(byte *buf, unsigned int buf_len) {
	/*
	Build the huffman tree, generate canonical encodings and output length in
	bits
	*/
	unsigned long op_len = 0;

	unsigned int char_count = count_unique_characters(buf, buf_len);
	// full binary tree needs 2n - 1 nodes (n - 1 internal, n external)
	struct character *tree = malloc(sizeof *tree * (2 * char_count - 1));
	struct character *root = build_tree(tree);

	printf("Tree top: %d\nUnique character count: %d\n", 2 * char_count - 1, char_count);
	print_tree(root, 0);

	// Get bit lengths from tree
	pq_reset();
	bit_lenghts(root, 0, &op_len);
	printf("Compression Ratio: %.3f\n", (buf_len - 1) * sizeof *buf * 8 / (float) op_len);
	pq_print();

	byte *op_buffer;
	const unsigned int op_buf_len =
		1 + // char_count
		(sizeof *op_buffer * 2 * char_count) + // each of the characters
		(op_len / 8 * sizeof *op_buffer) + 1; // all the output

	printf("\nOutput buffer length: %u\n", op_buf_len);
	printf("Header buffer length: %lu\n", op_buf_len - ((op_len / 8 * sizeof *op_buffer) + 1));
	printf("Only output length: %lu\n", (op_len / 8 * sizeof *op_buffer) + 1);

	op_buffer = malloc(op_buf_len);
	op_buffer[0] = char_count - 1; // 0 -> 255
	unsigned int op_buf_i = 1;

	// dequeue each character, write pair lengths to op_buffer and generate canon codes
	struct character *prev = pq_dequeue();
	// 0 initial encoding
	encodings[prev->c].n_bits = prev->count;
	memset(encodings[prev->c].bits, 0, sizeof (*encodings).bits);

	// write to output buffer
	op_buffer[op_buf_i] = prev->c;
	op_buffer[op_buf_i + 1] = prev->count;
	op_buf_i += 2;

	// debug print
	printf("%c(%d): %d ->\t", prev->c, prev->c, encodings[prev->c].n_bits);
	for (int i = 0; i < sizeof (*encodings).bits / sizeof *(*encodings).bits; i++)
		printf("0x%016lx ", encodings[prev->c].bits[i]);
	printf("\n");

	for (struct character *current; current = pq_dequeue(); prev = current) {
		// write to output buffer
		op_buffer[op_buf_i] = current->c;
		op_buffer[op_buf_i + 1] = current->count;
		op_buf_i += 2;

		gen_canon_codes(current, prev);

		// debug print
		if (isalnum(current->c))
			printf("%c(%d): %d ->\t", current->c, current->c, encodings[current->c].n_bits);
		else
			printf("(%d): %d ->\t", current->c, encodings[current->c].n_bits);
		for (int i = 0; i < sizeof (*encodings).bits / sizeof *(*encodings).bits; i++)
			printf("0x%016lx ", encodings[current->c].bits[i]);
		printf("\n");
	}

	printf("\nSerializer\n");
	uint op_written_len = serialize(buf, buf_len, op_buffer + op_buf_i);
	// this ^ is an index
	assert(op_written_len == (op_len / 8 * sizeof *op_buffer) + 1);

	free(tree);
	free(op_buffer);
}

static inline unsigned int count_unique_characters(
	byte *buf,
	unsigned int buf_len
) {
	/*
	fill hash map with unique characters and count unique characters
	*/
	unsigned int char_count = 0;
	for (int i = 0; i < buf_len; i++) {
		if (!hash_map[buf[i]]) char_count++;
		hash_map[buf[i]] += 1;
	}
	return char_count;
} 

static void print_tree(struct character *current_node, unsigned int lvl) {
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
static inline void gen_canon_codes(
	struct character *current,
	struct character *prev
) {
	unsigned int c_in = current->c;
	encodings[c_in].n_bits = current->count;

	// copy previous bits over
	memcpy(
		encodings[c_in].bits,
		encodings[prev->c].bits,
		sizeof (*encodings).bits
	);

	// add 1
	for (int i = 0; !++encodings[c_in].bits[i++];);

	// shift bits
	unsigned int shift = encodings[c_in].n_bits - encodings[prev->c].n_bits;

	/*
	Okay, so basically if shift > 64, we have a problem. Shifting by > 64
	bits means we lose the everything in that array cell.

	So, we compute where we'll end up if we were to shift there, copy the
	bytes over. Only then do we shift by (shift % TYPE_LEN_BITS)
	*/
	{
		unsigned int current_byte = encodings[c_in].n_bits;
		unsigned int index = (encodings[c_in].n_bits + shift) / TYPE_LEN_BITS;
		for (int i = current_byte; i >= 0 && index > current_byte; index--, i--) {
			encodings[c_in].bits[index] = encodings[c_in].bits[i];
			encodings[c_in].bits[i] = 0;
		}
	}

	/*
	now we handle the remaining shift % 64 bits of shifting
	Note that if shift % 64 is 0, then TYPE_LEN_BITS - shift is UB (can't
	shift 64 bits) hence the if (shift)
	*/
	shift &= TYPE_LEN_BITS - 1;
	if (shift) for (int i = 1; i < HM_LEN / TYPE_LEN_BITS; i++) {
		encodings[c_in].bits[i] =
			(encodings[c_in].bits[i] << shift)
			|
			(encodings[c_in].bits[i - 1] >> (TYPE_LEN_BITS - shift));
	}
	encodings[c_in].bits[0] <<= shift;
}

struct {
	union {
		uint64_t ac;
		byte raw_bytes[sizeof(uint64_t)];
	};
	int i; // index
} accumulator = {.ac = 0, .i = -1};

enum { AC_BIT_LEN = sizeof accumulator.ac * 8 };

static inline void ac_flush(byte *op_buffer) {
	for (
		int i = sizeof accumulator.raw_bytes / sizeof *accumulator.raw_bytes;
		i--;
	) *op_buffer++ = accumulator.raw_bytes[i];

	accumulator.ac = 0;
	accumulator.i = -1;
}

static uint serialize(byte *in_buffer, unsigned int in_buf_len, byte *op_buffer) {
	unsigned int op_buffer_i = 0; // index

	for (unsigned int i = 0; i < in_buf_len; i++) {
		// debug print
		if (isalnum(in_buffer[i]))
			printf("%d: %c(%d): %d\t", i, in_buffer[i], in_buffer[i], encodings[in_buffer[i]].n_bits);
		else
			printf("%d: (%d): %d\t", i, in_buffer[i], encodings[in_buffer[i]].n_bits);

		unsigned int n_bits = encodings[in_buffer[i]].n_bits;

		while (n_bits > AC_BIT_LEN - (accumulator.i + 1)) {
			accumulator.ac <<= AC_BIT_LEN - (accumulator.i + 1);

			accumulator.ac |=
				encodings[in_buffer[i]].bits[n_bits / TYPE_LEN_BITS]
				>>
				(n_bits - (AC_BIT_LEN - (accumulator.i + 1)));

			n_bits -= AC_BIT_LEN - (accumulator.i + 1);

			ac_flush(op_buffer + op_buffer_i);
			op_buffer_i += sizeof accumulator.ac;
		}

		/*
		Since the size of the accumulator == sizeof encodings[].bits[0], we
		handle only index 0 here.
		*/
		accumulator.ac <<= n_bits;
		accumulator.i += n_bits;
		accumulator.ac |= encodings[in_buffer[i]].bits[0];

		// accumulator debug
		printf("%064lb: %d\n", accumulator.ac, accumulator.i);
	}
	printf("Accumulator runs: %lu\n", op_buffer_i / sizeof accumulator.ac);

	// flush the accumulator
	uint mod = accumulator.i % (sizeof *accumulator.raw_bytes * 8);
	for (
		uint byte_index = accumulator.i / (sizeof *accumulator.raw_bytes * 8);
		byte_index;
		byte_index--
	) *op_buffer++ =
		accumulator.raw_bytes[byte_index] << mod
		|
		accumulator.raw_bytes[byte_index - 1] >> (sizeof *accumulator.raw_bytes * 8 - mod);
	*op_buffer |= accumulator.raw_bytes[0] << mod;

	op_buffer_i += accumulator.i / (sizeof *accumulator.raw_bytes * 8) + 1;
	printf("Total output length: %u\n", op_buffer_i);
	return op_buffer_i;
}
