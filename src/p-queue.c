#include "common.h"

/*
I wanted to make this a dynamic array, but if we're hardcoding the hash map as
a direct map, might as well do the same for the priority queue
*/
static struct character queue[HM_LEN];
static unsigned int front = 0, back = 0;

constexpr unsigned int MASK = HM_LEN - 1; // should be 2 ** 8 - 1

bool pq_enqueue(struct character c) {
	// overflow
	if ((back + 1) % HM_LEN == front) return false;
	
	// enqueue the actual character
	int i = front;
	for (; i < back && queue[i].count <= c.count; i = (i + 1) & MASK);
	queue[i] = c;
	back = (back + 1) & MASK;
	return true;
}

struct character pq_dequeue() {
	// queue full
	if (front == back) return (struct character) {'\0', 0};

	unsigned int f = front; // tmp
	front = (front + 1) & MASK;
	return queue[f];
}
