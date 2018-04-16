#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#define INSTEON_LIVING_ROOM_ON_SIGNAL 0x00000001
#define INSTEON_LIVING_ROOM_OFF_SIGNAL 0x00000002
#define INSTEON_OUTSIDE_ON_SIGNAL 0x00000004
#define INSTEON_OUTSIDE_OFF_SIGNAL 0x00000008
#define INSTEON_BFAST_ON_SIGNAL 0x00000010
#define INSTEON_BFAST_OFF_SIGNAL 0x00000020
#define INSTEON_KITCHEN_ON_SIGNAL 0x00000040
#define INSTEON_KITCHEN_OFF_SIGNAL 0x00000080

#define INSTEON_ALL_ON_SIGNALS (INSTEON_LIVING_ROOM_ON_SIGNAL | INSTEON_OUTSIDE_ON_SIGNAL | INSTEON_BFAST_ON_SIGNAL | INSTEON_KITCHEN_ON_SIGNAL)
#define INSTEON_ALL_OFF_SIGNALS (INSTEON_LIVING_ROOM_OFF_SIGNAL | INSTEON_OUTSIDE_OFF_SIGNAL | INSTEON_BFAST_OFF_SIGNAL | INSTEON_KITCHEN_OFF_SIGNAL)
#define INSTEON_ALL_SIGNALS (INSTEON_ALL_ON_SIGNALS | INSTEON_ALL_OFF_SIGNALS)

#endif // __HTTP_CLIENT_H__