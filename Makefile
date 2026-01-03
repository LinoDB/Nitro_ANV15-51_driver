MODNAME = nitro_anv15_51
KERNELDIR = /lib/modules/$(shell uname -r)
MODDIR = $(KERNELDIR)/kernel/drivers/misc
PWD := $(shell pwd)
obj-m := $(MODNAME).o
$(MODNAME)-y := nitro_anv15_51_module.o misc.o nitro_battery_control.o
	
all:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) modules
	zstd -f $(MODNAME).ko 2> /dev/null || true

clean:
	$(MAKE) -C $(KERNELDIR)/build M=$(PWD) clean

install: all
	sudo rmmod $(MODNAME) 2> /dev/null || true
	sudo install -Dm 644 $(MODNAME).ko.zst $(MODDIR)/$(MODNAME).ko.zst 2> /dev/null || sudo install -Dm 644 $(MODNAME).ko $(MODDIR)/$(MODNAME).ko
	sudo depmod -a

autostart:
	echo "$(MODNAME)" | sudo tee /etc/modules-load.d/$(MODNAME).conf > /dev/null

autostart-remove:
	sudo rm /etc/modules-load.d/$(MODNAME).conf || true

uninstall: autostart-remove
	sudo rmmod $(MODNAME) 2> /dev/null || true
	sudo rm $(MODDIR)/$(MODNAME).ko* || true
