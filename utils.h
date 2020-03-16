//
// Created by Hanyi Wang on 3/13/20.
//

#ifndef PROJECT2_UTILS_H
#define PROJECT2_UTILS_H

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

#define PACKET_DATA_LEN 1024*1024 / WINDOW_SIZE
#define PACKET_HEADER_LEN 2*sizeof(int) + sizeof(bool) + sizeof(unsigned short)
#define BUFFER_SIZE PACKET_DATA_LEN + PACKET_HEADER_LEN

#define WINDOW_SIZE 512
#define MAX_SEQ_LEN 2*WINDOW_SIZE

#define ACK_BUFF_LEN sizeof(int)+sizeof(bool)*2

#define META_DATA_FLAG 65535


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
    long file_len;
    int file_dir_name_len;
    unsigned short checksum;
//    string file_dir;
//    string file_name;

    char * file_dir_name;
};

struct data_packet {
    int seq_num;
    int packet_len;
    bool is_last_packet;
    unsigned short checksum;
//    time_t send_time;
//    time_t recv_time;
    char * data;
};

// Wrap packet's data and receive
struct receiver_window_node {
    bool isReceived;
    int seq_num;
    bool is_last;   // NEWADD
    char * data;
};

// Wrap packet and send
struct sender_window_node {
    bool is_received;
    int seq_num;
    bool is_last;
    char * packet;
//    char packet[BUFFER_SIZE];
};

bool check_file_existence(string file_path) {
    ifstream file(file_path.c_str());
    if (!file) return false;
    else {
        file.close();
        return true;
    }
}

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

bool inWindow(int input_num, int last_action_num) {
    int next_num = last_action_num + 1;
    if (next_num + WINDOW_SIZE <= MAX_SEQ_LEN) {
        return input_num >= next_num && input_num < next_num + WINDOW_SIZE;
    } else {
        bool in_left = input_num >= next_num && input_num < next_num + WINDOW_SIZE;
        bool in_right = input_num >= 0 && input_num < (next_num + WINDOW_SIZE) % MAX_SEQ_LEN;
        return in_left || in_right;
    }
}

unsigned short get_checksum(char * addr, int size) {
    return 1;
}

bool check_checksum(char * received_buff, bool is_meta) {
    unsigned short real_checksum;
    unsigned short calculated_checksum;
    if (!is_meta) {
        real_checksum = *(unsigned short*) (received_buff + 2* sizeof(int) + sizeof(bool));
//        unsigned short calculated_checksum = get_checksum(received_buff + PACKET_HEADER_LEN, PACKET_DATA_LEN);
        calculated_checksum = get_checksum(received_buff + PACKET_HEADER_LEN, PACKET_DATA_LEN);
    } else {
        real_checksum = *(unsigned short*) (received_buff + 2 * sizeof(int));
        calculated_checksum = get_checksum(received_buff+sizeof(int)*2, PACKET_DATA_LEN);
    }
    return real_checksum == calculated_checksum;
}



#endif //PROJECT2_UTILS_H
