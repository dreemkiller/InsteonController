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

void insteon_loop();
#endif // __INSTEON_INTERFACE_H__