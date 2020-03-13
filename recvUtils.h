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

#define PACKET_DATA_LEN 40
#define PACKET_HEADER_LEN 2*sizeof(int) + sizeof(bool)
#define BUFFER_SIZE PACKET_DATA_LEN + PACKET_HEADER_LEN
#define WINDOW_SIZE 8
#define MAX_SEQ_LEN 2*WINDOW_SIZE


#define CRC_POS 0
#define SEQ_POS 2

#define META_DATA_FLAG -1


#define TIMEOUT 5000
using namespace std;

struct ack_packet {
    int ack;
    bool is_meta;
    bool finish;
//    uint32_t send_sec;
//    uint32_t send_usec;
};

struct meta_data {
    int seq_num;
    int file_len;
    string file_dir;
    string file_name;
};

struct data_packet {
    int seq_num;
    int packet_len;
    bool is_last_packet;
//    time_t send_time;
//    time_t recv_time;
    char * data;
};

struct receiver_window_node {
    bool isReceived;
    int seq_num;
    char * data;
};


void update_window(vector<receiver_window_node *> &window, bool *finish, int * curr_ack, int * last_ack, ofstream &out_file) {
    // write data back to file, update curr_ack, update window
    int curr_idx = *curr_ack % WINDOW_SIZE;
    receiver_window_node * currNode = window[curr_idx];
    bool hasNext = currNode->isReceived;
    while (hasNext) {
        out_file << currNode->data << flush;
        if (*curr_ack == *last_ack) {
            *finish = true;
            break;
        }
        *curr_ack += 1;
        curr_idx = (curr_idx + 1) % WINDOW_SIZE;
        currNode = window[curr_idx];
        hasNext = currNode->isReceived;
    }
}


bool inWindow(int curr_seq, int last_ack_seq) {
    int next_ack = last_ack_seq + 1;
    bool is_overflow = next_ack + WINDOW_SIZE > MAX_SEQ_LEN;
    bool in_range = curr_seq >= next_ack && curr_seq < next_ack + WINDOW_SIZE;
    if (!is_overflow) return in_range;
    else {
        bool in_window = false;
        if (curr_seq >= 0 && curr_seq < (next_ack + WINDOW_SIZE) % MAX_SEQ_LEN) {
            in_window = true;
        }
        return in_window || in_range;
    }
}
#endif //PROJECT2PRE_RECEIVERCONTROLLER_H
