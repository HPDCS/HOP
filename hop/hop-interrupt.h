/*
 * This file defines the interrupt handler
 */

#ifndef MEPO_INTERRUPT_H
#define MEPO_INTERRUPT_H

#ifdef _IRQ
void handle_ibs_irq(void);
#else
int handle_ibs_nmi(unsigned int cmd, struct pt_regs *regs);
#endif

#endif /* MEPO_INTERRUPT_H */
