/*
 * This file contains the MSR numbers, bits and masks for AMD IBS data.
 * All these information are taken from AMD BKDG Fam 10th (K10) and
 * in order to keep the relation with BKDG, I used the same name convetion.
 * IMPORTANT! This works only for processor K10 architecture based,
 * Compatibility with other architectures is not guaranteed.
 */

#ifndef IBS_MSR_INDEX_H
#define IBS_MSR_INDEX_H

/**
 * Bits and masks by register
 */
							// (IbsFetchCtl)
#define MSR_IBS_FETCH_CTL 				0xc0011030		// IBS Fetch Control Register
#define 	IBS_RAND_EN					(1ULL<<57)	// random instruction fetch tagging enable
#define 	IBS_L2_TLB_MISS					(1ULL<<56)	// instruction cache L2TLB miss
#define 	IBS_L1_TLB_MISS					(1ULL<<55)	// instruction cache L1TLB miss
#define 	IBS_L1_TLB_PG_SZ				(3ULL<<53)	// instruction cache L1TLB page size
#define 	IBS_PHY_ADDR_VALID				(1ULL<<52)	// instruction fetch physical address valid
#define 	IBS_IC_MISS					(1ULL<<51)	// instruction cache miss
#define 	IBS_FETCH_COMP					(1ULL<<50)	// instruction fetch complete
#define 	IBS_FETCH_VAL					(1ULL<<49)	// instruction fetch valid
#define 	IBS_FETCH_EN					(1ULL<<48)	// instruction fetch enable
#define 	IBS_FETCH_LAT					(0xffffULL<<32)	// instruction fetch latency
#define 	IBS_FETCH_CNT					(0xffffULL<<16)	// periodic fetch counter count
#define 	IBS_FETCH_MAX_CNT				0xffffULL 	// periodic fetch counter maximum count

							// (IbsFetchLinAd)
#define MSR_IBS_FETCH_LIN_AD				0xc0011031		// IBS Fetch Linear Address Register
#define 	IBS_FETCH_LIN_AD				(~0ULL)		// instruction fetch linear address

							// (IbsFetchPhysAd)
#define MSR_IBS_FETCH_PHYS_AD				0xc0011032		// IBS Fetch Physical Address Register
#define 	IBS_FETCH_PHYS_AD				(~0ULL)		// instruction fetch physical address

							// (IbsOpCtl)
#define MSR_IBS_OP_CTL					0xc0011033		// IBS Execution Control Register
/* Optimization for register set */
#define 	OFF_OP_CUR_CNT					32
#define 	LEN_OP_CUR_CNT					0XfffffULL
#define		IBS_OP_CUR_CNT					(0XfffffULL<<32)// periodic op counter current count
/* Optimization for register set */
#define 	OFF_OP_CUR_CNT_RND				32
#define 	LEN_OP_CUR_CNT_RND				0XfULL
/* Alternate mask excludes bits that are randomized by software */
// Note: Read BKDG 10th - pag 440
#define 	IBS_OP_CNT_CTL					(1ULL<<19)	// periodic op counter count control
#define		IBS_OP_VAL					(1ULL<<18)	// micro-op sample valid
#define		IBS_OP_EN					(1ULL<<17)	// micro-op sampling enable
/* Optimization for register set */
#define 	OFF_OP_MAX_CNT					0
#define 	LEN_OP_MAX_CNT					0XffffULL
#define		IBS_OP_MAX_CNT					0xffffULL	// periodic op counter maximum count

							// (IbsOpRip)
#define MSR_IBS_OP_RIP					0xc0011034		// IBS Op Logical Address Register
#define 	IBS_OP_RIP					(~0ULL)		// micro-op linear address

							// (IbsOpData)
#define MSR_IBS_OP_DATA					0xc0011035		// IBS Op Data Register 
#define 	IBS_OP_BRN_RET					(1ULL<<37)	// branch micro-op retired
#define 	IBS_OP_BRN_MISP					(1ULL<<36)	// mispredicted branch micro-op
#define 	IBS_OP_BRN_TAKEN				(1ULL<<35)	// taken branch micro-op
#define 	IBS_OP_RETURN					(1ULL<<34)	// return micro-op
#define		IBS_OP_MISP_RETURN				(1ULL<<33)	// mispredicted return micro-op
#define		IBS_OP_BRN_RESYNC				(1ULL<<32)	// resync micro-op
#define 	IBS_TAG_TO_RET_CTR				(0xffffULL<<16) // micro-op tag to retire count
#define 	IBS_COMP_TO_RET_CTR				0xffffULL 	// micro-op completion to retire count

							// (IbsOpData2)
