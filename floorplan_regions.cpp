#include <stdio.h>
#include "floorplan_regions.h"
#include "insteon_client.h"

uint32_t num_floorplan_regions = 4;
RectangularRegion floorplan_regions[] = {
    { 100, 178, 128, 280, "Living Room", INSTEON_LIVING_ROOM_ON_SIGNAL, INSTEON_LIVING_ROOM_OFF_SIGNAL, { LIVING_ROOM_GROUP_ID, GROUP_ID  } },
    { 178, 265, 128, 178,   "Breakfast", INSTEON_BFAST_ON_SIGNAL,       INSTEON_BFAST_OFF_SIGNAL,       { BREAKFAST_INSTEON_ID, DEVICE_ID } },
    { 178, 265, 200, 310,     "Kitchen", INSTEON_KITCHEN_ON_SIGNAL,     INSTEON_KITCHEN_OFF_SIGNAL,     { KITCHEN_INSTEON_ID, DEVICE_ID   } },
    {  90, 214,   3, 127,       "Patio", INSTEON_OUTSIDE_ON_SIGNAL,     INSTEON_OUTSIDE_OFF_SIGNAL,     { OUTSIDE_GROUP_NUMBER, DEVICE_ID } }
};