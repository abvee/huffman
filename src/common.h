#ifndef PROJECT_COMMON
#define PROJECT_COMMON

#include <stdint.h>
typedef uint8_t byte;
typedef uint64_t u64;
typedef unsigned int uint;
enum {
	BYTE_BIT_LEN = sizeof(byte)  * 8,
	U64_BIT_LEN = sizeof(u64) * 8,
};

constexpr uint HM_LEN = pow(2, sizeof(char) * 8);
constexpr uint TREE_MAX = 2 * HM_LEN - 1;

typedef union {
	u64 a[HM_LEN / U64_BIT_LEN]; // the 'a' stands for array, couldn't come up with a better name
	byte bytes[HM_LEN / BYTE_BIT_LEN];
} u256;

// character to store in p_queue
struct treelink {
	struct character *left;
	struct character *right;
};

struct character {
	byte c;
	unsigned int count;
	struct treelink link;
};

enum {MAX = 128};

bool print_flag = false; // replace with bit field if we get many more options
bool decode_flag = false;

#include <stdarg.h>
int f_printf(const char *restrict format, ...) {
	va_list args;
	va_start(args, format);

	int ret = 0;
	if (print_flag)
		ret = vprintf(format, args);
	va_end(args);
	return ret;
}
#endif
