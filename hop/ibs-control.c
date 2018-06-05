#include <asm/apicdef.h>
#include <asm/apic.h>
#include <asm/nmi.h>
#include <linux/slab.h>
#include <asm/desc.h>
#include <asm/desc_defs.h>

#include <asm/irq.h>		// NR_IRQS
#include <asm/hw_irq.h>

#include <linux/interrupt.h>	// set_irq_regs

#include <linux/percpu.h>
#include <linux/gfp.h>		// alloc_kmem_pages_node

#include "ibs-control.h"
#include "ibs-cpuid-index.h"
#include "hop-structs.h"
#include "hop-interrupt.h"
#include "hop-mod.h"

#include <asm/cacheflush.h>	// set_memory_nx
#include <asm/topology.h>	// cpu_to_node


unsigned ibs_vector = 0xffU;

int ibs_op_supported;
int ibs_fetch_supported;
int ibs_brn_trgt_supported;
int sampling_frequency;

gate_desc old_ibs;

// This function has been taken from AMD Research IBS Toolkit
// This is limited to the 10th fam of cpus.
int check_for_ibs_support(void)
{
	unsigned int feature_id;

	/* Must be on an AMD CPU */
	struct cpuinfo_x86 *c = &boot_cpu_data;

	if (c->x86_vendor != X86_VENDOR_AMD)
	{
		pr_err("Required AMD processor.\n");
		return -EINVAL;
	}

	if (c->x86 != CPUID_AMD_FAM10h)
	{
		pr_err("This module is designed only for Fam 10h.\n");
		return -EINVAL;
	}

	// See AMD CPUID Specification Manual
	if (! (CPUID_Fn8000_0001_ECX & IBS_SUPPORT_EXIST))
	{
		pr_err("CPUID_Fn8000_0001 indicates no IBS support.\n");
		return -EINVAL;	
	}

	/* If we are here, time to check the IBS capability flags for
	 * what, if anything, is supported. */
	feature_id = CPUID_Fn8000_001B_EAX;

	if (! (feature_id & IBS_CPUID_IBSFFV))
	{
		pr_err("CPUID_Fn8000_001B indicates no IBS support.\n");
		return -EINVAL;
	}

	// Assuming that all the CPUs are the same. (Numa context).

	
	/* Now check support for Op or Fetch sampling. If neither, die. */
	ibs_fetch_supported = feature_id & IBS_CPUID_FetchSam;
	/* Op count is more complicated. We want all of its features in this
	 * driver, so or them all together */
	ibs_op_supported = feature_id & IBS_CPUID_OpSam;
	ibs_op_supported |= feature_id & IBS_CPUID_RdWrOpCnt;
	ibs_op_supported |= feature_id & IBS_CPUID_OpCnt;

	if (!ibs_fetch_supported)
	{
		pr_err("CPUID_Fn800_001B says no Op support.\n");
		return -EINVAL;
	}
	if (!ibs_op_supported)
	{
		pr_err("CPUID_Fn800_001B says no Fetch support.\n");
		return -EINVAL;
	}

	/* Now to set all the other feature flags */
	ibs_brn_trgt_supported = feature_id & IBS_CPUID_BrnTrgt;
	if (!ibs_brn_trgt_supported)
		pr_warn("CPUID_Fn800_001B says no Branch target support.\n");

	/* set default sampling frequency */
	sampling_frequency = (DEFAULT_MAX_CNT >> 4);
	
	return 0;
} // check_for_ibs_support

/*
 * setup the Local APIC lvt and register IBS support
 * to generate an NMI. Note tha this function must be 
 * invoked with interrupts disabled. on_each_cpu macro
 * does this for you. Furthermore, the setup_APIC_eilvt
 * function always checks on each CPU Local APIC to keep
 * consistency among cores. (All LVTs are equals at the 
 * same offset)
 */
