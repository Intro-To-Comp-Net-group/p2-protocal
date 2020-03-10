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
#include <string>
#include "iostream"
//#include "receiverController.h"
#include "utils.h"


using namespace std;

struct ack_packet {
    u_short crc;
    u_short seq_num;
    u_short ack;
    uint32_t send_sec;
    uint32_t send_usec;
    char * padding;
};

struct meta_data {
    uint16_t file_size;
    string file_dir;
    string file_name;
};

struct window_node {
    bool isReceived;
    u_short seq_num;
    struct ack_packet * data;
};

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
    char * recv_port_str = argv[2];
    if (!recv_port_str) {
        perror("Invalid recv_port input! \n");
        exit(0);
    }
    int recv_port = atoi(recv_port_str);
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
    socklen_t client_len = sizeof(client_sin);

    memset(&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sin.sin_port = htons(recv_port);

    if (::bind(server_sock, (struct sockaddr *) &server_sin, sizeof(server_sin)) < 0) { // make clear that it's in the global namespace
        perror("binding socket to address error!! \n");
        exit(1);
    }

    char *buff[BUFFER_SIZE + 16];
    // Start listening
    // First, receive the metadata (first packet)
    ack_packet * first_ack = (ack_packet *)malloc(16*sizeof(char));// = RController.get_ack_packet();
    uint16_t last_packet_seq;
    while (true) {
        int recv_packet_length = recvfrom(server_sock, buff, sizeof(meta_data),0,(struct sockaddr*) &client_sin, &client_len);
        if (recv_packet_length <= 0) continue;

        meta_data * file_received = (meta_data *) buff;
        last_packet_seq = file_received->file_size;
        cout << "File Size Received:" << file_received->file_size<< endl;
        cout << "File Name Received:" << file_received->file_name<< endl;
        cout << "File Dir Received:" << file_received->file_dir << endl;

        // NEED CHECKSUM
        cout << "RECEIVER: GET META" << endl;
        first_ack->seq_num = htons(0);
        first_ack->ack = htons(MAX_SEQ_LEN - 1);
        sendto(server_sock, first_ack, 16, 0,(struct sockaddr*) &client_sin, client_len);
        cout << "RECEIVER: Send ACK" << endl;
        break;
    }


//    // Sliding Window
//    window_node * window = new window_node[WINDOW_SIZE];
//    for (int i = 0; i < WINDOW_SIZE; i++) {
//        window[i].isReceived = false;
//        window[i].data = (ack_packet *) malloc(ACK_DATA_LEN);
//    }
//
//    uint16_t curr_ack = 0;
//    uint16_t last_seq = 0;
//    bool isComplete = false;


//    while (true) {
//        memset(buff, 0, BUFFER_SIZE + 16);
//        int recv_packet_length = recvfrom(server_sock,buff, BUFFER_SIZE + 16, 0, (struct sockaddr*) &client_sin, &client_len);
//        if (recv_packet_length > 0) {
//            cout << "RECEIVER: PACKET RECEIVED" << endl;
//            // Get seq_num of packet received
//
//            // Check if this is out of the range of window, ignore and do not send ack back!
//
//            // If the packet is received? (duplicate) Do not send ack back!
//
//            // If the expecting packet arrives, write back to file and move window
//
//            // Check is finished?
//
//        } else continue;
//    }

    free(first_ack);
    close(server_sock);
    return 0;
}

