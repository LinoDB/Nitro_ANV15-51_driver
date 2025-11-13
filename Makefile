# run the build command when used from the kernel build route -> KERNELDIR
ifneq ($(KERNELRELEASE),)
	obj-m := src/hello_world_module.o
# when run from anywhere else, run make again using the KERNELDIR
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif