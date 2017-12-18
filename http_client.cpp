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
    socket.connect("developer.mbed.org", 80);
    char sbuffer [] = "GET / HTTP/1.1\r\nHost: developer.mbed.org\r\n\r\n";
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