#include <linux/moduleparam.h>
#include <linux/hashtable.h>
#include <linux/sched.h>		/* current macro */
#include <linux/percpu.h>		/* this_cpu_ptr */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include "ibs-control.h"
#include "ibs-structs.h"
#include "hop-structs.h"
#include "hop-manager.h"
#include "hop-fops.h"
#include "hop-mod.h"

#include "pmc-control.h"

#define HOP_DEV_MINOR 0

static spinlock_t hash_lock;
static spinlock_t list_lock;
static struct cdev hop_cdev;
static struct class *hop_class = NULL;

// unsigned long monitor_hook = (unsigned long) threads_monitor_hook;
// #define PERMISSION_MASK (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
// module_param(monitor_hook, ulong, PERMISSION_MASK);

/** declare here the global variables **/
int hop_major = 0;
unsigned prf_state = DEFAULT_STATE;
unsigned buff_size = DEFAULT_BUFFER_SIZE;
void *pcpu_hop_dev;

/**
 * This list contains the available minors for further allocations.
 * Every time a new thread becames profiled, a minor number is got
 * from this list and a char device is created for it. When the pt
 * is deleted, the char device is destroyed and the minor number
 * comes back the tail of the list.
 */
LIST_HEAD(free_minors);

/* this is used just to cleanup minors at the end */
struct minor *ms;

#define LCK_LIST spin_lock(&list_lock)
#define UCK_LIST spin_unlock(&list_lock)
#define LCK_HASH spin_lock(&hash_lock)
#define UCK_HASH spin_unlock(&hash_lock)

#define get_minor(mn) \
	LCK_LIST; \
	mn = list_first_entry_or_null(&free_minors, struct minor, node); \
	if (mn) list_del(&(mn->node)); \
	UCK_LIST

#define put_minor(mn) \
	LCK_LIST; \
	if (mn) list_add_tail(&(mn->node), &free_minors); \
	UCK_LIST


DEFINE_HASHTABLE(tid_htable, 8);

/**
 * This function will feed the schedule-hook
 * module. It monitors the current thread each
 * context-switch end and decides if enabling
 * or disabling IBS support for profiling.
 */
// NOTE: check code
void threads_monitor_hook(void)
{
	/* logical view of the last kernel stack entry (64) */
	unsigned long *kstack;
	struct hop_dev *dev = this_cpu_ptr(pcpu_hop_dev);

	if (!prf_state) goto ibs_off;
	kstack = (unsigned long*) cur_thread_buf;

	/* hopefully, the check is fine if we set the stack before */
	if (check_crc(*kstack) && test_bit(ENABLED_BIT, kstack)) {
		read_pmc(1, dev->counter);
		enable_ibs_op(&dev->ibs);
	}
	else {
ibs_off:
		disable_ibs_op(&dev->ibs);
	}
}// threads_monitor_hook

static const struct file_operations hop_fops = {
	.open 		= hop_open,
	.owner 		= THIS_MODULE,
	.read 		= hop_read,
	// .poll	= hop_poll,
	.release 	= hop_release,
	.unlocked_ioctl = hop_ioctl
};

static const struct file_operations hop_ctl_fops = {
	.open 		= hop_ctl_open,
	.owner 		= THIS_MODULE,
	// .read 	= hop_ctl_read,
	.release 	= hop_ctl_release,
	.unlocked_ioctl = hop_ctl_ioctl
};

static int hop_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}// hop_uevent

static int create_pt_cdev(struct pt_info *pt)
{
	int err = 0;
	dev_t dev_nb;
	struct device *device;

	// dev_t should keep the numbers associated to the device (check it)
	dev_nb = MKDEV(hop_major, pt->mn->min);

	cdev_init(&pt->cdev, &hop_fops);
	pt->cdev.owner = THIS_MODULE;

	err = cdev_add(&pt->cdev, dev_nb, 1);
	if (err) {
		pr_err("MAKE DEVICE ERROR: %d while trying to add %s%d\n", err, HOP_DEVICE_NAME, pt->mn->min);
		return err;
	}

	device = device_create(hop_class, NULL, /* no parent device */ 
	dev_nb, NULL, /* no additional data */
	HOP_MODULE_NAME "/%d", pt->tid);

	if (IS_ERR(device)) {
		pr_err("MAKE DEVICE ERROR: %d while trying to create %s/%d\n", err, HOP_DEVICE_NAME, pt->tid);
		return PTR_ERR(device);
	}

	pr_info("MAKE DEVICE Device [%d %d] has been built\n", hop_major, pt->mn->min);

	return err;
}// create_pt_cdev

