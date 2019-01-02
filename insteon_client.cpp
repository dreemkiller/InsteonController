#include <stdio.h>

#include "stdio_thread.h"
#include "mbed.h"
#include "EthernetInterface.h"
#include "wizfi310-driver/WizFi310Interface.h"

#include "insteon_client.h"
#include "floorplan_regions.h"
#include "lcd.h"
#include "light_status.h"

extern RectangularRegion floorplan_regions[];
extern uint32_t num_floorplan_regions;
extern DigitalOut led2;
extern DigitalOut led3;
extern Mutex network_mutex;

WizFi310Interface net(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX, false);


int network_setup() {
    safe_printf("network_setup\n");

    net.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2, 0);

    const char *ip = net.get_ip_address();
    if (ip == NULL) {
        safe_printf("Failed to connect to network. Not sure why\n");
    }
    safe_printf("IP address is %s\n", ip ? ip: "No IP");
    if (ip == NULL) {
        return 1;
    } else {
        return 0;
    }
}


void insteon_control(uint32_t id, IdType type, uint32_t command) {
    TCPSocket socket;
    network_mutex.lock();
    led2 = 0;
    int open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("socket open failed:%d\n", open_result);
        return;
    }
    int connect_result = -1;
    while (connect_result) {
        connect_result = socket.connect(MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);
        if (connect_result != 0) {
            safe_printf("socket connect failed:%d\n", connect_result);
            socket.close();
            while (network_setup()) {
                led3 = 0;
                safe_printf("Failed to set up network. Will try again in a bit\n");
                wait(MBED_CONF_APP_NETWORK_CHECK_INTERVAL);
            }
            led3 = 0;
        }
    }
    char sbuffer [1024];
    if (type == DEVICE_ID) {
        sprintf(sbuffer, "GET /3?0262%06lx0F%02lxFF=I=3 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", id, command, MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);
    } else if (type == GROUP_ID) {
        sprintf(sbuffer, "GET /0?%lx%02lu=I=0 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", command, id, MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);
    } else {
        MBED_ASSERT(0);
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
    led2 = 1;
}

void insteon_loop() {
    Thread light_status_thread;
    int err = light_status_thread.start(&light_status_loop);
    if (err != 0) {
        safe_printf("Failed to start light status thread\n");
        MBED_ASSERT(0);
    }

    safe_printf("insteon_setup complete\n");
    while(true) {
        safe_printf("Http Thread waiting\n");
        osEvent wait_result = Thread::signal_wait(0);
        int32_t signals = wait_result.value.signals;
        for (uint32_t region_num = 0; region_num < num_floorplan_regions; region_num++ ) {
            RectangularRegion this_region = floorplan_regions[region_num];
            if (signals & floorplan_regions[region_num].on_signal) {
                floorplan_regions[region_num].on = true;
                insteon_control(this_region.arguments.id, this_region.arguments.type, INSTEON_ON);
                light_region(this_region.XMin, this_region.YMin, this_region.XMax, this_region.YMax);
            } else if (signals & this_region.off_signal) {
                floorplan_regions[region_num].on = false;
                insteon_control(this_region.arguments.id, this_region.arguments.type, INSTEON_OFF);
                unlight_region(this_region.XMin, this_region.YMin, this_region.XMax, this_region.YMax);
            }
        }
    }
}