# run the build command when used from the kernel build route -> KERNELDIR
ifneq ($(KERNELRELEASE),)
	obj-m := src/nitro_anv15_51.o
	src/nitro_anv15_51-y := src/nitro_anv15_51_module.o src/misc.o src/nitro_battery_control.o
# when run from anywhere else, run make again using the KERNELDIR
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif