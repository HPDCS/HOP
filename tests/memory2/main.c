#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <linux/version.h>

#include "dict.h"

#define PAGESIZE	4096
#define MEM_PER_ITER	100*PAGESIZE
#define __memory_addr	(1ULL << 46)//(unsigned long long) ((0xbeccaULL << 12) << 12)

#define SEED		0xbadf00d;

#define DUMP_ORACLE

#define PAGE(addr)	(void *)((unsigned long long)addr & ~(PAGESIZE - 1))

#ifdef DUMP_ORACLE
struct oracle {
	void *page;
	unsigned long count;
};

__thread dict_t *page_count;

static void oracle(unsigned char *addr)
{
	char address[512];
	void *page = PAGE(addr);

	snprintf(address, 512, "%p", page);
	
	struct oracle *or = dict_get(page_count, address);
	if(or == NULL) {
		or = malloc(sizeof(struct oracle));
		or->page = page;
		or->count = 0;
		dict_add(page_count, address, or);
	}

	or->count++;
}

void do_dump_oracle(void *_or)
{
	struct oracle *or = (struct oracle *)_or;
	printf("%p\t%lu\n", or->page, or->count);
}

void dump_oracle(void)
{
	dict_iter(page_count, do_dump_oracle);	
}

#define ORACLE oracle


#else
#define ORACLE(...)
#endif

static __thread unsigned char *memory;
static __thread unsigned long long rnd_seed;

static inline unsigned rng(void)
{
	unsigned long long c = 7319936632422683443ULL;
	unsigned long long x = (rnd_seed += c);

	x ^= x >> 32;
	x *= c;
	x ^= x >> 32;
	x *= c;
	x ^= x >> 32;

	/* Return lower 32bits */
	return x;
}

static void mem_write(unsigned char *ptr, size_t size)
{
	size_t i, j;
	size_t start, span;

	start = rng() % size;
	span = rng() % PAGESIZE;

	bzero(ptr, size);

	if (!size)
		return;
	for (i = start; i < size; i += span) {
		j = (size_t)ptr ^ i;
		ptr[i] = j ^ (j >> 8);
		ORACLE(&ptr[i]);
	}
}

int main(int argc, char **argv)
{
	int i, j, iterations, opcount;
	unsigned char *base;
	size_t buflen;
	int flags = MAP_ANONYMOUS | MAP_PRIVATE;
	
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
	flags |= MAP_FIXED_NOREPLACE;
	#else
	flags |= MAP_FIXED;
	#endif

	if(argc != 3) {
		printf("Usage: %s <iterations> <opcount>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	iterations = atoi(argv[1]);	
	opcount = atoi(argv[2]);	
	buflen = iterations * MEM_PER_ITER;

	// TODO: spawn threads here for multithreaded execution

	memory = mmap(
		(void*) __memory_addr, // Change to NULL for multithreaded execution
		buflen, 
		PROT_READ | PROT_WRITE,
		flags,
		-1,
		0);
	
	if (memory == MAP_FAILED) { 
		perror("mmap"); 
		exit(EXIT_FAILURE);
	}

	rnd_seed = SEED;

	#ifdef ORACLE
	page_count = dict_new();
	#endif

	for(i = 0; i < iterations; i++) {
		base = memory + (i * MEM_PER_ITER);

		for(j = 0; j < opcount; j++) {
			mem_write(base, MEM_PER_ITER);
		}
	}	

	munmap(memory, buflen);

	#ifdef ORACLE
	dump_oracle();
	dict_free(page_count, true);
	#endif
}

