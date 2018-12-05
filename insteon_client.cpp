#include <stdio.h>

#include "stdio_thread.h"
#include "mbed.h"
#include "EthernetInterface.h"
#include "wizfi310-driver/WizFi310Interface.h"

#include "insteon_client.h"
#include "floorplan_regions.h"
#include "lcd.h"

extern RectangularRegion floorplan_regions[];
extern uint32_t num_floorplan_regions;
extern DigitalOut led2;
extern Mutex network_mutex;

WizFi310Interface net(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX, false);


void insteon_setup() {
    safe_printf("insteon_setup\n");

    net.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2, 0);

    const char *ip = net.get_ip_address();
    if (ip == NULL) {
        safe_printf("Failed to connect to network. Not sure why\n");
        while(1) {
            led2 = 1;
            wait(0.05);
            led2 = 0;
            wait(0.05);
        }
    }
    safe_printf("IP address is %s\n", ip ? ip: "No IP");    
}


void insteon_control(uint32_t id, IdType type, uint32_t command) {
    TCPSocket socket;
    network_mutex.lock();
    int open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("socket open failed:%d\n", open_result);
        return;
    }
    int connect_result = socket.connect(INSTEON_IP, INSTEON_PORT);
    if (connect_result != 0) {
        safe_printf("socket connect failed:%d\n", connect_result);
        socket.close();
        network_mutex.unlock();
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
    int scount = socket.send(sbuffer, strlen(sbuffer));
    if (scount == 0) {
        safe_printf("Failed to send\n");
        return;
    }
    char rbuffer[1024];
    memset(rbuffer, 0, sizeof(rbuffer));
    socket.recv(rbuffer, sizeof(rbuffer));
    socket.close();
    network_mutex.unlock();
}

void insteon_loop() {
    insteon_setup();
#if 1
    Thread light_status_thread;
    int err = light_status_thread.start(&light_status_loop);
    if (err != 0) {
        safe_printf("Failed to start light status thread\n");
        assert(0);
    }
#endif
    safe_printf("insteon_setup complete\n");
    while(true) {
        safe_printf("Http Thread waiting\n");
        osEvent wait_result = Thread::signal_wait(0);
        int32_t signals = wait_result.value.signals;
        for (uint32_t region_num = 0; region_num < num_floorplan_regions; region_num++ ) {
            RectangularRegion this_region = floorplan_regions[region_num];
            if (signals & this_region.on_signal) {
                insteon_control(this_region.arguments.id, this_region.arguments.type, INSTEON_ON);
                light_region(this_region.XMin, this_region.YMin, this_region.XMax, this_region.YMax);
            } else if (signals & this_region.off_signal) {
                insteon_control(this_region.arguments.id, this_region.arguments.type, INSTEON_OFF);
                unlight_region(this_region.XMin, this_region.YMin, this_region.XMax, this_region.YMax);
            }
        }
    }
}

void check_network() {
    TCPSocket socket;
    network_mutex.lock();
    int open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("socket open failed:%d\n", open_result);
        return;
    }
    int connect_result = socket.connect(INSTEON_IP, INSTEON_PORT);
    if (connect_result != 0) {
        safe_printf("Network check socket connect failed:%d\n", connect_result);
        return;
    }
    socket.close();
    network_mutex.unlock();
}