#define MSR_IBS_OP_DATA2				0xc0011036		// IBS Op Data 2 Register
#define 	NB_IBS_REQ_CACHE_HIT_ST				(1ULL<<5)	// IBS L3 cache state
#define 	NB_IBS_REQ_DST_PROC				(1ULL<<4)	// IBS request destination processor
#define 	NB_IBS_REQ_SRC					7ULL		// Northbridge IBS request data source

							// (IbsOpData3)
#define MSR_IBS_OP_DATA3				0xc0011037		// IBS Op Data 3 Register 
#define 	IBS_DC_MISS_LAT					(0xffffULL<<32)	// data cache miss latency
#define 	IBS_DC_L2_TLB_HIT_1G				(1ULL<<19)	// data cache L2TLB hit in 1G page
#define 	IBS_DC_PHY_ADDR_VALID				(1ULL<<18)	// data cache physical address valid
#define 	IBS_DC_LIN_ADDR_VALID				(1ULL<<17)	// data cache linear address valid
#define 	IBS_DC_MAB_HIT					(1ULL<<16) 	// MAB hit (?)
#define 	IBS_DC_LOCKED_OP				(1ULL<<15)	// locked operation
#define 	IBS_DC_UC_MEM_ACC				(1ULL<<14)	// UC memory access
#define 	IBS_DC_WC_MEM_ACC				(1ULL<<13)	// WC memory access
#define		IBS_DC_ST_TO_LD_CAN				(1ULL<<12) 	// data forwarding from store to load operation cancelled
#define		IBS_DC_ST_TO_LD_FWD				(1ULL<<11)	// data forwarded from store to load operation
#define		IBS_DC_ST_BNK_CON				(1ULL<<10)	// bank conflict on store operation
#define		IBS_DC_LD_BNK_CON				(1ULL<<9) 	// bank conflict on load operation
#define 	IBS_DC_MIS_ACC					(1ULL<<8)	// misaligned access
#define 	IBS_DC_MISS					(1ULL<<7)	// data cache miss
#define 	IBS_DC_L2_TLB_HIT_2M				(1ULL<<6)	// data cache L2TLB hit in 2M page
#define 	IBS_DC_L1_TLB_HIT_1G				(1ULL<<5)	// data cache L1TLB hit in 1G page
#define 	IBS_DC_L1_TLB_HIT_2M				(1ULL<<4)	// data cache L1TLB hit in 2M page
#define 	IBS_DC_L2_TLB_MISS				(1ULL<<3)	// data cache L2TLB miss
#define 	IBS_DC_L1_TLB_MISS				(1ULL<<2)	// data cache L1TLB miss
#define 	IBS_ST_OP					(1ULL<<1)	// store op
#define 	IBS_LD_OP					1ULL		// load op

							// (IbsDcLinAd)
#define MSR_IBS_DC_LIN_AD				0xc0011038		// IBS DC Linear Address Register 
#define 	IBS_DC_LIN_AD					(~0ULL)		// linear address for the tagged l/s

							// (IbsDcPhysAd)
#define MSR_IBS_DC_PHYS_AD				0xc0011039		// IBS DC Physical Address Register
#define 	IBS_DC_PHYS_AD					(~0ULL)		// physical address for the tagged l/s				

#define MSR_IBS_CONTROL					0xc001103a		// IBS Control Register
#define 	IBS_LVT_OFFSET_VAL				(1ULL<<8)	// local vector table offset valid
#define 	IBS_LVT_OFFSET					0xfULL 		// local vector table offset

#define IBS_CONTROL 						0x1cc		// Real copy of MSR_IBS_CONTROL
#define IBS_LVT_NR						0xf 		// LVT entry number (4 bits) - BKDG

#endif	/* IBS_MSR_INDEX_H */
