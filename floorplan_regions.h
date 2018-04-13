#ifndef __FLOORPLAN_REGIONS_H__
#define __FLOORPLAN_REGIONS_H__

typedef struct RectangularRegion {
    uint32_t XMin;
    uint32_t XMax;
    uint32_t YMin;
    uint32_t YMax;
    const char *Name;
    int32_t on_signal;
    int32_t off_signal;
} RectangularRegion;

#endif // __FLOORPLAN_REGIONS_H__