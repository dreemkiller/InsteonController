#include <stdio.h>

#include "stdio_thread.h"
#include "mbed.h"
#include "EthernetInterface.h"

#include "http_client.h"

#define USE_DIGG 0

EthernetInterface net;

#define INSTEON_IP "192.168.0.100"
#define INSTEON_ON 0x11 
#define INSTEON_FAST_ON 0x12
#define INSTEON_OFF 0x13
#define INSTEON_FAST_OFF 0x14
#define INSTEON_BRIGHT 0x15
#define INSTEON_DIM 0x16
#define INSTEON_START_DIM_BRIGHT 0x17
#define INSTEON_STOP_DIM_BRIGHT 0x18

//#define LIVING_ROOM_GROUP_NUMBER 0x264b78
#define LIVING_ROOM_GROUP_NUMBER 0x02
#define OUTSIDE_GROUP_NUMBER     0x2896d5
#define KITCHEN_INSTEON_ID       0x46e275
#define BREAKFAST_INSTEON_ID     0x46e4f9

#define INSTEON_PORT 25105



void http_setup() {
    safe_printf("http_setup\n");

    net.connect();

    const char *ip = net.get_ip_address();
    safe_printf("IP address is %s\n", ip ? ip: "No IP");    
}

void http_get() {
    TCPSocket socket;
    socket.open(&net);
    socket.connect("digg.com", 80);
    char sbuffer [] = "GET / HTTP/1.1\r\nHost: digg.com\r\n\r\n";
    int scount = socket.send(sbuffer, sizeof(sbuffer));
    safe_printf("sent %d [%.*s]\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    // Receive a simple http response
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof(rbuffer));
    safe_printf("recv %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    socket.close();
}

typedef enum {
    GROUP_ID,
    DEVICE_ID,
} IdType;

void insteon(uint32_t id, IdType type, uint32_t command) {
    safe_printf("insteon called\n");
    TCPSocket socket;
    int open_result = socket.open(&net);
    if (open_result != 0) {
        safe_printf("socket open failed:%d\n", open_result);
        return;
    }
    safe_printf("socket opened\n");
    int connect_result = socket.connect(INSTEON_IP, INSTEON_PORT);
    if (connect_result != 0) {
        safe_printf("socket connect failed:%d\n", connect_result);
        return;
    }
    safe_printf("socket connected\n");
    char sbuffer [1024];
    if (type == DEVICE_ID) {
        sprintf(sbuffer, "GET /3?0262%06x0F%02xFF=I=3 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", id, command, INSTEON_IP, INSTEON_PORT);
    } else {
        sprintf(sbuffer, "GET /0?%lu%02lu=I=0 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", command, id, INSTEON_IP, INSTEON_PORT);
    }
    safe_printf("sending:%s\n", sbuffer);   
    int scount = socket.send(sbuffer, sizeof(sbuffer));
    safe_printf("sent %d [%.*s]\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    char rbuffer[64];
    memset(rbuffer, 0, sizeof(rbuffer));
    int rcount = socket.recv(rbuffer, sizeof(rbuffer));
    safe_printf("received: %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    int close_result = socket.close();
    if (close_result != 0) {
        safe_printf("close failed:%d\n", close_result);
    }
}

void turn_on_living_room() {
    safe_printf("Thread: turn on living room\n");
    #if USE_DIGG
    http_get();
    #else
    insteon(LIVING_ROOM_GROUP_NUMBER, GROUP_ID, INSTEON_ON);
    #endif
}

void turn_off_living_room() {
    safe_printf("Thread: turn off living room\n");
    #if USE_DIGG
    http_get();
    #else
    insteon(LIVING_ROOM_GROUP_NUMBER, DEVICE_ID, INSTEON_OFF);
    #endif
}

void turn_on_outside() {
    safe_printf("Thread: turn on outside\n");
    #if USE_DIGG
    http_get();
    #else
    insteon(OUTSIDE_GROUP_NUMBER, DEVICE_ID, INSTEON_ON);
    #endif
}

void turn_off_outside() {
    safe_printf("Thread: turn off outside\n");
    #if USE_DIGG
    http_get();
    #else
    insteon(OUTSIDE_GROUP_NUMBER, DEVICE_ID, INSTEON_OFF);
    #endif
}

void turn_on_kitchen() {
    safe_printf("Thread: turn on kitchen\n");
    #if USE_DIGG
    http_get();
    #else
    insteon(KITCHEN_INSTEON_ID, DEVICE_ID, INSTEON_ON);
    #endif
}

void turn_on_breakfast() {
    safe_printf("Thread: turn on breakfast\n");
    #if USE_DIGG
    http_get();
    #else
    insteon(BREAKFAST_INSTEON_ID, DEVICE_ID, INSTEON_ON);
    #endif
}

typedef void (*SignalFunction)(void);

SignalFunction signal_functions[] = {
    turn_on_living_room,
    turn_off_living_room,
    turn_on_outside,
    turn_off_outside,
    http_get, // BFast on
    http_get, // BFast off
    turn_on_kitchen, // Kitchen on
    http_get, // Kitchen off
};

void http_loop() {
    http_setup();
    safe_printf("http_setup complete\n");
    while(true) {
        safe_printf("Http Thread waiting\n");
        osEvent wait_result = Thread::signal_wait(0);
        int32_t signals = wait_result.value.signals;
        int32_t bit_count = 0;
        while(signals) {
            if (signals & 0x1) {
                if (signal_functions[bit_count]) {
                    signal_functions[bit_count]();
                } else {
                    safe_printf("Unsupported signal received\n");
                }
            }
            bit_count++;
            signals >>= 1;
        }
    }
}
