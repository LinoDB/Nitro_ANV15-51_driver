# Nitro_ANV15-51_driver

Creating a driver for the Nitro ANV15-51 laptop to make use of WMI functions.

This is a project to learn about driver development on linux.

## Resources and references
- https://lwn.net/Kernel/LDD3/
- https://github.com/torvalds/linux/blob/master/Documentation/wmi/driver-development-guide.rst
- https://docs.kernel.org/driver-api/wmi.html#c.wmi_driver
- https://github.com/0x7375646F/Linuwu-Sense
- https://github.com/DetuxTR/AcerNitroLinuxGamingDriver

## Installation

> [!CAUTION]
> This driver is meant to be used only on the laptop model **Acer Nitro ANV15-51**. Even when using this specific model, it can neither be guaranteed that this driver doesn't damage the system nor that it works properly. Proceed at your own risk!


### For testing without proper installation

Use the following commands to test the driver module:

```bash
# compile the module
make

# load the module
sudo insmod src/nitro_anv15_51.ko

# remove the module
sudo rmmod nitro_anv15_51
```


### System-wide installation for the current kernel

> [!WARNING]
> During a kernel update, this installation gets outdated (lost) and the current kernel's `/lib/modules/<kernel-version>` directory won't be deleted - uninstall before updating the current kernel.

```bash
make install
sudo modprobe nitro_anv15_51

# optionally auto-load the driver module at boot
make autostart
```

#### Uninstall

```bash
# also removes the auto-load configuration
make uninstall
```


### System-wide installation as a DKMS package for Arch (recommended)

Create the package and install it (also works for updating the installed package to a newer version):

```bash
makepkg -i --clean
```

Load the driver module:

```bash
sudo modprobe nitro_anv15_51

# optionally auto-load the driver module at boot
make autostart
```

#### Uninstall

Use pacman to uninstall the package:

```bash
sudo pacman -Rs nitro_anv15_51-dkms
```

Remove the auto-load configuration:

```bash
make autostart-remove
```


## Usage

Check current charge limit:

```bash
cat /dev/nitro_battery_charge_limit
# either "80%" or "100%"
```

Set current charge limit:

```bash
# set charge limit to 80%
echo 1 > /dev/nitro_battery_charge_limit

# set charge limit to 100%
echo 0 > /dev/nitro_battery_charge_limit
```
