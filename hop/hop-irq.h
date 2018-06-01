#ifndef HOP_IRQ_H
#define HOP_IRQ_H

typedef void (*hop_handler)(struct pt_regs *);

#define TEXT_SIZE       128

/* IBS_VECTOR takes the last available slot (238) */
#define IBS_VECTOR      LOCAL_TIMER_VECTOR - 1
/* this sequence identifies the 'push ~239' */
#define PUSH0           0x68
#define PUSH1           0x10
#define PUSH2           0xff
#define PUSH3           0xff
#define PUSH4           0xff
#define OP_SIZE	        5
/* push the ~IBS_VECTOR number */
#define PUSH_FIX		0x11
#define CALL_OP         0xe8
#define JUMP_OP         0xe9

/* both the CALL and the JUMP (relative) 32bit operand consider the RIP of the 
 * next instruction to sum the address to */
#define JUMP_AL         5
#define CALL_AL	        5

#define NOOP16() do { \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
		asm volatile ("nop" : : ); \
	} while(0)

#endif /* HOP_IRQ_H */
