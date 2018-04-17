#include <stdio.h>

#include "stdio_thread.h"
#include "mbed.h"
#include "EthernetInterface.h"

#include "insteon_client.h"
#include "floorplan_regions.h"

extern RectangularRegion floorplan_regions[];
extern uint32_t num_floorplan_regions;

#define USE_DIGG 0

EthernetInterface net;

#define INSTEON_IP "192.168.0.100"

#define INSTEON_PORT 25105

void insteon_setup() {
    safe_printf("insteon_setup\n");

    net.connect();

    const char *ip = net.get_ip_address();
    safe_printf("IP address is %s\n", ip ? ip: "No IP");    
}

void insteon(uint32_t id, IdType type, uint32_t command) {
    safe_printf("insteon called\n");
    TCPSocket socket;
    int open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("socket open failed:%d\n", open_result);
        return;
    }
    int connect_result = socket.connect(INSTEON_IP, INSTEON_PORT);
    if (connect_result != 0) {
        safe_printf("socket connect failed:%d\n", connect_result);
        socket.close();
        return;
    }
    char sbuffer [1024];
    if (type == DEVICE_ID) {
        sprintf(sbuffer, "GET /3?0262%06lx0F%02lxFF=I=3 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", id, command, INSTEON_IP, INSTEON_PORT);
    } else if (type == GROUP_ID) {
        sprintf(sbuffer, "GET /0?%lx%02lu=I=0 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", command, id, INSTEON_IP, INSTEON_PORT);
    } else {
        assert(0);
    }
    safe_printf("sending:%s\n", sbuffer);   
    int scount = socket.send(sbuffer, strlen(sbuffer));
    if (scount == 0) {
        safe_printf("Failed to send\n");
    }

    char rbuffer[1024];
    memset(rbuffer, 0, sizeof(rbuffer));
    int rcount = socket.recv(rbuffer, sizeof(rbuffer));
    safe_printf("received: %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    int close_result = socket.close();
    if (close_result != 0) {
        safe_printf("close failed:%d\n", close_result);
        return;
    }
}

void insteon_loop() {
    insteon_setup();
    safe_printf("insteon_setup complete\n");
    while(true) {
        safe_printf("Http Thread waiting\n");
        osEvent wait_result = Thread::signal_wait(0);
        int32_t signals = wait_result.value.signals;
        for (uint32_t region_num = 0; region_num < num_floorplan_regions; region_num++ ) {
            RectangularRegion this_region = floorplan_regions[region_num];
            if (signals & this_region.on_signal) {
                insteon(this_region.arguments.id, this_region.arguments.type, INSTEON_ON);
            } else if (signals & this_region.off_signal) {
                insteon(this_region.arguments.id, this_region.arguments.type, INSTEON_OFF);
            }
        }
    }
}
