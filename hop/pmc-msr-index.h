#ifndef PMC_MSR_INDEX
#define PMC_MSR_INDEX

#define PERF_EVT_SEL0					0xc0010000		// PES0 control register 
#define PERF_CTR0					0xc0010004		// PES0 value register
#define PERF_EVT_SEL1					0xc0010001		// PES1 control register 
#define PERF_CTR1					0xc0010005		// PES1 value register
#define PERF_EVT_SEL2					0xc0010002		// PES2 control register 
#define PERF_CTR2					0xc0010006		// PES2 value register
#define PERF_EVT_SEL3					0xc0010003		// PES3 control register 
#define PERF_CTR3					0xc0010007		// PES3 value register

#define		OFF_HG_ONLY					39	
#define		LEN_HG_ONLY					0X3ULL
#define		HG_ONLY						(0X3ULL<<39)	// periodic op counter current count

#define		OFF_EVENT_SELECT				32
#define		LEN_EVENT_SELECT				0XfULL
#define		EVENT_SELECT					(0XfULL<<32)	// periodic op counter current count

#define		OFF_CT_MASK					23
#define		LEN_CT_MASK					0XffULL
#define		CT_MASK						(0XffULL<<23)	// periodic op counter current count

#define		INV						(1ULL<<23)
#define		EN						(1ULL<<22)
#define		INT						(1ULL<<20)
#define		EDGE						(1ULL<<18)
#define		OS						(1ULL<<17)
#define		USR						(1ULL<<17)

#define		OFF_UNIT_MASK					7
#define		LEN_UNIT_MASK					0XffULL
#define		UNIT_MASK					(0XffULL<<7)	// periodic op counter current count

#define		OFF_EVENT_SELECT_L				7
#define		LEN_EVENT_SELECT_L				0XffULL
#define		EVENT_SELECT_L					(0XffULL<<7)	// periodic op counter current count


// EventSelect
#define 	CPU_CLOCK_NOT_HALTED				0x76
#define 	RETIRED_INSTRCUTIONS				0xc0
#define 	RETIRED_UOPS					0xc1

#endif

// page 444, cap 3.14 BKDG