static void destroy_pt_cdev(struct pt_info *pt)
{
	// Even no all devices have been allocated we do not accidentlly delete other devices
	device_destroy(hop_class, MKDEV(hop_major, pt->mn->min));

	// If null it doesn't matter
	cdev_del(&pt->cdev);
}// destroy_pt_cdev

struct pt_info *lookup(pid_t tid)
{
	struct pt_info *pt = NULL;

	/* check if the the tid is already profiled */
	LCK_HASH;
	hash_for_each_possible(tid_htable, pt, node, tid)
		if(pt->tid == tid) goto found;
	goto none;
found:
	UCK_HASH;
	return pt;
none:
	UCK_HASH;
	return NULL;
}// lookup

static void internal_cleanup_thread_resources(struct pt_info *pt)
{
	if (!pt) goto end;

	/* chdev no longer active */
	destroy_pt_cdev(pt);

	put_minor((pt->mn));

	// TODO: check if the thread is alive
	while (test_and_set_bit(PROCESS_BIT, pt->ctl));
	/* NMI free, no one will touch the buffer */

	/* clear the kernel stack entry */
	// shall I do that?
	*pt->ctl = 1ULL << PROCESS_BIT;

	pr_info("cln stack ctl %lx", *pt->ctl);

	vfree(pt->page_htable);
	vfree(pt->dbuf->buf);
	vfree(pt->dbuf);
	vfree(pt);
end:
	return;
}// internal_cleanup_thread_resources

void cleanup_active_threads(void)
{
	/**
	 * these are used for the hastable scan
	 * bkt: used as scan index 
	 * next: used for temporary storage
	 * tmp_pt: used as scan cursor
	 */
	int bkt;
	struct hlist_node *next;
	struct pt_info *tmp_pt;
	
	LCK_HASH;
	hash_for_each_safe(tid_htable, bkt, next, tmp_pt, node) {

		pr_info("cleanup %u\n", tmp_pt->tid);
		/**
		 * this will delete the pt from the hastable and 
		 * free all the associated structs pt included 
		 */
		hash_del(&tmp_pt->node);
		internal_cleanup_thread_resources(tmp_pt);
	}
	UCK_HASH;
}// cleanup_active_threads

/**
 * This method print the pages accessed by the specified pid
 */
int thread_stats_page_access(pid_t tid, struct tid_page **pages)
{
	int bkt;
	// int err = 1;
	int pos = 0;
	struct pt_info *pt;
	struct pg_info *pg;

	pr_info("Printing status\n");

	/* check if the the tid is already profiled */
	LCK_HASH;
	hash_for_each_possible(tid_htable, pt, node, tid)
		if(pt->tid == tid) {
			
			pr_info("DEBUG address %llx", pages);

			*pages = vmalloc(pt->accessed_pages * sizeof(struct tid_page));

			pr_info("Tid %u found - allocated memory at %llx\n", tid, *pages);
			/* this is the expanded macro of 'hash_for_each_possible' */
			for ((bkt) = 0, pg = NULL; pg == NULL && (bkt) < (1ULL << pt->hash_bits); (bkt)++)
				hlist_for_each_entry(pg, &pt->page_htable[bkt], node) {

					//(*pages)[pos].page = pg->page;
					//(*pages)[pos].counter = pg->counter;
					pos++;
					// pr_info("[%llx] %llu\n", pages[pos-1].page, pages[pos-1].counter);
				}
		}
	UCK_HASH;

	return pos;
}// print_threads_stats

/* TODO: make a unique method */
int enable_profiler_thread(pid_t tid)
{
	int err = 1;
	struct pt_info *pt;

	/* check if the the tid is already profiled */
	LCK_HASH;
	hash_for_each_possible(tid_htable, pt, node, tid)
		if(pt->tid == tid) {
			set_bit(ENABLED_BIT, pt->ctl);
			err = 0;
			break;
		}
	UCK_HASH;

	return err;
}// enable_profiler_thread

int disable_profiler_thread(pid_t tid)
{
	int err = 1;
	struct pt_info *pt;

	/* check if the the tid is already profiled */
	LCK_HASH;
	hash_for_each_possible(tid_htable, pt, node, tid)
		if(pt->tid == tid) {
			clear_bit(ENABLED_BIT, pt->ctl);
			err = 0;
			break;
		}
	UCK_HASH;
	
	return err;
}// disable_profiler_thread

