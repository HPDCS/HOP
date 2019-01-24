/* mmap */
#include <sys/mman.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>

#include <linux/version.h>

#define DEBUG		0
#define printd(...)	{if(DEBUG) printf(__VA_ARGS__);}


#define N_TOTAL		1
#define N_THREADS	1
#define N_TOTAL_PRINT	50

#define RANDOM(s)	(rng() % (s))

#define MSIZE		getpagesize()
#define I_MAX		2048 * 8
// #define ACTIONS_MAX	1024 * 128

#define WRITE_ACT_MAX	128
#define FREE_ACT_MAX	256
#define ALLOC_ACT_MAX	FREE_ACT_MAX

#define PAGE_ACT_MAX	(WRITE_ACT_MAX * 1024)

#define BINS_MAX	128


// compact __ALIGN_MASK into ALIGN
#define ALIGN(x, a)              (((x) + ((typeof(x))(a) - 1)) &~ ((typeof(x))(a) - 1))

#define __memory_addr 		(1ULL << 46)//(unsigned long long) ((0xbeccaULL << 12) << 12)

#define __memory_size		getpagesize() * 1024 * 128 //ALIGN(1 << 8, 4) // space for 256 pages

static void 			*__memory;
static unsigned int 		__offset;

#define available_memory	(__memory_size - __offset)

static unsigned all_pages [4096];
static unsigned page_ctr = 0;

void __alloc_memory_pool() {

	int flags = MAP_ANONYMOUS | MAP_PRIVATE;
	
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
	flags |= MAP_FIXED_NOREPLACE;
	#else
	flags |= MAP_FIXED;
	#endif

	__memory = mmap(
		(void*) __memory_addr,
		(size_t) __memory_size, 
		PROT_READ | PROT_WRITE,
		flags,
		-1,
		0);
	
	if (__memory == MAP_FAILED) 
		do { perror("mmap"); exit(EXIT_FAILURE); } while (0);

	__offset = 0;
	printd("[END] memory: req: %llx, addr %llx (%u) \n", __memory_addr, __memory, __memory_size);
}// __alloc_memory_pool

void __free_memory_pool() {
	munmap((void*) __memory_addr, (size_t) (__memory_size));
}// __free_memory_pool


/* wrap memory calls */
void *__alloc(size_t size)
{
	printd("[GO] alloc: space (%llx), req (%u)\n",
		available_memory, size);

	if (available_memory <= size) {
		printd("[ABORT] alloc has been called: \
			mem %llx, req (%u)\n", available_memory, size);
		return NULL;
	}

	void *alloc_mem = __memory + __offset;
	__offset += size;

	printd("[END] alloc: addr %llx, size %u, offset %u\n", alloc_mem, size, __offset);
	return alloc_mem;	
}// __alloc

void __free(void *ptr)
{
	printf("[%llx] free has been called\n", ptr);
}// __free

// Variables for execution
pthread_cond_t	finish_cond;
pthread_mutex_t finish_mutex;
int n_total = 0;
int n_total_max = N_TOTAL;
int n_running;


/*
 * Ultra-fast RNG: Use a fast hash of integers.
 * 2**64 Period.
 * Passes Diehard and TestU01 at maximum settings
 */
__thread unsigned long long rnd_seed;

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


struct bin {
	unsigned char *ptr;
	size_t size;
};

struct bin_info {
	struct bin *m;
	size_t size, bins;
};

struct thread_st {
	int bins, max, flags;
	size_t size;
	pthread_t id;
	size_t seed;
	int counter;
};

__thread struct thread_st *st;


static void free_it(struct bin *m) {
	printd("[END] free_it\n");
}


/*
 * Allocate a bin.
 * r must be a random number >= 1024.
 */
static void bin_alloc(struct bin *m, size_t size, unsigned r)
{

	unsigned long long page;

	// if (m->size > 0) // free it!
	printd("[GO] bin_alloc: size (%u)\n", size);
	
	m->ptr = __alloc(size);

	page = (unsigned long long) (m->ptr) / getpagesize();

	// printf("[PAGE] %llx \t\t [ADDR] %llx\n", page, m->ptr);

	// increse per_page counter and number counter
	if (!all_pages[page - 0x400000000]++) ++page_ctr;

	// do we need this?
	if (!m->ptr) {
		printd("[%d] out of memory (r=%d, size=%ld)!\n",
			st->counter, r, (unsigned long)size);
		exit(1);
	}

	printd("[-] bin_alloc: size (%u)\n", size);
	m->size = size;
	printd("[END] bin_alloc: size (%u)\n", size);
}

/* Free a bin. */
static void bin_free(struct bin *m)
{
	if (!m->size) return;

	free_it(m);
	m->size = 0;
}// bin_free

