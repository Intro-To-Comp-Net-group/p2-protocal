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
    if (argc != 5) {
        perror("You have to input 5 arguments! \n");
        exit(0);
    }

    // Get parameters 1
    char *command_to_check = argv[1];
    if (strcmp(command_to_check, "-r") != 0) {
        perror("Invalid command 1, it should be -r! \n");
        exit(0);
    }

    // Get parameters 3
    command_to_check = argv[3];
    if (strcmp(command_to_check, "-f") != 0) {
        perror("Invalid command 3, it should be -f! \n");
        exit(0);
    }

    int find_pos;
    // Get parameters 2
    string recv_host_port = argv[2];
    find_pos = recv_host_port.find(':');
    if (recv_host_port.empty() || find_pos == -1) {
        perror("Invalid recv_host_port input type! \n");
        exit(0);
    }
    string recv_host = recv_host_port.substr(0,find_pos);
    struct hostent * host = gethostbyname(recv_host.c_str());
    int recv_port = stoi(recv_host_port.substr(find_pos + 1));
    if (recv_port < 18000 || recv_port > 18200) {
        perror("Receiver port number should be within 18000 and 18200! \n");
        exit(0);
    }

    // Get parameters 4
    string file_dir_name = argv[4];
    find_pos = file_dir_name.find('/');
    if (file_dir_name.empty() || find_pos == -1) {
        perror("Invalid file_dir_name input type! \n");
        exit(0);
    }
    // check input file existence
    if (!check_file_existence(file_dir_name)) {
        perror("File not exists! \n");
        exit(0);
    }
    string file_dir = file_dir_name.substr(0,find_pos);
    string file_name = file_dir_name.substr(find_pos + 1);

    // Now we are ready to send data!!

    // Create socket and bind server address
    int send_sock = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket
    if (send_sock < 0) {
        perror("Unable to create socket! \n");
//        abort();
        exit(1);
    }

    struct sockaddr_in server_sin;
    memset (&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_addr.s_addr = *(unsigned int *) host->h_addr_list[0];
    server_sin.sin_port = htons(recv_port);


    // Start listening
//    while (true) {
//        // Send data
//
//        // Receive ACK
//    }
    close(send_sock);
    return 0;
}

