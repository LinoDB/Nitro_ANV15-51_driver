Creating a driver for the Nitro ANV15-51 laptop to make use of WMI functions.

This is a project to learn about driver development on linux.

### Resources
- https://lwn.net/Kernel/LDD3/
- https://github.com/torvalds/linux/blob/master/Documentation/wmi/driver-development-guide.rst
- https://docs.kernel.org/driver-api/wmi.html#c.wmi_driver
- https://github.com/0x7375646F/Linuwu-Sense
- https://github.com/DetuxTR/AcerNitroLinuxGamingDriver

## Installation

This driver is meant to be used only on the PC model **Acer Nitro ANV15-51**. Even if using this specific model, it can neither be guaranteed that this driver doesn't damage the system nor that it works properly. Proceed at your own risk!


### For testing

Use the following commands to test the module:

```bash
# compile the module
make

# load the module
sudo insmod src/nitro_anv15_51.ko

# remove the module
sudo rmmod nitro_anv15_51
```


### System-wide

Run

```bash
make install
```

to install the driver globally and

```bash
make uninstall
```

to uninstall it.


## Use

Check current charge limit:

```bash
cat /dev/nitro_battery_charge_limit
# either "80%" or "100%"
```

Set current charge limit:

```bash
# set charge limit to "80%"
echo 1 > /dev/nitro_battery_charge_limit

# set charge limit to "100%"
echo 0 > /dev/nitro_battery_charge_limit
```
