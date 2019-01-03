#include <stdio.h>

#include "stdio_thread.h"
#include "mbed.h"
#include "wizfi310-driver/WizFi310Interface.h"

#include "insteon_client.h"
#include "floorplan_regions.h"
#include "lcd.h"

extern RectangularRegion floorplan_regions[];
extern uint32_t num_floorplan_regions;

extern WizFi310Interface net;
extern Mutex network_mutex;
extern uint32_t current_floor;
extern DigitalOut led2;
extern DigitalOut led3;

/* Get the on/off status of an Insteon device
 * The following sequence is derived from page 10 under "Status Request Example:" of the pdf at
 * http://cache.insteon.com/developer/2242-222dev-062013-en.pdf
 * In case the URL gets stale, the document title is "Insteon Hub: Developer's Guide"
 */
/* TODO: look into using http://X.X.X.X:25105/sx.xml?ABABAB=1900, from http://forum.smarthome.com/topic.asp?TOPIC_ID=13762
 */
static int get_device_status(uint32_t id) {
    TCPSocket socket;
    network_mutex.lock();
    led2 = 0;
    int open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("socket open failed:%d\n", open_result);
        network_mutex.unlock();
        led2 = 1;
        return -1;
    }
    int connect_result = -1;
    while (connect_result) {
        connect_result = socket.connect(MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);
        if (connect_result != 0) {
            safe_printf("socket connect failed:%d\n", connect_result);
            socket.close();
            const char *ip = net.get_ip_address();
            if (ip == NULL) {
                while (network_setup()) {
                    led3 = 0;
                    safe_printf("Failed to set up network. Will try again in a bit\n");
                    wait(MBED_CONF_APP_NETWORK_CHECK_INTERVAL);
                }
                led3 = 1;
            } else {
                // the endpoint is probably not available. bail
                network_mutex.unlock();
                led2 = 1;
                return -1;
            }
        }
    }
    char read_status_command[17];
    snprintf(read_status_command, sizeof(read_status_command), "0262%06lX0F1900", id);
    char sbuffer[1024];
    sprintf(sbuffer, "GET /3?%s=I=3 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", read_status_command, MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);
    int scount = socket.send(sbuffer, strlen(sbuffer));
    if (scount == 0) {
        safe_printf("Failed to send\n");
    }

    char rbuffer[1024];
    memset(rbuffer, 0, sizeof(rbuffer));
    socket.recv(rbuffer, sizeof(rbuffer));
    int close_result = socket.close();
    if (close_result != 0) {
        //safe_printf("Socket close failed:%d\n");
        //return -1;
    }
    network_mutex.unlock();
    led2 = 1;

    network_mutex.lock();
    led2 = 0;
    open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("Socket open failed:%d\n", open_result);
        return -1;
    }

    connect_result = -1;
    while (connect_result) {
        connect_result = socket.connect(MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);
        if (connect_result != 0) {
            safe_printf("socket connect failed:%d\n", connect_result);
            socket.close();
            const char *ip = net.get_ip_address();
            if (ip == NULL) {
                while (network_setup()) {
                    led3 = 0;
                    safe_printf("Failed to set up network. Will try again in a bit\n");
                    wait(MBED_CONF_APP_NETWORK_CHECK_INTERVAL);
                }
                led3 = 1;
            } else {
                // the endpoint is probably not available
                network_mutex.unlock();
                led2 = 1;
                return -1;
            }
        }
    }

    // wait for the response to be ready
    wait(INSTEON_STATUS_DELAY);

    sprintf(sbuffer, "GET /buffstatus.xml HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", MBED_CONF_APP_INSTEON_IP, MBED_CONF_APP_INSTEON_PORT);

    scount = socket.send(sbuffer, strlen(sbuffer));
    if (scount == 0) {
        safe_printf("Failed to send get for buffstatus.xml");
    }
    memset(rbuffer, 0, sizeof(rbuffer));
    socket.recv(rbuffer, sizeof(rbuffer));

    socket.close();
    network_mutex.unlock();
    led2 = 1;

    // Now, I could implement an entire XML parser (kinda hard, kinda memory intensive), or I could just munge the string. Which do you think
    // I'll do?
    char *rbuffer_ptr = &rbuffer[101]; // 101 bytes skips past some header stuff that's uninteresting to me
    if (strncmp(rbuffer_ptr, read_status_command, strlen(read_status_command))) {
        // I got unexpected data in the buffer
        safe_printf("Unexpected buffer response\n");
        return -1;
    }
    rbuffer_ptr += strlen(read_status_command);
    if (strncmp(rbuffer_ptr, "06", 2)) { // PLM says "I got it"
        safe_printf("PLM did not say \"I got it\"\n");
        return -1;
    }
    rbuffer_ptr += 2;
    if (strncmp(rbuffer_ptr, "0250", 4)) { // From PLM Insteon Received
        safe_printf("\"From PLM Insteon Received\" not received, instead %c%c%c%c\n", rbuffer_ptr[0], rbuffer_ptr[1], rbuffer_ptr[2], rbuffer_ptr[3]);
        //return -1;
    }
    rbuffer_ptr += 4;
    char device_id_str[7];
    snprintf(device_id_str, sizeof(device_id_str), "%06lX", id);
    if (strncmp(rbuffer_ptr, device_id_str, strlen(device_id_str))) {
        safe_printf("Did not receive a response for the correct device. Expected %s, got %s\n", device_id_str, rbuffer_ptr);
        return -1;
    }
    rbuffer_ptr += (int)strlen(device_id_str);
    rbuffer_ptr += 6; // The PLM Device ID is uninsteresting to me here
    rbuffer_ptr += 2; // so is hop count
    rbuffer_ptr += 2; // so is the delta
    if (strncmp(rbuffer_ptr, "FF", 2)) {
        // not all the way on
        return 0;
    } else {
        return 1;
    }
}

int insteon_get_region_on(uint32_t id, IdType type, uint32_t reference_device) {
    if (type == DEVICE_ID) {
        return get_device_status(id);
    } else {
        return get_device_status(reference_device);
    }
    
}

void update_regions() {
    lcd_update_mutex.lock();
    for (uint32_t i = 0; i < num_floorplan_regions; i++) {
        RectangularRegion this_region = floorplan_regions[i];
        if (this_region.floor == current_floor) {
            if (this_region.on) {
                light_region(this_region.XMin, this_region.YMin, this_region.XMax, this_region.YMax);
            } else {
                unlight_region(this_region.XMin, this_region.YMin, this_region.XMax, this_region.YMax);
            }
        }
    }
    lcd_update_mutex.unlock();
}

void light_status_loop() {
    while(true) {
        safe_printf("Light status check triggered\n");
        for (uint32_t i = 0; i < num_floorplan_regions; i++) {
            RectangularRegion this_region = floorplan_regions[i];
            int region_on_result = insteon_get_region_on(this_region.arguments.id,
                                                        this_region.arguments.type,
                                                        this_region.arguments.reference_device);
            if (region_on_result == 0) {
                floorplan_regions[i].on = false;
            } else if (region_on_result == 1) {
                floorplan_regions[i].on = true;
            } else {
                safe_printf("Failed to get region(%s) status:%d\n", this_region.Name, region_on_result);
            }
        }
        update_regions();
        wait(MBED_CONF_APP_LIGHTSTATUS_CHECK_INTERVAL); // TODO: Also wait for screen saver to be off
    }
}