//
// Created by Hanyi Wang on 3/6/20.
//

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <vector>
#include "senderController.h"

using namespace std;

int main(int argc, char **argv) {
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

    // Get parameters 2
    int find_pos = -1;
    string recv_host_port = string(argv[2]);
    find_pos = recv_host_port.find(':');
    if (recv_host_port.empty() || find_pos == -1) {
        perror("Invalid recv_host_port input type! \n");
        exit(0);
    }
    string recv_host = recv_host_port.substr(0, find_pos);
    struct hostent *host = gethostbyname(recv_host.c_str());
    uint16_t recv_port = stoi(recv_host_port.substr(find_pos + 1));
    if (recv_port < 18000 || recv_port > 18200) {
        perror("Receiver port number should be within 18000 and 18200! \n");
        exit(0);
    }

    // Get parameters 4
    string file_dir_name = string(argv[4]);
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
    string file_dir = file_dir_name.substr(0, find_pos);
    string file_name = file_dir_name.substr(find_pos + 1);
//    size_t file_len;

    // Create socket
    int send_sock = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket
    if (send_sock < 0) {
        perror("Unable to create socket! \n");
        exit(1);
    }

    struct sockaddr_in sender_sin;
    memset(&sender_sin, 0, sizeof(sender_sin));
    sender_sin.sin_family = AF_INET;
    sender_sin.sin_addr.s_addr = *(unsigned int *) host->h_addr_list[0];
    sender_sin.sin_port = htons(recv_port);
    socklen_t sender_sin_len = sizeof(sender_sin);

    bind(send_sock, (struct sockaddr *) &sender_sin, sizeof(sockaddr));

//    fcntl(send_sock, F_SETFL, O_NONBLOCK);  // Non blocking mode

    char * buff[BUFFER_SIZE];

    // Start sending Meta Data
    char * meta_buff[PACKET_DATA_LEN + sizeof(int) + sizeof(bool)]; // Do we actually need bool as indicator?
    meta_data *meta_packet = (meta_data *) meta_buff;
    meta_packet->seq = META_DATA_FLAG;
    meta_packet->file_name = file_name;
    meta_packet->file_dir = file_dir;

    cout << "SEND META DIR: " << meta_packet->file_dir << endl;
    cout << "SEND META Name: " << meta_packet->file_name << endl;
    cout << "SEND META SEQ: " << meta_packet->seq << endl;

    struct timeval timestamp;
    struct timeval meta_time;
    int time_gap;
    bool meta_first_time = true;

    gettimeofday(&timestamp, NULL);
    uint32_t start_sec = timestamp.tv_sec;
    uint32_t start_usec = timestamp.tv_usec;

    struct ack_packet * ack_received = (struct ack_packet *) malloc(sizeof(ack_packet));
    int received_ack_len;
    while (true) {
        // Need Timeout
        gettimeofday(&meta_time,NULL);
        time_gap = (meta_time.tv_sec-start_sec)*1000000+(meta_time.tv_usec - start_usec);
        if (meta_first_time || time_gap > TIMEOUT) {
            if (meta_first_time) meta_first_time = false;
            sendto(send_sock, meta_buff, sizeof(meta_buff), 0, (struct sockaddr *) &sender_sin, sender_sin_len);
            cout << "SEND META DATA" << endl;
        }
        received_ack_len = recvfrom(send_sock, ack_received, sizeof(ack_packet), MSG_DONTWAIT,(struct sockaddr*)&sender_sin, &sender_sin_len);
        int meta_ack = ack_received->ack;
        bool is_meta = ack_received->is_meta;
        if (is_meta) {
            cout << "META ACK RECEIVED" <<endl;
            break;
        }
    }

    // Init window
    vector<sender_window_node *> window;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sender_window_node * node = (sender_window_node *) malloc(sizeof(sender_window_node));  // Do we need it?
        node->is_last = false;
        node->is_received = false;
        window.push_back(node);
    }

//    while (true) {
//
//    }


    for (int i = 0; i < WINDOW_SIZE; i++) {
        sender_window_node * node = window[i];
        free(node);
    }
    close(send_sock);
    return 0;
}

