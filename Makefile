MODNAME = nitro_anv15_51
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
obj-m := src/$(MODNAME).o
src/nitro_anv15_51-y := src/nitro_anv15_51_module.o src/misc.o src/nitro_battery_control.o
	
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
