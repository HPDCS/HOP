
#include <linux/slab.h> // memcpy

#include "lend.h"

void print_bytes(char *str, unsigned char byte, unsigned char *ptr, unsigned s, unsigned long add) {
	size_t i;

	printk(KERN_DEBUG "%s\t%02x: [%lx] ", str, byte, add);
	for(i = 0; i < s; i++)
		printk(KERN_CONT "%02x ", ptr[i]);
	printk(KERN_CONT "\n");
}

int diss(instr_t* v, unsigned long ptr, unsigned len)
{

	int i, size;
	void *temp = (void *)ptr;
	if (!v) return -1;

	for (i = 0; temp < ((void *)ptr + len); ++i)
	{
		size = length_disasm(temp, MODE_X64);
		v[i].size = size;
		v[i].ptr = (void*)((char*)(temp)); // ?
		memcpy(v[i].bytecode, (unsigned char*)(temp), size);
		temp = ((unsigned char*)(temp)) + size;
	}

	return i;
}// diss
