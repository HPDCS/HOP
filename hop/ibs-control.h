#ifndef IBS_CONTROL_H
#define IBS_CONTROL_H

#include "ibs-msr-index.h"
#include "hop-structs.h"

#define NMI_NAME	"ibs"
#define NMI_NAME_CK	"check_ibs"
#define DEFAULT_MAX_CNT	0x2000	// 8196
#define DEFAULT_MIN_CNT	0x90	// 144
/* may be changed in a per core version exploiting ibs_hw */
extern int sampling_frequency;		
extern int ibs_brn_trgt_supported;	// Keep ibs state information

#define register_set(val, len, off)	((val & len) << off)
#define register_get(reg, msk, off)	((val & msk) >> off)

/**
 * lfsr_random - 16-bit Linear Feedback Shift Register (LFSR)
 * LFSR from Paul Drongowski
 */
static inline unsigned int lfsr_random(void)
{
	static unsigned int lfsr_value = 0xF00D;
	unsigned int bit;

	/* Compute next bit to shift in */
	bit = ((lfsr_value >> 0) ^
		(lfsr_value >> 2) ^
		(lfsr_value >> 3) ^
		(lfsr_value >> 5)) & 0x0001;

	/* Advance to next register value */
	lfsr_value = (lfsr_value >> 1) | (bit << 15);

	return lfsr_value;
}// lfsr_random

/**
 * Setup IBS op ctl:
 * Default MAX, Random CUR, Instructions Counting, Enabled
 */
#define set_ibs_randomized(ctl) \
	ctl = (register_set(sampling_frequency, LEN_OP_MAX_CNT, OFF_OP_MAX_CNT) | \
		IBS_OP_CNT_CTL | IBS_OP_EN | register_set(lfsr_random(), \
			LEN_OP_CUR_CNT_RND, OFF_OP_CUR_CNT_RND))

/**
 * Setup IBS op ctl:
 * Default MAX, 0 CUR, Instructions Counting
 */
#define set_ibs_default(ctl) \
	ctl = (register_set(sampling_frequency, LEN_OP_MAX_CNT, OFF_OP_MAX_CNT) | \
		IBS_OP_CNT_CTL)

/**
 * Reset and active IBS for a new sampling cycle.
 * This is used in the NMI handler
 */
static inline void set_and_go_ibs_random(struct ibs_hw *ibs)
{
	set_ibs_randomized(ibs->ctl);
	wrmsrl(MSR_IBS_OP_CTL, ibs->ctl);
}// set_and_go_ibs_random

/**
 * Enable the IBS support on specific cpu.
 */
static inline void enable_ibs_op(struct ibs_hw *ibs)
{
	/* if already enabled return */
	if (test_and_set_bit(IBS_ACTIVE, &ibs->state)) return;

	// NOTE: logically, this should be out this function
	set_ibs_default(ibs->ctl);
	ibs->ctl |= IBS_OP_EN;
	wrmsrl(MSR_IBS_OP_CTL, ibs->ctl);
}// enable_ibs_op

/**
 * Disable the IBS support on specific cpu.
 * This function applies the Erratum 420 workaround.
 */
static inline void disable_ibs_op(struct ibs_hw *ibs)
{
	/* if already disabled return */
	if (!test_and_clear_bit(IBS_ACTIVE, &ibs->state)) return;

	ibs->ctl = (ibs->ctl | IBS_OP_VAL) & (~IBS_OP_MAX_CNT);
	wrmsrl(MSR_IBS_OP_CTL, ibs->ctl);
	ibs->ctl = 0ULL;
	wrmsrl(MSR_IBS_OP_CTL, ibs->ctl);
}// disable_ibs_op

int check_for_ibs_support(void);	/* check for IBS support */


#ifdef _IRQ
/* setup the lvt and register the nmi hanlder */
int setup_ibs_irq(void (*hop_handler) (void));

void cleanup_ibs_irq(void);
#else /* _NMI */
/* setup the lvt and register the nmi hanlder */
int setup_ibs_nmi(int (*handler) (unsigned int, struct pt_regs*));

void cleanup_ibs_nmi(void);
#endif

#endif /* IBS_CONTROL_H */
