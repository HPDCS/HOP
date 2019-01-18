#include <linux/percpu.h>	/* Macro per_cpu */
#include <linux/ioctl.h>
// #include <asm/uaccess.h>
#include <linux/hashtable.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched.h>	/* task_struct */
#include <linux/cpu.h>
#include <asm/cmpxchg.h>	/* cmpxchg function */

#include "ibs-control.h"
#include "ibs-structs.h"
#include "hop-structs.h"
#include "hop-ioctl.h"

#include "hop-manager.h"
#include "hop-fops.h"
#include "hop-mod.h"
#include "hop-irq.h"


extern void *pcpu_hop_dev;	/* defined in hop-manager.c*/
extern int hop_major;		/* defined in hop-manager.c*/


int hop_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	int tid;
	struct pt_info *pt;
	unsigned int mj = imajor(inode);
	unsigned int mn = iminor(inode);

	// pr_info("OPEN file: %s", filp->f_path.dentry->d_iname);

	if (mj != hop_major || mn > NR_CPUS)
	{
		pr_warn("OPEN: No device found with [%d %d]\n", mj, mn);
		err = -ENODEV; /* No such device */
		goto out;
	}

	if (kstrtoint(filp->f_path.dentry->d_iname, 10, &tid)) {
		pr_warn("OPEN: Device [%d %d], wrong name: %s\n", mj, mn, 
			filp->f_path.dentry->d_iname);
		err = -EINVAL;
		goto out;
	}

	pt = lookup(tid);
	if (!pt) {
		pr_warn("OPEN: Device [%d %d] cannot find tid %u metadata (is it registered?)\n", mj, mn, tid);
		err = -EINVAL;
		goto out;	
	}
	filp->private_data = (struct pt_info *)pt;
	goto out;

out:
	return err;
}// HOP_open

int hop_release(struct inode *inode, struct file *file)
{
	return 0;
}// hop_release

int hop_ctl_open(struct inode *inode, struct file *filp)
{
	return 0;
}// hop_ctl_open


int hop_ctl_release(struct inode *inode, struct file *file)
{
	return 0;
}// hop_ctl_release

static int read_buffer(struct pt_dbuf *dbuf, struct ibs_op *entry)
{
	u64 midx;
	struct ibs_op tmp;
	while(1) {
		do {
			midx = dbuf->idx;
		}
		while(HEAD(midx) + dbuf->size < TAIL(midx));

		if (HEAD(midx) == TAIL(midx))
			return 0;	/* empty */

		memcpy(&tmp, &(dbuf->buf[HEAD(midx) % dbuf->size]),
			sizeof(struct ibs_op));

		if (cmpxchg(&(dbuf->idx), midx, midx + (1UL << 32)) == midx)
			goto read;	/* success */
	}
read:
	return !copy_to_user(entry, &tmp, sizeof(struct ibs_op));
}// read_buffer

ssize_t hop_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int samp = 0;
	size_t space = count / sizeof(struct ibs_op);
	struct ibs_op *nbuf = (struct ibs_op *) buf;
	struct pt_info *pt = (struct pt_info *) file->private_data;

	if (!space) return -ENOMEM;

	while (samp < space && read_buffer(pt->dbuf, &(nbuf[samp]))) {
		samp++;
	}

	return samp * sizeof(struct ibs_op);
}// hop_read

