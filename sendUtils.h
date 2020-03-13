//
// Created by Hanyi Wang on 3/8/20.
//

#ifndef PROJECT2_SENDUTILS_H
#define PROJECT2_SENDUTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#define PACKET_DATA_LEN 150
#define PACKET_HEADER_LEN 2*sizeof(int) + sizeof(bool)
#define BUFFER_SIZE PACKET_DATA_LEN + PACKET_HEADER_LEN
#define WINDOW_SIZE 16
#define MAX_SEQ_LEN 2*WINDOW_SIZE


#define CRC_POS 0
#define SEQ_POS 2

#define META_DATA_FLAG -1


#define TIMEOUT 5000


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

struct sender_window_node {
    bool is_received;
    int seq_num;
    bool is_last;
    char * packet;
};

bool check_file_existence(string file_path) {
    ifstream file(file_path);
    if (!file) return false;
    else {
        file.close();
        return true;
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

#endif //PROJECT2_SENDUTILS_H
