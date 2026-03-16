# Nitro_ANV15-51_driver

Creating a driver for the Nitro ANV15-51 laptop to make use of WMI functions.

This is a project to learn about driver development on linux.

## Resources and references
- https://lwn.net/Kernel/LDD3/
- https://github.com/torvalds/linux/blob/master/Documentation/wmi/driver-development-guide.rst
- https://docs.kernel.org/driver-api/wmi.html#c.wmi_driver
- https://github.com/0x7375646F/Linuwu-Sense
- https://github.com/DetuxTR/AcerNitroLinuxGamingDriver


# Installation

> [!CAUTION]
> This driver is meant to be used only on the laptop model **Acer Nitro ANV15-51**. Even when using this specific model, it can neither be guaranteed that this driver doesn't damage the system nor that it works properly. Proceed at your own risk!

Clone the repository and change working directory:
```bash
git clone https://github.com/LinoDB/Nitro_ANV15-51_driver.git && cd Nitro_ANV15-51_driver
```

This driver will be compiled during installation. Make sure you have the headers package for your kernel installed before proceeding (e.g. the `linux-headers` package for the `linux` kernel on Arch Linux):

```bash
pacman -S linux-headers
```

## System-wide installation as a DKMS package (recommended)

_These instructions use installation commands for the Arch Linux distribution._

Using DKMS, this kernel module (driver) will be re-compiled automatically when the kernel is updated. This is a great benefit, since the module is not included in the Linux distribution as a package by default, which means that it would need to be reinstalled after every kernel update. To keep the module installed, up to date, and working automatically, make sure you have the `dkms` module installed:
```bash
pacman -S dkms
```

Create the `nitro_anv15_51-dkms` package and install it (this command also works for updating the installed package to a newer version):

```bash
makepkg -i --clean
```

Load the driver module:

```bash
sudo modprobe nitro_anv15_51

# optionally auto-load the driver module at boot
make autostart
```

### Uninstall

Use pacman to uninstall the package:

```bash
sudo pacman -Rsn nitro_anv15_51-dkms
```

Remove the auto-load configuration:

```bash
make autostart-remove
```


## Testing the module without a proper installation

Use the following commands to test the driver module:

```bash
# compile the module
make

# load the module
sudo insmod nitro_anv15_51.ko

# remove the module
sudo rmmod nitro_anv15_51
```


## System-wide installation for the current kernel

> [!WARNING]
> During a kernel update, this installation gets outdated (lost) and the current kernel's `/lib/modules/<kernel-version>` directory won't be deleted - uninstall before updating the current kernel.

```bash
make install
sudo modprobe nitro_anv15_51

# optionally auto-load the driver module at boot
make autostart
```

### Uninstall

```bash
# also removes the auto-load configuration
make uninstall
```


# Usage

## Battery charge limit

Check current charge limit:

```bash
cat /dev/nitro_battery_charge_limit
# either "80%" or "100%"
```

Set charge limit:

```bash
# set charge limit to 80%
echo 1 > /dev/nitro_battery_charge_limit

# set charge limit to 100%
echo 0 > /dev/nitro_battery_charge_limit
```


## Power profile

Check current power profile:

```bash
cat /dev/nitro_power_profile
# either "quiet", "balanced", "performance", or "eco"
```

Set power profile:

```bash
# set power profile to "quiet"
echo "quiet" > /dev/nitro_battery_charge_limit

# set power profile to "balanced"
echo "balanced" > /dev/nitro_battery_charge_limit

# set power profile to "performance"
echo "performance" > /dev/nitro_battery_charge_limit

# set power profile to "eco"
echo "eco" > /dev/nitro_battery_charge_limit
```