int setup_thread_resources(pid_t tid)
{
	int err = 0;
	// TODO: this must change in a concurrent structure
	struct pt_info *new_pt;
	struct pt_dbuf *new_dbuf;
	struct task_struct *tsk;
	unsigned long kstack;

	/* check if the the tid is already profiled */
	LCK_HASH;
	hash_for_each_possible(tid_htable, new_pt, node, tid)
		if(new_pt->tid == tid) {
			UCK_HASH;
			goto out;
		}
	UCK_HASH;

	pr_info("free %u\n", tid);

	new_pt = vmalloc(sizeof(struct pt_info));
	if (!new_pt) goto out;

	/* init dedicated buffer and build the info struct */
	new_dbuf = vmalloc(sizeof(struct pt_dbuf));
	if (!new_dbuf) goto buf_meta_err;

	new_dbuf->size = buff_size;
	new_dbuf->idx = 0;

	new_dbuf->buf = vzalloc(new_dbuf->size * sizeof(struct ibs_op));
	if (!new_dbuf->buf) goto buf_err;

	new_pt->tid = (int) tid;
	get_minor(new_pt->mn);
	/* there are no minors left */
	if (!new_pt->mn) {
		pr_warn("HOP module is full, cannot profile PT %u", tid);
		goto cdevs_fault;
	}
	
	new_pt->dbuf = new_dbuf;

	new_pt->busy = 0;
	new_pt->kernel = 0;
	new_pt->memory = 0;
	new_pt->samples = 0;
	new_pt->overwritten = 0;

	// this should ensure the initialization of the hlist
	new_pt->hash_bits = 12;
	new_pt->accessed_pages = 0;
	new_pt->page_htable = (struct hlist_head *) vzalloc((1<<12) * sizeof(struct hlist_head));

	pr_info("pt %u added", tid);
	
	if (create_pt_cdev(new_pt))
		goto cdevs_fault;

	/*
	 * get the last kernel stack address of the tid;
	 * point it with ctl inside new_node;
	 * write the stack position via ctl;
	 */

	tsk = get_pid_task(find_get_pid(tid), PIDTYPE_PID);
	if (!tsk) {
		err = -EINVAL;
		goto no_tid;
	}

	/* now the ctl is bound to the stack entry */
	new_pt->ctl = (unsigned long*) tid_thread_buf(tsk);

	/* the last kstack position contains the pt buffer addr */
	kstack = ((unsigned long) new_pt) & PTR_USED_MASK;
	/* set the crc */
	kstack |= ((kstack & PTR_CCB_MASK) ^ CRC_MAGIC) << 48;
	/* clear processing bit */
	kstack &= ~PROCESS_MASK;
	/* set enable bit */
	kstack |= ENABLED_MASK;
	/* visible to threads_monitor and NMI */
	*new_pt->ctl = kstack;

	pr_info("rebuilding test: original %lx - built %llx\n", (unsigned long) new_pt, build_ptr(kstack));

	pr_info("add stack ctl %llx", *(u64*)tid_thread_buf(tsk));

	LCK_HASH;
	hash_add(tid_htable, &new_pt->node, new_pt->tid);
	UCK_HASH;

	goto out;

no_tid:
	destroy_pt_cdev(new_pt);
cdevs_fault:
	vfree(new_dbuf->buf);
buf_err:
	vfree(new_dbuf);
buf_meta_err:
	vfree(new_pt);
out:
	return err;
}// setup_thread_resources

void cleanup_thread_resources(pid_t tid)
{
	struct pt_info *pt;
	struct hlist_node *next;
	LCK_HASH;
	hash_for_each_possible_safe(tid_htable, pt, next, node, tid)
		if (pt->tid == tid) {
			hash_del(&pt->node);
			break;
		}
	UCK_HASH;
	internal_cleanup_thread_resources(pt);
}// cleanup_thread_resources

static int setup_hop_resources(void)
{
	int err = 0;
	dev_t dev_nb;
	struct device *device;
	struct cdev *cdev = &hop_cdev;

	// dev_t should keep the numbers associated to the device (check it)
	dev_nb = MKDEV(hop_major, HOP_DEV_MINOR);

	cdev_init(cdev, &hop_ctl_fops);
	cdev->owner = THIS_MODULE;

	err = cdev_add(cdev, dev_nb, 1);
	if (err) {
		pr_err("MAKE DEVICE ERROR: %d while trying to add %s%d\n", err, HOP_DEVICE_NAME, HOP_DEV_MINOR);
		return err;
	}

	device = device_create(hop_class, NULL, /* no parent device */ 
	dev_nb, NULL, /* no additional data */
	HOP_MODULE_NAME "/%s", HOP_DEVICE_NAME);

	pr_devel("MAKE DEVICE Device [%d %d] has been built\n", hop_major, HOP_DEV_MINOR);

	if (IS_ERR(device)) {
		pr_err("MAKE DEVICE ERROR: %d while trying to create %s%d\n", err, HOP_DEVICE_NAME, HOP_DEV_MINOR);
		return PTR_ERR(device);
	}

	return err;
}// setup_hop_resources