static void setup_ibs_lvt(void *err)
{
	u64 reg;
	u64 ibs_ctl;
	u32 entry;
	u32 new_entry;
	u8 offset;

	/*
	 * The LAPIC entry is set by the BIOS and reserves the 
	 * offset specified by the IBS_CTL register for 
	 * Non Maskable Interrupt (NMI). This entry should be 
	 * masked.
	 */

	/* Get the IBS_LAPIC offset by IBS_CTL */
	rdmsrl(MSR_IBS_CONTROL, ibs_ctl);
	if (!(ibs_ctl & IBS_LVT_OFFSET_VAL)) {
		pr_err("APIC setup failed: invalid offset by MSR_bits: %llu\n", ibs_ctl);
		goto no_offset;
	}

	offset = ibs_ctl & IBS_LVT_OFFSET;
	pr_info("IBS_CTL Offset: %u\n", offset);

	reg = APIC_EILVTn(offset);
	entry = apic_read(reg);

	/* print the apic register : | mask | msg_type | vector | before setup */
	pr_info("[APIC] CPU %u - READ offset %u -> | %lu | %lu | %lu |\n", 
		smp_processor_id(), 
		offset, ((entry >> 16) & 0xFUL), ((entry >> 8) & 0xFUL), (entry & 0xFFUL));

	// if different, clear


	// This is the entry we want to install, ibs_irq_line has been got in the step before
	#ifdef _IRQ
	new_entry = (0UL) | (APIC_EILVT_MSG_FIX << 8) | (ibs_vector);
	#else
	new_entry = (0UL) | (APIC_EILVT_MSG_NMI << 8) | (0);
	#endif

	// If not masked, remove it
	if (entry != new_entry || !((entry >> 16) & 0xFUL) ) {
		if (!setup_APIC_eilvt(offset, 0, 0, 1)) {
			pr_info("Cleared LVT entry #%i on cpu:%i\n", 
				offset, smp_processor_id());
			reg = APIC_EILVTn(offset);
			entry = apic_read(reg);
		} else {
			goto fail;
		}
	}

	#ifdef _IRQ
	if (!setup_APIC_eilvt(offset, ibs_vector, APIC_EILVT_MSG_FIX, 0))
	#else
	if (!setup_APIC_eilvt(offset, 0, APIC_EILVT_MSG_NMI, 0))
	#endif
		pr_info("LVT entry #%i setup on cpu:%i\n", 
			offset, smp_processor_id());
	else
		goto fail;

	return;

fail:
	pr_err("APIC setup failed: cannot set up the LVT entry \
		#%u on CPU: %u\n", offset, smp_processor_id());
no_offset:
	*(int*)err = -1;
} // setup_ibs_lvt

#ifdef _IRQ
extern void ibs_entry(void);
asm("    .globl ibs_entry\n"
    "ibs_entry:\n"
    "    cld\n"
    "    testq $3,8(%rsp)\n"
    "    jz    1f\n"
    "    swapgs\n"
    "1:\n"
    "    pushq $0\n" /* error code */
    "    pushq %rdi\n"
    "    pushq %rsi\n"
    "    pushq %rdx\n"
    "    pushq %rcx\n"
    "    pushq %rax\n"
    "    pushq %r8\n"
    "    pushq %r9\n"
    "    pushq %r10\n"
    "    pushq %r11\n"
    "    pushq %rbx\n"
    "    pushq %rbp\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "1:  call handle_ibs_irq\n"
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %rbp\n"
    "    popq %rbx\n"
    "    popq %r11\n"
    "    popq %r10\n"
    "    popq %r9\n"
    "    popq %r8\n"
    "    popq %rax\n"
    "    popq %rcx\n"
    "    popq %rdx\n"
    "    popq %rsi\n"
    "    popq %rdi\n"
    "    addq $8,%rsp\n" /* error code */
    "    testq $3,8(%rsp)\n"
    "    jz 2f\n"
    "    swapgs\n"
    "2:\n"
    "    iretq");

/**
 * the way we patch the idt is not the best. We need to tackle the SMP system
 * and find a way to patch the idt without locking the whole system for a while.
 * Furthemore, we have to be sure that each cpu idtr is pointing to the 
 * patched idt */
