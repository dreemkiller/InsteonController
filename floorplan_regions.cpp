#include <stdio.h>
#include "floorplan_regions.h"
#include "insteon_client.h"

uint32_t num_floorplan_regions = 4;
RectangularRegion floorplan_regions[] = {
    {  93, 178, 145, 313, "Living Room", INSTEON_LIVING_ROOM_ON_SIGNAL, INSTEON_LIVING_ROOM_OFF_SIGNAL, { LIVING_ROOM_GROUP_ID, GROUP_ID, 0x264f09 } },
    { 178, 260, 145, 213,   "Breakfast", INSTEON_BFAST_ON_SIGNAL,       INSTEON_BFAST_OFF_SIGNAL,       { BREAKFAST_INSTEON_ID, DEVICE_ID, 0 } },
    { 178, 260, 214, 335,     "Kitchen", INSTEON_KITCHEN_ON_SIGNAL,     INSTEON_KITCHEN_OFF_SIGNAL,     { KITCHEN_INSTEON_ID, DEVICE_ID, 0   } },
    {  93, 212,  35, 145,       "Patio", INSTEON_OUTSIDE_ON_SIGNAL,     INSTEON_OUTSIDE_OFF_SIGNAL,     { OUTSIDE_GROUP_NUMBER, DEVICE_ID, 0 } }
};