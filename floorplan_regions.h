#ifndef __FLOORPLAN_REGIONS_H__
#define __FLOORPLAN_REGIONS_H__
#include "insteon_interface.h"

typedef struct RectangularRegion {
    uint32_t XMin;
    uint32_t XMax;
    uint32_t YMin;
    uint32_t YMax;
    const char *Name;
    int32_t on_signal;
    int32_t off_signal;
    struct InsteonArguments arguments;
} RectangularRegion;

#define LIVING_ROOM_GROUP_ID 0x08
#define OUTSIDE_GROUP_NUMBER     0x2896d5
#define KITCHEN_INSTEON_ID       0x46e275
#define BREAKFAST_INSTEON_ID     0x46e4f9

#endif // __FLOORPLAN_REGIONS_H__