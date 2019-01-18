#ifndef HOP_STRUCTS_H
#define HOP_STRUCTS_H

#include <linux/cdev.h>
#include <linux/hashtable.h>

#define TAIL_MASK ((1ULL << 32) - 1)
#define HEAD_MASK (~TAIL_MASK)
#define TAIL(t)	(t & TAIL_MASK)
#define HEAD(h)	((h & HEAD_MASK) >> 32)

#define register_set(val, len, off)	((val & len) << off)
#define register_get(reg, msk, off)	((val & msk) >> off)

enum ibs_states {
	IBS_ENABLED	= 0,		/* actually never used */
	IBS_ACTIVE	= 1,
	IBS_STOPPING	= 2,		/* actually never used */

	IBS_MAX_STATES,
};

struct ibs_hw {
	unsigned long 	state;		/* BITS_TO_LONGS(IBS_MAX_STATES) */
	u64		ctl;		/* logic state of msr op_ctl */
	int 		cpu;		/* related cpu */
};

/* minor used to make a chdev, can be reused */
struct minor {
	int min;
	struct list_head node;
};

/* profiled thread dedicated buffer */
struct pt_dbuf {
	struct ibs_op *buf;
	size_t size;
	/* <HEAD, TAIL> */
	u64 idx;
};


/* profiled thread information */
struct pt_info {
	int tid;			/* tid of the thread */
	struct minor *mn;		/* minor number for chdev */

	unsigned long *ctl;		/* control variable pointer */

	struct pt_dbuf *dbuf;		/* sample buffer */
	volatile unsigned long busy;
	volatile unsigned long kernel;
	volatile unsigned long memory;
	volatile unsigned long samples;
	volatile unsigned long overwritten;

	unsigned hash_bits;
	struct hlist_head *page_htable; //page_info struct

	wait_queue_head_t readq;	/* used for poll fop */
	struct mutex readl;		/* read lock */
	struct cdev cdev;		/* cdev for char dev */
	struct hlist_node node;		/* hashtable struct */
};

/* per-thread accessed page information */
struct pg_info {
	unsigned long long page;
	unsigned long counter;
	struct hlist_node node;		/* hashtable struct */
};

// mepo device
struct hop_dev {
	struct ibs_hw		ibs;
	volatile unsigned long 	requests;
	volatile unsigned long 	spurious;
	volatile unsigned long 	no_ibs;	
	volatile unsigned long 	denied;
	// DEBUG
	volatile unsigned long 	counter;
	volatile unsigned long 	latency;
	// NMI CHECK
	volatile unsigned long 	check;
	volatile unsigned long 	unknown;
};

#endif /* HOP_STRUCTS_H */