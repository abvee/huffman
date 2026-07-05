
#ifndef PROJECT_COMMON
#define PROJECT_COMMON
// character to store in p_queue
struct character {
	char c; // idk if this is signed or unsigned, I just need it to store a raw byte
	unsigned int count;
};

enum {MAX = 128};

constexpr unsigned int HM_LEN = pow(2, 8);
#endif
