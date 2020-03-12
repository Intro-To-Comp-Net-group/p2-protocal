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

#define BUFFER_SIZE 32768
#define WINDOW_SIZE 8
#define MAX_SEQ_LEN 2*WINDOW_SIZE

// Sender data packet configuration
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


struct meta_data {
    uint16_t file_size;
    string file_dir;
    string file_name;
};


struct send_node {
    uint16_t packet_len;
    uint16_t seq_num;
//    time_t send_time;
//    time_t recv_time;
    bool received_ack;
    char * data;
};


struct ack_packet {
    u_short crc;
    bool finish;
    u_short ack;
    uint32_t send_sec;
    uint32_t send_usec;
    char * padding;
};

struct window_node {
    bool isReceived;
    u_short seq_num;
    char * data;
};

bool check_file_existence(string file_path) {
    ifstream file(file_path);
    if (!file) return false;
    else {
        file.close();
        return true;
    }
}

class senderController {
public:
    explicit senderController(string file_dir_name);

    ~senderController();

    send_node * getPacket();

    void setMetadata(meta_data * firstPacket);

    void updateWindow(u_short ack_num);

    bool isAllSent();

    bool isFinish();

    bool inWindow();


private:
    vector<send_node *> window;

    string file_dir;
    string file_name;
    size_t file_path_len;
    long file_len;
    long curr_file_pos;
    bool isComplete;

    bool allSent;

    int curr_seq;
    int last_ack_seq;

    ifstream fp;
};

#endif //PROJECT2_SENDERCONTROLLER_H
