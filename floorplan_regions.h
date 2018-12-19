#ifndef __FLOORPLAN_REGIONS_H__
#define __FLOORPLAN_REGIONS_H__
#include "insteon_client.h"

typedef struct RectangularRegion {
    uint32_t XMin;
    uint32_t XMax;
    uint32_t YMin;
    uint32_t YMax;
    const char *Name;
    uint32_t floor;
    int32_t on_signal;
    int32_t off_signal;
    struct InsteonArguments arguments;
} RectangularRegion;

#define LIVING_ROOM_GROUP_ID 0x08
#define OUTSIDE_GROUP_NUMBER     0x2896d5
#define KITCHEN_INSTEON_ID       0x46e275
#define BREAKFAST_INSTEON_ID     0x46e4f9

#define INSTEON_LIVING_ROOM_ON_SIGNAL 0x00000001
#define INSTEON_LIVING_ROOM_OFF_SIGNAL 0x00000002
#define INSTEON_OUTSIDE_ON_SIGNAL 0x00000004
#define INSTEON_OUTSIDE_OFF_SIGNAL 0x00000008
#define INSTEON_BFAST_ON_SIGNAL 0x00000010
#define INSTEON_BFAST_OFF_SIGNAL 0x00000020
#define INSTEON_KITCHEN_ON_SIGNAL 0x00000040
#define INSTEON_KITCHEN_OFF_SIGNAL 0x00000080

#endif // __FLOORPLAN_REGIONS_H__