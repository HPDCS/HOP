obj-m := hop.o
hop-objs := lend.o ld.o ibs-control.o hop-manager.o hop-fops.o hop-interrupt.o hop-mod.o

ifeq ($(type), irq)
ccflags-y := -D_IRQ
else
ccflags-y := -D_NMI
endif

KDIR ?= /lib/modules/$(shell uname -r)/build
# EXTRA_CFLAGS=-I$(PWD)/../include


all:
	# make -C $(KDIR) M=$(PWD) $(INC) modules
	@echo $(ccflags-y)
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod hop.ko
	dmesg | tail -n 15
remove:
	sudo rmmod hop
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	dmesg | tail -f

reboot:
	sudo rmmod hop
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod hop.ko
	dmesg | tail -f
