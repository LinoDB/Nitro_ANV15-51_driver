#ifndef NITRO_ANV15_51_H
#define NITRO_ANV15_51_H


/************************************
****** Major device structure *******
************************************/

struct nitro_anv15_51 {
    dev_t devno;
    int major;
    unsigned int char_device_count;
    bool initialized;
    const char* name;
    struct nitro_char_dev** char_devs;
};

#endif
