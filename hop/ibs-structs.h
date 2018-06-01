/**
 * This file contains the structs used to communicate IBS samples from the
 * driver into user-space.
 */

#ifndef IBS_STRUCTS_H
#define IBS_STRUCTS_H

#include <linux/types.h>

#ifndef u64
#define u64 uint64_t
#endif
#ifndef u32
#define u32 uint32_t
#endif
#ifndef u16
#define u16 uint16_t
#endif
#ifndef u8
#define u8 uint8_t
#endif

/**
 * This file keeps the name convention used by AMD.
 * You can find all this information inside the BKDG.
 * Remember this file is defined for AMD Fam 10th,
 * some structs/fields could not be compatible with
 * other families products.
 */
struct ibs_op {

	/* used to tell if the sample is valid */
	volatile unsigned v;

	/**
 	 * IbsOpCtl: IBS Execution Control Register
 	 * MSRC001_1033
	 */
	union
	{
		u64 value;
		struct
		{
			// periodic op counter current count
			u16 		ibs_op_max_cnt 		: 	16;

			unsigned 	reserved0		:	1;
			// micro-op sampling enable
			unsigned	ibs_op_en		:	1;
			// micro-op sample valid
			unsigned	ibs_op_val		:	1;
			// periodic op counter count count
			unsigned	ibs_op_cnt_ctl		:	1;

			u16 		reserved1		:	12;
			// periodic op counter current control			
			u32 		ibs_op_cur_cnt		:	20;

			u16 		reserved2		:	12;	
		} reg;
	} ibs_op_ctl;

	/*
 	 * IbsOpRip: IBS Op Logical Address Register
 	 * MSRC001_1034
	 */
	union
	{
		u64 value;
		struct
		{
			// micro-op linear address
			u64 		ibs_op_rip		: 	64;
		} reg;
	} ibs_op_rip;

	/*
 	 * IbsOpData: IBS Op Data Register
 	 * MSRC001_1035
	 */
	union
	{
		u64 value;
		struct
		{
			// micro-op completion to retire count
			u16 		ibs_comp_to_ret_ctr	:	16;
			// micro-op tag to retire count
			u16 		ibs_tag_to_ret_ctr	:	16;
			// resync micro-op
			unsigned	ibs_op_brn_resync	:	1;
			// mispredicted return micro-op
			unsigned 	ibs_op_misp_return	:	1;
			// return micro-op
			unsigned	ibs_op_return 		:	1;
			// taken branch micro-op
			unsigned	ibs_op_brn_taken	:	1;
			// mispredicted branch micro-op
			unsigned	ibs_op_brn_misp		:	1;
			// branch micro-op retired
			unsigned	ibs_op_brn_ret		:	1;

			u32 		reserved0		:	16;
		} reg;
	} ibs_op_data;

	/*
 	 * IbsOpData: IBS Op Data 2 Register
 	 * MSRC001_1036
	 */
	union
	{
		u64 value;
		struct
		{
			// Northbridge IBS request data source
			unsigned	nb_ibs_req_src		:	2;
			
			unsigned	reserved0		:	1;
			// IBS request destination processor
			unsigned	nb_ibs_req_dst_proc	:	1;
			// IBS L3 cache state
			unsigned	nb_ibs_req_cache_hit_st	:	1;

			u64 		reserved1		:	58;
		} reg;
	} ibs_op_data2;

	/*
 	 * IbsOpData: IBS Op Data 3 Register
 	 * MSRC001_1037
	 */
	union
	{
		u64 value;
		struct
		{
			// load op
			unsigned	ibs_ld_op		:	1;
			// stroe op
			unsigned	ibs_st_op		:	1;
			// data cache L1TLB miss
			unsigned	ibs_dc_l1tlb_miss	:	1;
			// data cache L2TLB miss
			unsigned	ibs_dc_l2tlb_miss	:	1;
			// data cache L1TLB hit in 2M page
			unsigned	ibs_dc_l1tlb_hit_2m	:	1;
			// data cache L1TLB hit in 1G page
			unsigned	ibs_dc_l1tlb_hit_1g	:	1;
			// data cache L2TLB hit in 2M page
			unsigned	ibs_dc_l2tlb_hit_2m	:	1;
			// data cache miss
			unsigned	ibs_dc_miss		:	1;
			// misaligned access
			unsigned	ibs_dc_mis_acc		:	1;
			// bank conflict on load operation
			unsigned	ibs_dc_ld_bnk_con	:	1;
			// bank conflict on store operation
			unsigned	ibs_dc_st_bnk_con	:	1;
			// data forwarded from store to load operation
			unsigned	ibs_dc_st_to_ld_fwd	:	1;
			// data forwarding from store to load operation canc.
			unsigned	ibs_dc_st_to_ld_can	:	1;
			// WC memory access
			unsigned	ibs_dc_wc_mem_acc	:	1;
			// UC memory access
			unsigned	ibs_dc_uc_mem_acc	:	1;
			// locked operation
			unsigned	ibs_dc_locked_op	:	1;
			// MAB hit
			unsigned	ibs_dc_mab_hit		:	1;
			// data cache linear address valid
			unsigned	ibs_dc_lin_addr_valid	:	1;
			// data cache physical address valid
			unsigned	ibs_dc_phy_addr_valid	:	1;
			// data cache L2TLB hit in 1G page
			unsigned	ibs_dc_l2tlb_hit_1g	:	1;

			u16		reserved0		:	12;
			// data cache miss latency
			u16 		ibs_dc_miss_lat		:	16;

			u16 		reserved1		:	16;
		} reg;
	} ibs_op_data3;

	/*
 	 * IbsDcLinAd: DC Linear Address Register
 	 * MSRC001_1038
	 */
	union
	{
		u64 value;
		struct
		{
			
			u64 		ibs_dc_lin_ad		: 	64;
		} reg;
	} ibs_dc_lin_ad;

	/*
 	 * IbsDcPhysAd: DC Physical Address Register
 	 * MSRC001_1039
	 */
	union
	{
		u64 value;
		struct
		{
			
			u64 		ibs_dc_phys_ad		: 	64;
		} reg;
	} ibs_dc_phys_ad;

	u64		tsc;
	pid_t		tid;
	pid_t		pid;
	unsigned	cpu;
	unsigned	kernel;
};

#endif	/* IBS_STRUCTS_H */
