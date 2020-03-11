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
#include "senderController.cpp"

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
    size_t file_len;

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

    fcntl(send_sock, F_SETFL, O_NONBLOCK);  // Non blocking mode

    // Start sending the first packet -- file metadata
    // Get info about metadata
    struct timeval timestamp;
    int time_gap;
    bool meta_first_time = true;

    senderController sController(file_dir_name);

    gettimeofday(&timestamp, NULL);
    uint32_t start_sec = timestamp.tv_sec;
    uint32_t start_usec = timestamp.tv_usec;
    uint32_t check_point_time = start_usec + 1000000 * start_sec;

    // Sending first packet
    char *buff[BUFFER_SIZE];
    meta_data *first_packet = (meta_data *) malloc(sizeof(meta_data));

    sController.setMetadata(first_packet);

    cout << "SEND META DIR: " << first_packet->file_dir << endl;
    cout << "SEND META Name: " << first_packet->file_name << endl;
    cout << "SEND META Size: " << first_packet->file_size << endl;

    int ACK_len;
    while (true) {
        // NEED TIMEOUT
        if (meta_first_time || time_gap > TIMEOUT) {
            if (meta_first_time) meta_first_time = false;
            sendto(send_sock, first_packet, sizeof(meta_data), 0, (struct sockaddr *) &sender_sin, sender_sin_len);
            cout << "SENDER: Send META" << endl;
        }
        // Ready to receive ACK for the first packet
        ACK_len = recvfrom(send_sock, buff, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *) &sender_sin,
                           &sender_sin_len);
        if (ACK_len > 0) {
            // NEED CHECKSUM
            // Get ACK
            ack_packet *getFile = (ack_packet *) buff;
            bool finish = ntohs(getFile->finish);
            u_short ack_num = ntohs(getFile->ack);
            if (ack_num == MAX_SEQ_LEN - 1) {
                cout << "ACK: " << ack_num << endl;
                cout << "SENDER: Received ACK for META" << endl;
                break;
            }
            cout << "META SEND FAILED" << endl;
        }
    }
    free(first_packet);

    send_node *packet;
    while (true) {
        // 1. Send Packet
        if (sController.inWindow() && !sController.isAllSent()) {
            packet = sController.getPacket();
            cout << "SEQ NUM: " << packet->seq_num << endl;
            sendto(send_sock, packet->data, packet->packet_len, 0, (struct sockaddr *) &sender_sin, sender_sin_len);
        }
        // 2. Receive ACK
        ACK_len = recvfrom(send_sock, buff, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *) &sender_sin,
                           &sender_sin_len);
        if (ACK_len > 0) {
            // NEED CHECKSUM
            // Get ACK
            ack_packet *getFile = (ack_packet *) buff;
            bool isComplete = getFile->finish;
            u_short ack_num = ntohs(getFile->ack);


            // If out of window --- Ignore ACK
            // If in window:
            sController.updateWindow(ack_num);
            if (sController.isFinish()) {
                cout << "[completed] finish" << endl;
                break;
            }
        }

    }
    close(send_sock);
    return 0;
}

