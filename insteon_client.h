#ifndef __INSTEON_INTERFACE_H__
#define __INSTEON_INTERFACE_H__

typedef enum {
    GROUP_ID,
    DEVICE_ID,
} IdType;

struct InsteonArguments {
    uint32_t id;
    IdType type;
    uint32_t reference_device; // I can't query group status,
                               // so instead I need to query
                               // the status of a "reference"
                               // device in the group
                               // This is not completely reliable
                               // (a device in the group may have
                               // a different status than the rest
                               // of the devices)
};

#define INSTEON_ON 0x11 
#define INSTEON_OFF 0x13

#define INSTEON_STATUS_REQUEST 0x19
#define INSTEON_STATUS_DELAY 1.0f
#define LIGHT_STATUS_INTERVAL 30.0f

int insteon_get_region_on(uint32_t id, IdType type);

void light_status_loop();

void insteon_loop();

void network_status_loop();

void check_network();

#endif // __INSTEON_INTERFACE_H__