long hop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;
	struct pt_info *pt = (struct pt_info *) file->private_data;
	struct tid_stats stats = {.pages = 0};

	/* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
	if (_IOC_TYPE(cmd) != HOP_IOC_MAGIC) return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));

	switch (cmd) {
	case HOP_TID_STATS:
		stats.tid = pt->tid;
		stats.busy = pt->busy;
		stats.kernel = pt->kernel;
		stats.memory = pt->memory;
		stats.samples = pt->samples;

		stats.pages_length = thread_stats_page_access(pt->tid, stats.pages);

		if (copy_to_user((struct tid_stats __user *)arg, &(stats), sizeof(struct tid_stats))) {
			err = -EFAULT;
		}
		break;
	case HOP_TID_PAGES:
		stats.pages_length = thread_stats_page_access(pt->tid, &(stats.pages));
		
		pr_info("Ready to send %llu pages - address: %llx\n", stats.pages_length, stats.pages);
		
	//	for (err = 0; err < stats.pages_length; ++err)
			pr_info("[%llx]: %llu\n", stats.pages->page, stats.pages->counter);

		err = 0;
			
		if (copy_to_user((void __user *)arg, (void *) stats.pages, sizeof(struct tid_page) * stats.pages_length)) {
			err = -EFAULT;
		}
		if (stats.pages_length)
			vfree(stats.pages);
	}
	return err;
}// hop_ioctl

static void clear_stats(void)
{
	int cpu;
	struct hop_dev *dev;

	for_each_possible_cpu(cpu) {
		dev = per_cpu_ptr(pcpu_hop_dev, cpu);
		dev->requests = 0;
		dev->spurious = 0;
		dev->no_ibs = 0;	
		dev->denied = 0;
		dev->latency = 0;
	}
}// clear_stats

static void fill_ctl_stats(struct ctl_stats *stats)
{
	int cpu;
	struct hop_dev *dev;

	stats->nr_cpu = 0;
	stats->no_ibs = 0;
	stats->spurious = 0;
	stats->requests = 0;
	stats->denied = 0;
	stats->latency = 0;
	
	for_each_possible_cpu(cpu) {
		dev = per_cpu_ptr(pcpu_hop_dev, cpu);
		stats->no_ibs += dev->no_ibs;
		stats->spurious += dev->spurious;
		stats->requests += dev->requests;
		stats->denied += dev->denied;
		stats->latency += dev->latency;
		pr_info("HOP STATS: no_ibs %lu, spurious %lu, requests %lu, denied %lu, latency%lu", 
			dev->no_ibs, dev->spurious, dev->requests, dev->denied, dev->latency);
	}

}// fill_ctl_stats

long hop_ctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	// u64 debug;
	int val;
	long err = 0;
	struct ctl_stats stats;

	// int cmd_no = _IOC_NR(cmd);

	/* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
	if (_IOC_TYPE(cmd) != HOP_IOC_MAGIC) return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) goto out;
 
	switch (cmd) {
	case HOP_PROFILER_ON:
		prf_state = 1;
		pr_info("HOP_PROFILER_ON\n");
		break;
	case HOP_PROFILER_OFF:
		prf_state = 0;
		pr_info("HOP_PROFILER_OFF\n");
		break;
	case HOP_DEBUGGER_ON:
		pr_info("HOP_DEBUGGER_ON\n");
		break;
	case HOP_DEBUGGER_OFF:
		pr_info("HOP_DEBUGGER_OFF\n");
		break;    
	case HOP_CLEAN_TIDS:
		cleanup_active_threads();
		clear_stats();
		pr_info("HOP_CLEAN_TIDS\n");
		break;
	case HOP_CTL_STATS:
		fill_ctl_stats(&stats);
		if (copy_to_user((struct ctl_stats __user *)arg, &(stats), sizeof(struct ctl_stats))) {
			err = -EFAULT;
		}
		break;
	case HOP_ADD_TID:
		__get_user(val, (int __user *) arg);
		err = setup_thread_resources((pid_t)val);
		if (!err) pr_info("start monitoring TID %u", (pid_t)val);
		break;
	case HOP_DEL_TID:
		__get_user(val, (int __user *) arg);
		cleanup_thread_resources((pid_t)val);
		pr_info("stop monitoring TID %u", (pid_t)val);
		break; 
	case HOP_START_TID:
		__get_user(val, (int __user *) arg);
		err = enable_profiler_thread((pid_t)val);
		if (!err) pr_info("profiler switched ON for TID %u", (pid_t)val);
		break;
	case HOP_STOP_TID:
		__get_user(val, (int __user *) arg);
		disable_profiler_thread((pid_t)val);
		// create a new FOP
		if (!err) pr_info("profiler switched OFF for TID %u", (pid_t)val);
		break;
	case HOP_SET_BUF_SIZE:
		__get_user(val, (int __user *) arg);
		if (val < MININUM_BUFFER_SIZE) {
			err = -EINVAL;
		} else {
			buff_size = val;
			pr_info("NEW buffer size: %u", buff_size);
		}
		break;
	case HOP_SET_SAMPLING:
		__get_user(val, (int __user *) arg);
		/* NOTE: this is hard coded */
		if (val < DEFAULT_MIN_CNT || val > (LEN_OP_MAX_CNT << 4)) {
			err = -EINVAL;
		} else {
			sampling_frequency = (val >> 4);
			pr_info("NEW sampling freq: %u", val);
		}
		break;
	default:	/* Command not recognized */
		err = -ENOTTY;
	}

out:
	return err;
}// hop_ctl_ioctl
