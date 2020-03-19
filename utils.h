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

#define PACKET_DATA_LEN 1024 * 8
#define PACKET_HEADER_LEN 2*sizeof(int) + sizeof(bool) + sizeof(unsigned short)
#define BUFFER_SIZE PACKET_DATA_LEN + PACKET_HEADER_LEN
#define MAX_FILE_PATH_LEN 64

#define WINDOW_SIZE 48
#define MAX_SEQ_LEN 2*WINDOW_SIZE

#define ACK_BUFF_LEN sizeof(int)+sizeof(bool) + sizeof(unsigned short)

#define META_DATA_FLAG 65535


#define TIMEOUT 5000
using namespace std;


struct ack_packet {
    int ack;
    bool is_meta;
    unsigned short checksum;
//    uint32_t send_sec;
//    uint32_t send_usec;
};

struct meta_data {
    int seq_num;
    int file_len;
    int file_dir_name_len;
    unsigned short checksum;
    char *file_dir_name;
};

struct data_packet {
    int seq_num;
    int packet_len;
    bool is_last_packet;
    unsigned short checksum;
//    time_t send_time;
//    time_t recv_time;
    char *data;
};

// Wrap packet's data and receive
struct receiver_window_node {
    bool isReceived;
    int seq_num;
    bool is_last;   // NEWADD
    char *data;
};

// Wrap packet and send
struct sender_window_node {
    bool is_received;
    int seq_num;
    bool is_last;
    char *packet;
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

void
update_window(vector<receiver_window_node *> &window, bool *finish, int *curr_ack, int *last_ack, ofstream &out_file) {
    // write data back to file, update curr_ack, update window
    int curr_idx = *curr_ack % WINDOW_SIZE;
    receiver_window_node *currNode = window[curr_idx];
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

unsigned short get_checksum(char *addr, int count) {
    int sum = 0;
    while (count > 1) {
        sum = sum + *(unsigned short *) addr;
        addr += 2;
        count -= 2;
    }
    if (count > 0) sum += *addr;  //=1,说明count为奇数
    while (sum >> 16) {//当和的高16位不为0，把高16位作为校验和的一部分求和，
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return (unsigned short) ~sum;
}

bool check_ack_checksum(char *ack_buff) {
    unsigned short received_ack_checksum = *(unsigned short *) (ack_buff + sizeof(int) + sizeof(bool));
    unsigned short cal_ack_checksum = get_checksum(ack_buff, sizeof(int));
//    return received_ack_checksum == cal_ack_checksum;
    return true;
}

bool check_checksum(char *received_buff) {
    unsigned short recv_checksum = *(unsigned short *) (received_buff + 2 * sizeof(int) + sizeof(bool));
    unsigned short cal_checksum = get_checksum(received_buff + PACKET_HEADER_LEN, PACKET_DATA_LEN);
    cout << "Received meta checksum is: " << recv_checksum << ", Calculated Checksum is: " << cal_checksum << endl;
    return recv_checksum == cal_checksum;
//    return true;
}

bool check_meta_checksum(char *received_buff, int file_path_len) {
    unsigned short recv_checksum = *(unsigned short *) (received_buff + 3 * sizeof(int));
    unsigned short cal_checksum = get_checksum(received_buff + 3 * sizeof(int) + sizeof(unsigned short), file_path_len);
    cout << "Received meta checksum is: " << recv_checksum << ", Calculated Checksum is: " << cal_checksum << endl;
    return recv_checksum == cal_checksum;
//    return true;
}


#endif //PROJECT2_UTILS_H
