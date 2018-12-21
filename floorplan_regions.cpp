#include <stdio.h>
#include "floorplan_regions.h"
#include "insteon_client.h"


RectangularRegion floorplan_regions[] = {
    {  93, 178, 145, 313, "Living Room", 0, INSTEON_LIVING_ROOM_ON_SIGNAL, INSTEON_LIVING_ROOM_OFF_SIGNAL, false, { LIVING_ROOM_GROUP_ID,  GROUP_ID,   0x264f09 } },
    { 178, 260, 145, 213,   "Breakfast", 0, INSTEON_BFAST_ON_SIGNAL,             INSTEON_BFAST_OFF_SIGNAL, false, { BREAKFAST_INSTEON_ID,  DEVICE_ID,  0        } },
    { 178, 260, 214, 335,     "Kitchen", 0, INSTEON_KITCHEN_ON_SIGNAL,         INSTEON_KITCHEN_OFF_SIGNAL, false, { KITCHEN_INSTEON_ID,    DEVICE_ID,  0        } },
    {  93, 212,  35, 145,       "Patio", 0, INSTEON_OUTSIDE_ON_SIGNAL,         INSTEON_OUTSIDE_OFF_SIGNAL, false, { OUTSIDE_GROUP_NUMBER,  DEVICE_ID,  0        } },
    {  67, 177, 337, 435,  "Front Room", 0, INSTEON_FRONT_ROOM_ON_SIGNAL,   INSTEON_FRONT_ROOM_OFF_SIGNAL, false, { FRONT_ROOM_INSTEON_ID, DEVICE_ID, 0         } },
    {  67, 177, 337, 435,  "Front Room", 1, INSTEON_FRONT_ROOM_ON_SIGNAL,   INSTEON_FRONT_ROOM_OFF_SIGNAL, false, { FRONT_ROOM_INSTEON_ID, DEVICE_ID, 0         } },
};
uint32_t num_floorplan_regions = sizeof(floorplan_regions) / sizeof(RectangularRegion);