#include "pmc-msr-index.h"

#define PMC_CFG_BASE	0x430000ULL
#define PMC_CFG_USR	0x410000ULL
#define PMC_CFG_OS	0x420000ULL

#define print_pmc(nb, tmp)						\
({									\
	rdmsrl(PERF_CTR##nb, tmp);					\
	pr_info("PERF_CTR %u: %llu\n", nb, tmp);			\
	rdmsrl(PERF_EVT_SEL##nb, tmp);					\
	pr_info("PERF_EVT_SEL %u: %02llx|%02llx|%02llx|%02llx\n",	\
		nb,							\
		(tmp>>56) & (0xffU),					\
		(tmp>>48) & (0xffU),					\
		(tmp>>40) & (0xffU),					\
		(tmp>>32) & (0XFFu));					\
	pr_info("             %02llx|%02llx|%02llx|%02llx\n",		\
		(tmp>>24) & (0xffU), (tmp>>16) & (0xffU),		\
		(tmp>>8) & (0xffU), (tmp) & (0xffU));			\
})

#define read_pmc(nb, tmp)		\
({					\
	rdmsrl(PERF_CTR##nb, tmp);	\
})

#define reset_pmc(nb, tmp)		\
({					\
	wrmsrl(PERF_CTR##nb, tmp);	\
	wrmsrl(PERF_EVT_SEL##nb, tmp);	\
})

/* setup the pmc ctr and reset the evt_sel */
#define setup_pmc(nb, tmp)		\
({					\
	wrmsrl(PERF_CTR##nb, 0ULL);	\
	wrmsrl(PERF_EVT_SEL##nb, tmp);	\
})

#define stop_and_save_pmc(nb, save)	\
({					\
	rdmsrl(PERF_CTR##nb, save);	\
	wrmsrl(PERF_EVT_SEL##nb, 0ULL);	\
})

#define restart_pmc(nb, tmp, save)	\
({					\
	wrmsrl(PERF_CTR##nb, save);	\
	wrmsrl(PERF_EVT_SEL##nb, tmp);	\
})

#define write2_pmc(nb, ctr, sel)	\
({					\
	wrmsrl(PERF_CTR##nb, ctr);	\
	wrmsrl(PERF_EVT_SEL##nb, sel);	\
})	

#define NEVERAJOY NULL