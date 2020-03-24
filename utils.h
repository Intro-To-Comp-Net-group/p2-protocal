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
#include <time.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#define WINDOW_SIZE 128
#define MAX_SEQ_LEN 2*WINDOW_SIZE

#define PACKET_DATA_LEN 1024
#define PACKET_HEADER_LEN 3*sizeof(int) + sizeof(bool) + sizeof(unsigned short)
#define BUFFER_SIZE PACKET_DATA_LEN + PACKET_HEADER_LEN
#define MAX_FILE_PATH_LEN 64

#define ACK_BUFF_LEN 2*sizeof(int) + 2*sizeof(bool) + sizeof(unsigned short)

#define META_DATA_FLAG 65535
#define END_DATA_FLAG 65534


#define TIMEOUT 100000
using namespace std;


struct ack_packet {
    unsigned short checksum;
    int ack;
    int acc_ack;
    bool is_meta;
    bool is_last;
};

struct meta_data {
    unsigned short checksum;
    int seq_num;
    int file_len;
    int file_dir_name_len;
    char *file_dir_name;
};

struct data_packet {
    unsigned short checksum;
    int seq_num;
    int acc_seq_num;
    int packet_len;
    bool is_last_packet;
    char *data;
};

// Wrap packet's data and receive
struct receiver_window_node {
    bool isReceived;
    int packet_len;
    int seq_num;
    bool is_last;   // NEWADD
    char *data;
};

// Wrap packet and send
struct sender_window_node {
    bool is_received;
    bool is_send;
    int seq_num;
    bool is_last;
    char *packet;
    struct timeval send_time;
};

bool check_file_existence(string file_path) {
    ifstream file(file_path.c_str());
    if (!file) return false;
    else {
        file.close();
        return true;
    }
}

bool inWindow(int input_num, int last_action_num) {
    int next_num = last_action_num + 1;
    if (next_num + WINDOW_SIZE <= MAX_SEQ_LEN) {
        return input_num >= next_num && input_num < next_num + WINDOW_SIZE;
    } else {
        bool in_left = input_num >= next_num && input_num < next_num + WINDOW_SIZE;
        bool in_right = input_num >= 0 && input_num < (next_num + WINDOW_SIZE) % (MAX_SEQ_LEN);
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
    if (count > 0) sum += *addr;
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return (unsigned short) ~sum;
}

bool check_ack_checksum(char *ack_buff) {
    unsigned short received_ack_checksum = *(unsigned short *) (ack_buff);
    unsigned short cal_ack_checksum = get_checksum(ack_buff + sizeof(unsigned short),
                                                   ACK_BUFF_LEN - sizeof(unsigned short));
//    cout << "Received ack checksum is: " << received_ack_checksum << ", Calculated Checksum is: " << cal_ack_checksum << endl;
    return received_ack_checksum == cal_ack_checksum;
}

bool check_checksum(char *received_buff) {
    unsigned short recv_checksum = *(unsigned short *) received_buff;
    unsigned short cal_checksum = get_checksum(received_buff + sizeof(unsigned short),
                                               BUFFER_SIZE - sizeof(unsigned short));
//    cout << "Received packet checksum is: " << recv_checksum << ", Calculated Checksum is: " << cal_checksum << endl;
    return recv_checksum == cal_checksum;
}

bool check_meta_checksum(char *received_buff, int file_path_len) {
    unsigned short recv_checksum = *(unsigned short *) received_buff;
    unsigned short cal_checksum = get_checksum(received_buff + sizeof(unsigned short),
                                               3 * sizeof(int) + file_path_len);
//    cout << "Received meta checksum is: " << recv_checksum << ", Calculated Checksum is: " << cal_checksum << endl;
    return recv_checksum == cal_checksum;
}


#endif //PROJECT2_UTILS_H
