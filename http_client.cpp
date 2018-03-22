#include <stdio.h>

#include "stdio_thread.h"
#include "mbed.h"
#include "EthernetInterface.h"

#include "http_client.h"

//#define USE_DIGG 0

EthernetInterface net;

#define INSTEON_IP "192.168.0.123"
#define INSTEON_ON 0x11 
#define INSTEON_FAST_ON 0x12
#define INSTEON_OFF 0x13
#define INSTEON_FAST_OFF 0x14
#define INSTEON_BRIGHT 0x15
#define INSTEON_DIM 0x16
#define INSTEON_START_DIM_BRIGHT 0x17
#define INSTEON_STOP_DIM_BRIGHT 0x18

#define LIVING_ROOM_GROUP_NUMBER 0x264b78
#define OUTSIDE_GROUP_NUMBER     0x2896d5

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

void insteon(uint32_t group, uint32_t command) {
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
    sprintf(sbuffer, "GET /3?0262%06x0F%02xFF=I=3 HTTP/1.1\nAuthorization: Basic Q2xpZnRvbjg6MEJSR2M4cnE=\nHost: %s:%d\r\n\r\n", group, command, INSTEON_IP, INSTEON_PORT);
    safe_printf("sending:%s\n", sbuffer);   
    int scount = socket.send(sbuffer, sizeof(sbuffer));
    safe_printf("sent %d [%.*s]\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof(rbuffer));
    safe_printf("received: %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);
    int close_result = socket.close();
    if (close_result != 0) {
        safe_printf("close failed:%d\n", close_result);
    }
}

void turn_on_living_room() {
    safe_printf("Thread: turn on living room\n");
    #ifdef USE_DIGG
    http_get();
    #else
    insteon(LIVING_ROOM_GROUP_NUMBER, INSTEON_ON);
    #endif
}

void turn_off_living_room() {
    safe_printf("Thread: turn off living room\n");
    #ifdef USE_DIGG
    http_get();
    #else
    insteon(LIVING_ROOM_GROUP_NUMBER, INSTEON_OFF);
    #endif
}

void turn_on_outside() {
    safe_printf("Thread: turn on outside\n");
    #ifdef USE_DIGG
    http_get();
    #else
    insteon(OUTSIDE_GROUP_NUMBER, INSTEON_ON);
    #endif
}

void turn_off_outside() {
    safe_printf("Thread: turn off outside\n");
    #ifdef USE_DIGG
    http_get();
    #else
    insteon(OUTSIDE_GROUP_NUMBER, INSTEON_OFF);
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
    http_get, // Kitchen on
    http_get, // Kitchen off
};

void http_loop() {
    http_setup();
    safe_printf("http_setup complete\n");
    while(true) {
        safe_printf("Http Thread waiting\n");
        osEvent wait_result = Thread::signal_wait(0);
        safe_printf("osEvent:%lx\n", wait_result.value.signals);
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
