//
// Created by Hanyi Wang on 3/6/20.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include "utils.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 3) {
        perror("You have to input 3 arguments! \n");
        exit(0);
    }

    // Get parameters 1
    char *command_to_check = argv[1];
    if (strcmp(command_to_check, "-p") != 0) {
        perror("Invalid command 1, it should be -p! \n");
        exit(0);
    }

    // Get parameters 2
    string recv_port_str = argv[2];
    if (recv_port_str.empty()) {
        perror("Invalid recv_port input! \n");
        exit(0);
    }
    int recv_port = atoi(recv_port_str.c_str());
    if (recv_port < 18000 || recv_port > 18200) {
        perror("Receiver port number should be within 18000 and 18200! \n");
        exit(0);
    }

    int server_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (server_sock < 0) {
        perror("Unable to create socket! \n");
//        abort();
        exit(1);
    }

    struct sockaddr_in server_sin, client_sin;
    memset(&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sin.sin_port = htons(recv_port);

    if (::bind(server_sock, (struct sockaddr *) &server_sin, sizeof(server_sin)) < 0) { // make clear that it's in the global namespace
        perror("binding socket to address error!! \n");
        exit(1);
    }

    // Start listening


    close(server_sock);
    return 0;
}

