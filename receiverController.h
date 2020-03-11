//
// Created by Hanyi Wang on 3/8/20.
//

#ifndef PROJECT2PRE_RECEIVERCONTROLLER_H
#define PROJECT2PRE_RECEIVERCONTROLLER_H


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#define BUFFER_SIZE 32768
#define WINDOW_SIZE 8
#define ACK_NOT_RECEIVED 0
#define ACK_RECEIVED 1
#define MAX_SEQ_LEN 2*WINDOW_SIZE
#define ACK_PACKET_LEN 24

#define PACKET_DATA_LEN 20
#define PACKET_HEADER_LEN 4
#define CRC_POS 0
#define SEQ_POS 2

// ACK packet configuration
#define ACK_DATA_LEN 18
#define ACK_HEADER_LEN 6
#define ACK_CRC_POS 0
#define ACK_SEQ_POS 2
#define ACK_ACK_POS 4

#define TIMEOUT 5000
using namespace std;

struct ack_packet {
    u_short crc;
    bool finish;
    u_short ack;
    uint32_t send_sec;
    uint32_t send_usec;
    char * padding;
};

struct send_node {
    uint16_t packet_len;
    uint16_t seq_num;
    time_t send_time;
    time_t recv_time;
    bool received_ack;
    char * data;
};

struct meta_data {
    uint16_t file_size;
    string file_dir;
    string file_name;
};

struct window_node {
    bool isReceived;
    u_short seq_num;
    char * data;
};


void update_window(vector<window_node *> &window, bool *finish, int * curr_ack, int last_ack, ofstream &out_file) {
    // write data back to file, update curr_ack, update window
    int curr_idx = *curr_ack % WINDOW_SIZE;
    window_node * currNode = window[curr_idx];
    bool hasNext = currNode->isReceived;
    while (hasNext) {
        if (currNode->seq_num < last_ack) {
            out_file << currNode->data << flush;
        } else if (currNode->seq_num == last_ack) {
            cout << "Reach the last node" <<endl;
            out_file << currNode->data << flush;
        }

        currNode->isReceived = false;
        *curr_ack += 1;
        curr_idx = (curr_idx + 1) % WINDOW_SIZE;
        if (*curr_ack == last_ack) {
            *finish = true;
            break;
        }
        currNode = window[curr_idx];
        hasNext = currNode->isReceived;
    }
}

class receiverController {
public:
    receiverController();

    ack_packet * get_ack_packet();

private:
    ack_packet * meta_packet_ack;

};

#endif //PROJECT2PRE_RECEIVERCONTROLLER_H
