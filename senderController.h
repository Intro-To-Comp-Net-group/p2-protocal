//
// Created by Hanyi Wang on 3/8/20.
//

#ifndef PROJECT2_SENDERCONTROLLER_H
#define PROJECT2_SENDERCONTROLLER_H

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

#define PACKET_DATA_LEN 40
#define PACKET_HEAD_LEN 2*sizeof(int) + sizeof(bool)
#define BUFFER_SIZE PACKET_DATA_LEN + PACKET_HEAD_LEN
#define WINDOW_SIZE 8
#define MAX_SEQ_LEN 2*WINDOW_SIZE


#define PACKET_HEADER_LEN 4
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
    int seq;
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

struct sender_window_node {
    bool is_received;
    int seq_num;
    bool is_last;
};

bool check_file_existence(string file_path) {
    ifstream file(file_path);
    if (!file) return false;
    else {
        file.close();
        return true;
    }
}

//class senderController {
//public:
//    explicit senderController(string file_dir_name);
//
//    ~senderController();
//
//    send_node * getPacket();
//
//    void setMetadata(meta_data * firstPacket);
//
//    void updateWindow(u_short ack_num);
//
//    bool isAllSent();
//
//    bool isFinish();
//
//    bool inWindow();
//
//
//private:
//    vector<send_node *> window;
//
//    string file_dir;
//    string file_name;
//    size_t file_path_len;
//    long file_len;
//    long curr_file_pos;
//    bool isComplete;
//
//    bool allSent;
//
//    int curr_seq;
//    int last_ack_seq;
//
//    ifstream fp;
//};

#endif //PROJECT2_SENDERCONTROLLER_H
