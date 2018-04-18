#ifndef __INSTEON_INTERFACE_H__
#define __INSTEON_INTERFACE_H__

typedef enum {
    GROUP_ID,
    DEVICE_ID,
} IdType;

struct InsteonArguments {
    uint32_t id;
    IdType type;
};

#define INSTEON_ON 0x11 
#define INSTEON_OFF 0x13

void insteon_loop();
#endif // __INSTEON_INTERFACE_H__