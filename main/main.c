#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "hop-ioctl.h"

#define EXIT	0
#define IOCTL 	1

int int_from_stdin()
{
	char buffer[12];
	int i = -1;

	if (fgets(buffer, sizeof(buffer), stdin)) {
		sscanf(buffer, "%d", &i);
	}

	return i;
}// int_from_stdin

int ioctl_cmd(int fd)
{
	
	printf("%d: set HOP_PROFILER_ON\n", _IOC_NR(HOP_PROFILER_ON));
	printf("%d: set HOP_PROFILER_OFF\n", _IOC_NR(HOP_PROFILER_OFF));
	printf("%d: set HOP_DEBUGGER_ON\n", _IOC_NR(HOP_DEBUGGER_ON));
	printf("%d: set HOP_DEBUGGER_OFF\n", _IOC_NR(HOP_DEBUGGER_OFF));
	printf("%d: set HOP_CLEAN_TIDS\n", _IOC_NR(HOP_CLEAN_TIDS));
	printf("%d: set HOP_ADD_TID\n", _IOC_NR(HOP_ADD_TID));
	printf("%d: set HOP_DEL_TID\n", _IOC_NR(HOP_DEL_TID));
	printf("%d: set HOP_START_TID\n", _IOC_NR(HOP_START_TID));
	printf("%d: set HOP_STOP_TID\n", _IOC_NR(HOP_STOP_TID));
	printf("%d: set HOP_SET_BUF_SIZE\n", _IOC_NR(HOP_SET_BUF_SIZE));
	printf("%d: set HOP_SET_SAMPLING\n", _IOC_NR(HOP_SET_SAMPLING));

	printf("Put cmd >> ");
	int cmd = int_from_stdin();

	int err = 0;
	int var = 0;
	unsigned long uvar = -1;


	switch(cmd)
	{
	case _IOC_NR(HOP_PROFILER_ON) : 
		if (!(err = ioctl(fd, HOP_PROFILER_ON)))
			printf("IOCTL: HOP_PROFILER_ON\n");
		break;
	case _IOC_NR(HOP_PROFILER_OFF) : 
		if (!(err = ioctl(fd, HOP_PROFILER_OFF)))
			printf("IOCTL: HOP_PROFILER_OFF\n");
		break;
	case _IOC_NR(HOP_DEBUGGER_ON) : 
		if (!(err = ioctl(fd, HOP_DEBUGGER_ON)))
			printf("IOCTL: HOP_DEBUGGER_ON\n");
		break;
	case _IOC_NR(HOP_DEBUGGER_OFF) : 
		if (!(err = ioctl(fd, HOP_DEBUGGER_OFF)))
			printf("IOCTL: HOP_DEBUGGER_OFF\n");
		break;
	case _IOC_NR(HOP_CLEAN_TIDS) : 
		if (!(err = ioctl(fd, HOP_CLEAN_TIDS)))
			printf("IOCTL: HOP_CLEAN_TIDS\n");
		break;
	case _IOC_NR(HOP_ADD_TID) :
		printf("tid >> ");
		uvar = int_from_stdin();
		if (!(err = ioctl(fd, HOP_ADD_TID, &uvar)))
			printf("HOP_ADD_TID %u\n", uvar);
		break;
	case _IOC_NR(HOP_DEL_TID) : 
		printf("tid >> ");
		uvar = int_from_stdin();
		if (!(err = ioctl(fd, HOP_DEL_TID, &uvar)))
			printf("HOP_DEL_TID %u\n", uvar);
		break;
	case _IOC_NR(HOP_START_TID) :
		printf("tid >> ");
		uvar = int_from_stdin();
		if (!(err = ioctl(fd, HOP_START_TID, &uvar)))
			printf("HOP_START_TID %u\n", uvar);
		break;
	case _IOC_NR(HOP_STOP_TID) : 
		printf("tid >> ");
		uvar = int_from_stdin();
		if (!(err = ioctl(fd, HOP_STOP_TID, &uvar)))
			printf("HOP_STOP_TID %u\n", uvar);
		break;
	case _IOC_NR(HOP_SET_BUF_SIZE) :
		printf("size >> ");
		uvar = int_from_stdin();
		if (!(err = ioctl(fd, HOP_SET_BUF_SIZE, &uvar)))
			printf("HOP_SET_BUF_SIZE %u\n", uvar);
		break;
	case _IOC_NR(HOP_SET_SAMPLING) : 
		printf("freq >> ");
		uvar = int_from_stdin();
		if (!(err = ioctl(fd, HOP_SET_SAMPLING, &uvar)))
			printf("HOP_SET_SAMPLING %u\n", uvar);
		break;
	default : 
		break;
	}

	return err;
}// ioctl_cmd

const char * device = "/dev/hop/ctl";

int main (int argc, char* argv[])
{

	int fd = open(device, 0666);

	if (fd < 0) {
		printf("Error, cannot open %s\n", device);
		return -1;
	}

	printf("What do you wanna do?\n");
	printf("0) EXIT\n");
	printf("1) IOCTL\n");

	int cmd = int_from_stdin();

	while (cmd)	{
		switch (cmd) {
		case IOCTL :
			if (ioctl_cmd(fd))
				printf("IOCTL ERROR\n");
			break;
		default : 
			fprintf(stderr, "bad cmd\n");
		}

		printf("\n\n NEW REQ \n\n\n");
		printf("0) EXIT\n");
		printf("1) IOCTL\n");
		cmd = int_from_stdin();
	}

    close(fd);
	return 0;
}// main
