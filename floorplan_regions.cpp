#include <stdio.h>
#include "floorplan_regions.h"
#include "insteon_interface.h"

uint32_t num_floorplan_regions = 4;
RectangularRegion floorplan_regions[] = {
    { 104, 208, 125, 290, "Living Room", INSTEON_LIVING_ROOM_ON_SIGNAL, INSTEON_LIVING_ROOM_OFF_SIGNAL, { LIVING_ROOM_GROUP_ID, GROUP_ID  } },
    { 208, 269, 125, 194,   "Breakfast", INSTEON_BFAST_ON_SIGNAL,       INSTEON_BFAST_OFF_SIGNAL,       { BREAKFAST_INSTEON_ID, DEVICE_ID } },
    { 208, 269, 193, 315,     "Kitchen", INSTEON_KITCHEN_ON_SIGNAL,     INSTEON_KITCHEN_OFF_SIGNAL,     { KITCHEN_INSTEON_ID, DEVICE_ID   } },
    { 104, 269,  38, 125,       "Patio", INSTEON_OUTSIDE_ON_SIGNAL,     INSTEON_OUTSIDE_OFF_SIGNAL,     { OUTSIDE_GROUP_NUMBER, DEVICE_ID } }
};