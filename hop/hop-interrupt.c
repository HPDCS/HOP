#include <asm/apic.h>
#include <asm/apicdef.h>
#include <asm/nmi.h> 		// nmi stuff
#include <asm/cmpxchg.h>	// cmpxchg function
#include <linux/sched.h>	// current macro
#include <linux/percpu.h>	// this_cpu_ptr
#include <linux/slab.h>

#include "ibs-msr-index.h"
#include "ibs-structs.h"
#include "ibs-control.h"
#include "hop-manager.h"

#include "hop-interrupt.h"
#include "hop-mod.h"
#include "hop-irq.h"		// IBS_VECTOR

#include "pmc-control.h"

extern void *pcpu_hop_dev;

#define hash_add(hashtable, bits, node, key)						\
	hlist_add_head(node, &hashtable[hash_min(key, bits)])

static inline void hash_add_or_increase(struct pt_info *pt, unsigned long long page)
{

	struct hlist_head *pg_htable = pt->page_htable;
	unsigned hash_bits = pt->hash_bits;

	struct pg_info *pg;
	unsigned long long key = page;

	/* this is the exapnded macro of 'hash_for_each_possible' */
	hlist_for_each_entry(pg, &pg_htable[hash_min(page, hash_bits)], node)
	// hash_for_each_possible(page_htable, pg, node, key)
		if(pg->page == key) {
			pg->counter ++;
			return;
		}

	pt->accessed_pages ++;

	pg = kzalloc(sizeof(struct pg_info), GFP_KERNEL);
	pg->page = page;
	pg->counter = 1;

	hash_add(pg_htable, hash_bits, &(pg->node), page);
}

#ifdef _IRQ
static inline void handle_ibs_event(void)
#else
static inline int handle_ibs_event(struct pt_regs *regs)
#endif
{
	/* NMI_DONE means that this interrupt is not for this handler */
	int retval = NMI_DONE;

	struct hop_dev *dev;		/* percpu meta info */
	// struct ibs_op *entry;		/* entry being filled */
	struct pt_info *pt;		/* profiled thread info */
//	struct pt_dbuf *dbuf;		/* pertid dedicated buffer */
	// struct pg_info *pg_info;

	unsigned long *kstack;		/* logical view of the last kernel stack entry (64) */
	u64 midx;			/* used to read MSRs content */
	// unsigned long tmp;

	unsigned long long entry;

	

	dev = this_cpu_ptr(pcpu_hop_dev);
	
	// dev->latency += (tmp - dev->counter);

	dev->requests++;		/* # of times this handler is invoked on this cpu */

	/* if there is no valid sample, exit */
	rdmsrl(MSR_IBS_OP_CTL, midx);
#ifdef _NMI
	if (!(midx & IBS_OP_VAL)) {
//		dev->no_ibs++;
		goto out;
	}
#endif
	/* ok, interrupt is for us */
	retval = NMI_HANDLED;

#ifdef _NMI
	/* we are not interested in kernel mode sample */
	if (!(regs->cs & 0x3)) {
		dev->denied++;
	}
#endif
	if (!(midx & IBS_OP_MAX_CNT))
		goto out;

	/* catch spurious interrupt */
	if (!test_bit(IBS_ACTIVE, &dev->ibs.state)) {
		dev->spurious++;
		goto out;
	}

	kstack = (unsigned long*)cur_thread_buf;

	/* the pid must be profiled and the crc consistent */
	if (!check_crc((*kstack)) || !test_bit(ENABLED_BIT, kstack)) {
		dev->denied++;
		goto out;
	}

	/* build canonical form pointer */
	pt = (struct pt_info*)build_ptr(*kstack);


	// dbuf = pt->dbuf;

	/* NMI signals that is working on buffer, anyone else must wait or abort */
//	if (test_and_set_bit(PROCESS_BIT, kstack)) {
//		pt->busy++;
//		goto out;
//	}

	/* we are not interested in kernel mode sample */
//	if (!(regs->cs & 0x3)) {
//		pt->kernel++;
//		goto skip;
//	}

	/* this is a wait-free write */
//	midx = xadd(&(pt->dbuf->idx), 1);
//	entry = &(pt->dbuf->buf[TAIL(midx) % pt->dbuf->size]);

	/* fill the entry by reading all MSRs */
//	rdmsrl(MSR_IBS_OP_CTL, entry->ibs_op_ctl.value);
//	rdmsrl(MSR_IBS_OP_RIP, entry->ibs_op_rip.value);
//	rdmsrl(MSR_IBS_OP_DATA, entry->ibs_op_data.value);
//	rdmsrl(MSR_IBS_OP_DATA2, entry->ibs_op_data2.value);
	rdmsrl(MSR_IBS_OP_DATA3, entry);
//	rdmsrl(MSR_IBS_DC_LIN_AD, entry->ibs_dc_lin_ad.value);
//	rdmsrl(MSR_IBS_DC_PHYS_AD, entry->ibs_dc_phys_ad.value);

	/* check valid memory operation */
	if (entry & IBS_DC_LIN_ADDR_VALID) {

		rdmsrl(MSR_IBS_DC_LIN_AD, entry);

		// rewrite from scratch
		hash_add_or_increase(pt, (entry >> 12));
		// pt->memory++;
	}

	/* get the time stamp counter (TSC) */
//	rdtscll(entry->tsc);
	// NOTE: at least entry-tid is gonna being deprecated
//	entry->tid = current->pid;
//	entry->pid = current->tgid;
//	entry->cpu = smp_processor_id();
//	entry->kernel = !(regs->cs & 0x3);

	/* if the buffer is full, overwrite the oldest entry */
//	if (HEAD(midx) + pt->dbuf->size == TAIL(midx)) {
//		pt->dbuf->idx += (1UL << 32);
//		pt->overwritten++;
//	}

//	pt->samples++;

//skip:
	/* re-enable IBS and add randomization to sampling */
	set_and_go_ibs_random(&dev->ibs);

	/* NMI has done */
//	clear_bit(PROCESS_BIT, kstack);
out:
#ifdef _IRQ
	apic->write(APIC_EOI, APIC_EOI_ACK);
#else
	return retval;
#endif
}// handle_ibs_event

#ifdef _IRQ
void handle_ibs_irq(void)
{
	return handle_ibs_event();
}// handle_ibs_nmi

#else

int handle_ibs_nmi(unsigned int cmd, struct pt_regs *regs)
{
	return handle_ibs_event(regs);
}// handle_ibs_nmi
#endif
