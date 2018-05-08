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

#define INSTEON_IP "192.168.0.100"

#define INSTEON_PORT 25105

#define INSTEON_STATUS_REQUEST 0x19
#define INSTEON_STATUS_DELAY 1.0f
#define LIGHT_STATUS_INTERVAL 30.0f

int insteon_get_region_on(uint32_t id, IdType type);

void light_status_loop();

void insteon_loop();
#endif // __INSTEON_INTERFACE_H__