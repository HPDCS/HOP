#ifndef IBS_CPUID_INDEX_H
#define IBS_CPUID_INDEX_H

#define CPUID_AMD_FAM10h				0x10

// This function returns IBS feature information
#define CPUID_Fn8000_001B_EAX 			cpuid_eax(0x8000001B)
#define 	IBS_CPUID_IBSFFV			1ULL		//  IBS feature flags valid.
#define 	IBS_CPUID_FetchSam			(1ULL<<1)	//  IBS fetch sampling supported.
#define 	IBS_CPUID_OpSam				(1ULL<<2)	//  IBS execution sampling supported.
#define 	IBS_CPUID_RdWrOpCnt			(1ULL<<3)	//  Read write of op counter supported. 
#define 	IBS_CPUID_OpCnt				(1ULL<<4)	//  Op counting mode supported.
#define 	IBS_CPUID_BrnTrgt			(1ULL<<5)	//  Branch target address reporting supported.
// Above specified in the BKDG Fam 10th. I should not care about the following
#define 	IBS_CPUID_OpCntExt			(1ULL<<6)	//  IbsOpCurCnt and IbsOpMaxCnt extend by 7 bits.
#define 	IBS_CPUID_RipInvalidChk			(1ULL<<7)	//  Invalid RIP indication supported.
#define 	IBS_CPUID_OpBrnFuse			(1ULL<<8)	//  Fused branch micro-op indication supported
// Above specified in the BKDG Fam 17th
#define 	IBS_CPUID_IbsFetchCtlExtd		(1ULL<<9)	//  IBS fetch control extended MSR supported.
#define 	IBS_CPUID_IbsOpData4			(1ULL<<10)	//  IBS op data 4 MSR supported.
// Above specified in the BKDG Fam 15h_30h-3Fh

// This function contains the following miscellaneous feature identifiers.
#define CPUID_Fn8000_0001_ECX 			cpuid_ecx(0x80000001)
#define 	IBS_SUPPORT_EXIST			(1ULL<<10)

#endif /* IBS_CPUID_INDEX_H */