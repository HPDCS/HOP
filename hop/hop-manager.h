#ifndef HOP_MANAGER_H
#define HOP_MANAGER_H

#include <linux/cdev.h>
#include <linux/hashtable.h>

#include "hop-ioctl.h"
#include "ibs-control.h"

/* 
 * man 5 proc:
 * /proc/sys/kernel/pid_max (since Linux 2.5.34)
 * This file specifies the value at which PIDs wrap around 
 * (i.e., the value in this file is one greater than the maximum  PID).
 * PIDs greater than this value are not allocated; thus,
 * the value in this file also acts as a system-wide limit 
 * on the total number of processes and threads.
 * The default value for this file, 32768, results in the same 
 * range of PIDs as on earlier kernels. On 32-bit platforms, 32768 is
 * the maximum value for pid_max. On 64-bit systems, pid_max can be
 * set to any value up to 2^22 (PID_MAX_LIMIT, approximately 4 million)
 *
 * This module can monitor up to 255 threads.
 */

/**
 * [0:47] 	buffer pointer
 * [48:61]	crc
 * [62]		p bit
 * [63]		e bit
 */
#define PTR_BITS		48
#define PTR_THR_BIT		(1ULL << (PTR_BITS - 1))			/* threshold bit */
#define PTR_USED_MASK 		((1ULL << PTR_BITS) - 1)			// 0x0000F...F
#define PTR_FREE_MASK		(~PTR_USED_MASK)				// 0xFFFF0...0
#define PTR_CRC_MASK		(((1ULL << (64 - PTR_BITS - 2)) - 1) << 48)	// 0x3FFF0...0
#define PTR_CCB_MASK		((1ULL << (64 - PTR_BITS - 2)) - 1)		// 0x0...03FFF
#define PROCESS_BIT		62
#define ENABLED_BIT		63
#define PROCESS_MASK		(1ULL << PROCESS_BIT)
#define ENABLED_MASK		(1ULL << ENABLED_BIT)

#define CRC_MAGIC		0xc
#define set_crc(ctl)		(ctl.crc = ctl.ccb ^ CRC_MAGIC)
#define check_crc(ctl)		(((ctl & PTR_CCB_MASK) ^ CRC_MAGIC) == ((ctl & PTR_CRC_MASK) >> 48))
#define build_ptr(ptr) 		((ptr & PTR_THR_BIT) ? PTR_FREE_MASK | ptr : PTR_USED_MASK & ptr)

#define NR_ALLOWED_TIDS		255
#define HOP_MINORS		(NR_ALLOWED_TIDS + 1)

#define MININUM_BUFFER_SIZE	(1 << 8)	/* 256 */
#define DEFAULT_BUFFER_SIZE	(1 << 12)	/* 4096 */
#define DEFAULT_STATE		0

extern int hop_major;
extern unsigned prf_state;
extern unsigned buff_size;
extern void *pcpu_hop_dev;
// extern struct hop_ctl hop_ctl;

#define tid_thread_buf(ptr) 	((u64)ptr->stack + sizeof(struct thread_info) + sizeof(u64))
#define cur_thread_buf 		((u64)current_thread_info() + sizeof(struct thread_info) + sizeof(u64))

struct pt_info *lookup(pid_t tid);

void cleanup_active_threads(void);

int thread_stats_page_access(pid_t tid, struct tid_page **pages);

int enable_profiler_thread(pid_t tid);

int disable_profiler_thread(pid_t tid);

int setup_thread_resources(pid_t tid);

void cleanup_thread_resources(pid_t tid);

int setup_resources(void);

void cleanup_resources(void);

#endif /* HOP_MANAGER_H */