static void *memory_test(void *ptr)
{
	int i, pid = 1;
	unsigned long long page;
	unsigned b, j, k, h, actions, action;
	struct bin_info p;

	printf("[GO] memory_test\n");


	st = ptr;

	rnd_seed = st->seed;

	p.m = malloc(st->bins * sizeof(*p.m));
	p.bins = st->bins;
	p.size = st->size;

	// Init bins
	printd("[-] init bins: bins %u\n", p.bins);
	for (b = 0; b < p.bins; b++) {
		p.m[b].size = 0;
		p.m[b].ptr = NULL;
		if (!RANDOM(2))
			bin_alloc(&p.m[b], RANDOM(p.size) + 1, rng());
	}

	for (i = 0; i <= st->max;) {

		// printf("[-] for-loop: i %u, max %llu\n", i, st->max);
		
		
		actions = RANDOM(WRITE_ACT_MAX);

		for (j = 0; j < actions; j++) {
			b = RANDOM(p.bins);
			if (p.m[b].size) {
				k = 0;
				h = RANDOM(PAGE_ACT_MAX);
				while(++k < h) {

					*p.m[b].ptr += 127;

					page = (unsigned long long) (p.m[b].ptr) / getpagesize();

					if (!all_pages[page - 0x400000000]++) ++page_ctr;
				}

				// printf("[PAGE] %llx \t\t [ADDR] %llx\n", 
					// (unsigned long long) (p.m[b].ptr) / getpagesize(), p.m[b].ptr);
			}
			// bin_test(&p);
		}

		i += actions;
		
		actions = RANDOM(FREE_ACT_MAX);
		
		for (j = 0; j < actions; j++) {
			b = RANDOM(p.bins);
			bin_free(&p.m[b]);
		}

		i += actions;

		actions = RANDOM(ALLOC_ACT_MAX);

		for (j = 0; j < actions; j++) {
			b = RANDOM(p.bins);
			action = rng();
			bin_alloc(&p.m[b], RANDOM(p.size) + 1, action);
		}

		i += actions;
	}

	for (b = 0; b < p.bins; b++)
		bin_free(&p.m[b]);

	free(p.m);

	if (pid > 0) {
		pthread_mutex_lock(&finish_mutex);
		st->flags = 1;
		pthread_cond_signal(&finish_cond);
		pthread_mutex_unlock(&finish_mutex);
	}

	printf("[END] memory_test\n");

	return NULL;
}// memory_test

static int start_thread(struct thread_st *st)
{
	printf("start_thread called\n");
	return pthread_create(&st->id, NULL, memory_test, st);
}// start_thread

static void end_thread(struct thread_st *st)
{
	/* Thread st has finished.  Start a new one. */
	if (n_total >= n_total_max) {
		n_running--;
	// } else if (st->seed++, start_thread(st)) {
	// 	printf("Creating thread #%d failed.\n", n_total);
	} else {
		n_total++;
		if (!(n_total % N_TOTAL_PRINT)) printf("n_total = %d\n", n_total);
	}
}// end_thread

int main(int argc, char const *argv[])
{

	int i, bins;
	int n_thr = N_THREADS;
	int i_max = I_MAX;
	size_t size = MSIZE;
	struct thread_st *st;

	bins = BINS_MAX;

	// init memory space
	__alloc_memory_pool();

	pthread_cond_init(&finish_cond, NULL); // TODO!!
	pthread_mutex_init(&finish_mutex, NULL);

	printf("total=%d threads=%d i_max=%d size=%ld bins=%d\n", n_total_max, n_thr, i_max, size, bins);


	st = malloc(n_thr * sizeof(*st));
	if (!st)
		exit(-1);

	pthread_mutex_lock(&finish_mutex);


	/* Start all n_thr threads. */
	for (i = 0; i < n_thr; i++) {
		st[i].bins = bins;
		st[i].max = i_max;
		st[i].size = size;
		st[i].flags = 0;
		st[i].counter = i;
		st[i].seed = (i_max * size + i) ^ bins;
		if (start_thread(&st[i])) {
			printf("Creating thread #%d failed.\n", i);
			n_thr = i;
			break;
		}
		printf("Created thread %lx.\n", (long)st[i].id);
	}

	for (n_running = n_total = n_thr; n_running > 0;) {

		printf("Up and Running\n");
		/* Wait for subthreads to finish. */
		pthread_cond_wait(&finish_cond, &finish_mutex);
		for (i = 0; i < n_thr; i++) {
			if (st[i].flags) {
				pthread_join(st[i].id, NULL);
				st[i].flags = 0;
				end_thread(&st[i]);
			}
		}
	}



	pthread_mutex_unlock(&finish_mutex);

	free(st);
	
	// free memory space
	__free_memory_pool();

	printf("[page_ctr]%x\n", page_ctr);

	for (i = 0; i < page_ctr && i < 4096; ++i)
		printf("[%llx]\t%u\n", (0x400000000 + i), all_pages[i]);
	
	printf("Done.\n");
	return 0;
}// main


