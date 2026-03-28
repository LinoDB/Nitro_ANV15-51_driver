#ifndef NITRO_ANV15_51_FAN_CONTROL_H
#define NITRO_ANV15_51_FAN_CONTROL_H

#include "misc.h"

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/wmi.h>


#define FAN_GET_BEHAVIOUR_METHOD_ID 15
#define FAN_SET_BEHAVIOUR_METHOD_ID 14
#define FAN_GET_SPEED_METHOD_ID 5
#define FAN_SET_SPEED_METHOD_ID 16
#define CPU_FAN_SPEED_READ_VALUE 0x0201
#define CPU_FAN_BEHAVIOUR_MASK 0x02
#define CPU_FAN_AUTO_MODE 0x010001
#define CPU_FAN_MANUAL_MODE 0x030001
#define CPU_FAN_INDEX 0x1
#define GPU_FAN_SPEED_READ_VALUE 0x0601
#define GPU_FAN_BEHAVIOUR_MASK 0x90
#define GPU_FAN_AUTO_MODE 0x400008
#define GPU_FAN_MANUAL_MODE 0xC00008
#define GPU_FAN_INDEX 0x4


/************************************
* File operation method definitions *
************************************/

int _nitro_behaviour_check(
    struct wmi_device* wdev,
    u64* behaviour
);

ssize_t nitro_fan_read(
    struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos
);

ssize_t nitro_fan_write(
    struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos
);


/************************************
******* WMI evaluation in/out *******
************************************/

struct __attribute__((packed)) get_fan_behaviour_in {
    u32 gmInput;
};

struct __attribute__((packed)) get_fan_behaviour_out {
    u64 gmOutput;
};

struct __attribute__((packed)) set_fan_behaviour_in {
    u64 gmInput;
};

struct __attribute__((packed)) set_fan_behaviour_out {
    u32 gmOutput;
};


struct __attribute__((packed)) get_fan_speed_in {
    u32 gmInput;
};

struct __attribute__((packed)) get_fan_speed_out {
    u64 gmOutput;
};

struct __attribute__((packed)) set_fan_speed_in {
    u64 gmInput;
};

struct __attribute__((packed)) set_fan_speed_out {
    u32 gmOutput;
};


#endif
