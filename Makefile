MODNAME = nitro_anv15_51
KERNELDIR = /lib/modules/$(shell uname -r)
MODDIR = $(KERNELDIR)/kernel/drivers/$(MODNAME)
PWD := $(shell pwd)
obj-m := src/$(MODNAME).o
src/nitro_anv15_51-y := src/nitro_anv15_51_module.o src/misc.o src/nitro_battery_control.o
	
all:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

install: all
	sudo rmmod $(MODNAME) 2> /dev/null || true
	sudo mkdir $(MODDIR) 2> /dev/null || true
	sudo cp src/$(MODNAME).ko $(MODDIR)
	echo "$(MODNAME)" | sudo tee /etc/modules-load.d/$(MODNAME).conf > /dev/null
	sudo depmod -a
	sudo modprobe $(MODNAME)

uninstall:
	sudo rmmod $(MODNAME) 2> /dev/null || true
	sudo rm -r $(MODDIR) 2> /dev/null || true
	sudo rm /etc/modules-load.d/$(MODNAME).conf 2> /dev/null || true