static int setup_idt_entry (void)
{
	
	struct desc_ptr idtr;
	gate_desc ibs_desc;
	unsigned long cr0;

	/* read the idtr register */
	store_idt(&idtr);

	/* copy the old entry before overwritting it */
	memcpy(&old_ibs, (void*)(idtr.address + ibs_vector * sizeof(gate_desc)), sizeof(gate_desc));
	
	pack_gate(&ibs_desc, GATE_INTERRUPT, (unsigned long)ibs_entry, 0, 0, 0);
	
	/* the IDT id read only */
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);

	write_idt_entry((gate_desc*)idtr.address, ibs_vector, &ibs_desc);
	
	/* restore the Write Protection BIT */
	write_cr0(cr0);	
	// on_each_cpu(install_idt_numa, NULL, 1);

	return 0;
}// setup_idt

static int acquire_free_vector(void)
{
	while (test_bit(ibs_vector, used_vectors)) {
		if (ibs_vector == 0x40) {
			pr_err("No free vector found\n");
			return -1;
		}
		ibs_vector--;
	}
	set_bit(ibs_vector, used_vectors);
	pr_info("Got vector #%x\n", ibs_vector);
	return 0;
}

int setup_ibs_irq(void (*handler) (void))
{
	int err = 0;
	
	// err = acquire_free_vector();
	// if (err) goto out;

	ibs_vector = 0x2;

	err = setup_idt_entry();
	if (err) goto out;
	
	on_each_cpu(setup_ibs_lvt, &err, 1);
	if (!err) goto out;
	
out:
	return err;
}// setup_ibs_irq

static void mask_ibs_lvt(void *err)
{
	u64 reg;
	u64 ibs_ctl;
	u32 entry;
	u8 offset;

	/* Get the IBS_LAPIC offset by IBS_CTL */
	rdmsrl(MSR_IBS_CONTROL, ibs_ctl);

	offset = ibs_ctl & IBS_LVT_OFFSET;

	reg = APIC_EILVTn(offset);
	entry = apic_read(reg);

	if (setup_APIC_eilvt(offset, entry & 0xFFUL, (entry >> 8) & 0xFUL, 1UL))
		goto fail;

	return;

fail:
	*(int*)err = -1;
	pr_err("APIC setup failed: cannot mask the LVT entry \
		#%u on CPU: %u\n", offset, smp_processor_id());
}// mask_ibs_lvt

static void restore_idt(void)
{
	struct desc_ptr idtr;
	unsigned long cr0;

	/* read the idtr register */
	store_idt(&idtr);

	/* the IDT id read only */
	cr0 = read_cr0();
	write_cr0(cr0 & ~X86_CR0_WP);

	write_idt_entry((gate_desc*)idtr.address, ibs_vector, &old_ibs);
	
	/* restore the Write Protection BIT */
	write_cr0(cr0);	
	// on_each_cpu(install_idt_numa, NULL, 1);
}// restore_idt

void cleanup_ibs_irq(void)
{
	int err = 0;
	// on_each_cpu(mask_ibs_lvt, &err, 1);
	if (err)
		pr_err("Error while masking the LVT, trying to restore the IDT\n");
	// restore_idt();
	/* release the ibs vector */
	// clear_bit(ibs_vector, used_vectors);	
	
	pr_info("IRQ cleaned\n");
}

#else /* NMI */

int setup_ibs_nmi(int (*handler) (unsigned int, struct pt_regs*))
{
	int err = 0;
	static struct nmiaction handler_na;

	/* look at the gate_desc 0x2 */
	struct desc_ptr idtr;
	gate_desc *desc;

	/* read the idtr register */
	store_idt(&idtr);
	desc = (gate_desc*) idtr.address;
	desc += 2;

	pr_info("NMI handler at: %llx", ((unsigned long long)(desc->offset_high) << 32) | 
		(desc->offset_middle << 16) | desc->offset_low);

	on_each_cpu(setup_ibs_lvt, &err, 1);
	if (err) goto out;

	// register_nmi_handler(NMI_LOCAL, handler, NMI_FLAG_FIRST, NMI_NAME);
	handler_na.handler = handler;
	handler_na.name = NMI_NAME;
	/* this is a Local CPU NMI, thus must be processed before others */
	handler_na.flags = NMI_FLAG_FIRST;
	err = __register_nmi_handler(NMI_LOCAL, &handler_na);

out:
	return err;
}// setup_ibs_nmi

void cleanup_ibs_nmi(void)
{
	unregister_nmi_handler(NMI_LOCAL, NMI_NAME);
}// cleanup_ibs_nmi
#endif