static void cleanup_hop_resources(void)
{
	// Even no all devices have been allocated we do not accidentlly delete other devices
	device_destroy(hop_class, MKDEV(hop_major, HOP_DEV_MINOR));

	// If null it doesn't matter
	cdev_del(&hop_cdev);
}// cleanup_hop_resources

static int setup_chdevs_resources(void)
{
	int err = 0;
	dev_t dev = 0;

	int i;

	/* Get a range of minor numbers (starting with 0) to work with */
	err = alloc_chrdev_region(&dev, hop_major, HOP_MINORS, HOP_DEVICE_NAME);
	if (err < 0)
	{
		pr_err("alloc_chrdev_region() failed\n");        
		goto out;
	}

	hop_major = MAJOR(dev);

	/* Create device class (before allocation of the array of devices) */
	hop_class = class_create(THIS_MODULE, HOP_DEVICE_NAME);
	hop_class->dev_uevent = hop_uevent;

	/**
	 * The IS_ERR macro encodes a negative error number into a pointer, 
	 * while the PTR_ERR macro retrieves the error number from the pointer. 
	 * Both macros are defined in include/linux/err.h
	 */
	if (IS_ERR(hop_class))
	{
		pr_err("Class creation failed\n");
		err = PTR_ERR(hop_class);
		goto out_unregister_chrdev;
	}

	ms = vmalloc(NR_ALLOWED_TIDS * sizeof(struct minor));
	if (!ms) {
		err = -ENOMEM;
		goto out_class;
	}

	/* fill the minor list */
	for (i = 1; i < NR_ALLOWED_TIDS; ++i) {
		ms[i].min = i;
		put_minor((&ms[i]));
	}

	goto out;

out_class:
	class_destroy(hop_class);
out_unregister_chrdev:
	unregister_chrdev_region(MKDEV(hop_major, 0), HOP_MINORS);
out:
	return err;
}// setup_chdevs_resources

static void cleanup_chdevs_resources(void)
{
	vfree(ms);	/* free minors*/
	class_destroy(hop_class);
	unregister_chrdev_region(MKDEV(hop_major, 0), HOP_MINORS);
}// cleanup_chdevs_resources

static void setup_pmcs(void* dummy) {
	setup_pmc(1, PMC_CFG_BASE | RETIRED_UOPS);
}// setup_pmcs

static int setup_metadata(void)
{
	int err = 0;
	int cpu = 0;
	struct hop_dev *dev;

	spin_lock_init(&list_lock);
	spin_lock_init(&hash_lock);

	pcpu_hop_dev = alloc_percpu(struct hop_dev);
	if (!pcpu_hop_dev) {
		pr_err("Failed to allocate HOP metadata, exiting\n");
		err = -ENOMEM;
		goto out;
	}

	for_each_online_cpu(cpu) {
		dev = per_cpu_ptr(pcpu_hop_dev, cpu);
		dev->requests = 0;
		dev->spurious = 0;
		dev->no_ibs = 0;
		dev->denied = 0;
		dev->latency = 0;
		dev->ibs.cpu = cpu;
		dev->ibs.state = 0;		/* disabled */
		set_ibs_default(dev->ibs.ctl);
	}

	on_each_cpu(setup_pmcs, NULL, 1);
out:
	return err;
}// setup_metadata

static void cleanup_metadata(void)
{
	free_percpu(pcpu_hop_dev);
}// cleanup_metadata

int setup_resources(void)
{
	int err = 0;

	err = setup_metadata();
	if (err) goto out;
	
	err = setup_chdevs_resources();
	if (err) goto no_chdevs;
		
	err = setup_hop_resources();
	if (err) goto no_hop;

	goto out;
no_hop:
	cleanup_chdevs_resources();
no_chdevs:
	cleanup_metadata();
out:
	return err;
}// setup_resources

void cleanup_resources(void)
{
	/* cleanup the control device */
	cleanup_hop_resources();

	cleanup_active_threads();

	cleanup_chdevs_resources();
	cleanup_metadata();
}// cleanup_resources
