#include <stdio.h>

#include "mbed.h"
#include "EthernetInterface.h"

#include "http_client.h"

EthernetInterface net;

void http_setup() {
    printf("http_setup\n");

    net.connect();

    const char *ip = net.get_ip_address();
    printf("IP address is %s\n", ip ? ip: "No IP");

    
}

void http_get() {
    TCPSocket socket;
    socket.open(&net);
    socket.connect("digg.com", 80);
    char sbuffer [] = "GET / HTTP/1.1\r\nHost: digg.com\r\n\r\n";
    int scount = socket.send(sbuffer, sizeof(sbuffer));
    printf("sent %d [%.*s]\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    // Receive a simple http response
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof(rbuffer));
    printf("recv %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    socket.close();
}

void http_stop() {
    net.disconnect();
}

#define INSTEON_IP "192.168.1.121"
#define INSTEON_ON 0x11 
#define INSTEON_FAST_ON 0x12
#define INSTEON_OFF 0x13
#define INSTEON_FAST_OFF 0x14
#define INSTEON_BRIGHT 0x15
#define INSTEON_DIM 0x16
#define INSTEON_START_DIM_BRIGHT 0x17
#define INSTEON_STOP_DIM_BRIGHT 0x18

#define LIVING_ROOM_GROUP_NUMBER 11111
#define OUTSIDE_GROUP_NUMBER     22222

void insteon(uint32_t group, uint32_t command) {
    TCPSocket socket;
    socket.open(&net);
    socket.connect(INSTEON_IP, 80);
    char sbuffer [1024];
    sprintf(sbuffer, "GET /0?%lu%lu=I=0 HTTP/1.1\r\nHost: %s\r\n\r\n", command, group, INSTEON_IP);
    int scount = socket.send(sbuffer, sizeof(sbuffer));
    printf("sent %d [%.*s]\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof(rbuffer));
    printf("recv %d [%.*s]\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);
    socket.close();
}

void turn_on_living_room() {
    insteon(LIVING_ROOM_GROUP_NUMBER, INSTEON_ON);
}

void turn_off_living_room() {
    insteon(LIVING_ROOM_GROUP_NUMBER, INSTEON_OFF);
}

void turn_on_outside() {
    insteon(OUTSIDE_GROUP_NUMBER, INSTEON_ON);
}

void turn_off_outside() {
    insteon(OUTSIDE_GROUP_NUMBER, INSTEON_OFF);
}