#ifndef NITRO_ANV15_51_POWER_PROFILES_H
#define NITRO_ANV15_51_POWER_PROFILES_H

#include "misc.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/wmi.h>


#define POWER_PROFILE_GET_MISCELLANEOUS_SETTING_METHOD_ID 23
#define POWER_PROFILE_SET_MISCELLANEOUS_SETTING_METHOD_ID 22


/************************************
* File operation method definitions *
************************************/

ssize_t nitro_profile_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
);

ssize_t nitro_profile_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
);


/************************************
******* WMI evaluation in/out *******
************************************/

struct __attribute__((packed)) profile_get_in {
    u32 gmInput;
};

struct __attribute__((packed)) profile_get_out {
    u64 gmOutput;
};

struct __attribute__((packed)) profile_write_in {
    u64 gmInput;
};

struct __attribute__((packed)) profile_write_out {
    u32 gmOutput;
};

#endif
