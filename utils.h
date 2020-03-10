//
// Created by Hanyi Wang on 3/6/20.
//

#ifndef PROJECT2_UTILS_H
#define PROJECT2_UTILS_H

#define BUFFER_SIZE 32768
#define WINDOW_SIZE 8
#define ACK_NOT_RECEIVED 0
#define ACK_RECEIVED 1
#define MAX_SEQ_LEN 2*WINDOW_SIZE
#define ACK_PACKET_LEN 24

#define PACKET_DATA_LEN 1024
#define PACKET_HEADER_LEN 4
#define CRC_POS 0
#define SEQ_POS 2

// ACK packet configuration
#define ACK_DATA_LEN 18
#define ACK_HEADER_LEN 6
#define ACK_CRC_POS 0
#define ACK_SEQ_POS 2
#define ACK_ACK_POS 4

using namespace std;




#endif //PROJECT2_UTILS_H
