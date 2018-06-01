#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kprobes.h>

#include "ibs-control.h"
#include "hop-manager.h"
#include "hop-interrupt.h"
#include "hop-mod.h"

#define FUNC_NAME "finish_task_switch";

// static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
// {
// 	return 0;
// }// entry_handler

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	threads_monitor_hook();
	return 0;
}// ret_handler

static struct kretprobe hop_kretprobe = {
	.handler		= ret_handler,
	// .entry_handler		= entry_handler,
	//	.data_size		= sizeof(struct my_data),
	/* Probe up to 20 instances concurrently. */
	// .maxactive		= 20,
};

static int setup_scheduler_probe(void)
{
	int err;
	// Kprobe
	hop_kretprobe.kp.symbol_name = FUNC_NAME;
	err = register_kretprobe(&hop_kretprobe);
	if (err) 
		pr_err("register_kretprobe failed, returned %d\n", err);

	return err;
}// setup_scheduler_probe


static __init int hop_init(void)
{

	int err = 0;

	pr_info("Module Init\n");

	err = check_for_ibs_support();
	if (err) goto out;

#ifdef _IRQ
	pr_info("IRQ mode\n");
	err = setup_ibs_irq(handle_ibs_irq);
	if (err) goto out;
#else
	pr_info("NMI mode\n");
	err = setup_ibs_nmi(handle_ibs_nmi);
	if (err) goto out;
#endif

	err = setup_resources();
	if (err) goto no_res;

	err = setup_scheduler_probe();
	if (err) goto no_probe;

	goto out;

no_probe:
	cleanup_resources();
no_res:
#ifdef _IRQ
	cleanup_ibs_irq();
#else
	cleanup_ibs_nmi();
#endif
out:
	return err;
}// hop_init

void __exit hop_exit(void)
{
	// kprobe
	unregister_kretprobe(&hop_kretprobe);
	printk(KERN_INFO "kretprobe at %p unregistered\n",
			hop_kretprobe.kp.addr);

	/* nmissed > 0 suggests that maxactive was set too low. */
	printk(KERN_INFO "Missed probing %d instances of %s\n",
		hop_kretprobe.nmissed, hop_kretprobe.kp.symbol_name);

	cleanup_resources();

#ifdef _IRQ
	cleanup_ibs_irq();
#else
	cleanup_ibs_nmi();
#endif
	pr_info("Module Exit\n");
}// hop_exit

// Register these functions
module_init(hop_init);
module_exit(hop_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");
