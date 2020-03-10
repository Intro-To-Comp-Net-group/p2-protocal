//
// Created by Hanyi Wang on 3/8/20.
//

#include "senderController.h"

senderController::senderController(string file_dir_name) {
    int find_pos = file_dir_name.find('/');
    string file_dir = file_dir_name.substr(0, find_pos);
    string file_name = file_dir_name.substr(find_pos + 1);

    this->file_dir = file_dir;
    this->file_name = file_name;
    fp = fopen(file_dir_name.c_str(),"r");
    if (fp == nullptr) {
        perror("Cannot open file \n");
        exit(0);
    }
    // Get the length of file
    fseek(fp,0L,SEEK_END);
    this->file_len = ftell(fp);
    fseek(fp,0L,SEEK_SET);  // set the file to the beginning
//    curr_file_pos = 0;
//    finish = false;
//
//    last_ack_seq = -1;
//    curr_seq = 0;

    // Prepare sliding window
    window = (send_packet *) malloc(WINDOW_SIZE);
    for (int i = 0; i < WINDOW_SIZE; i++) {
        struct send_packet * curr_packet = &window[i];
        curr_packet->type = ACK_NOT_RECEIVED;
        curr_packet->data = (char*) malloc(PACKET_HEADER_LEN + PACKET_DATA_LEN);    // CHECK
    }
}

senderController::~senderController() {
    for (int i = 0; i < WINDOW_SIZE; i++) {
        struct send_packet * curr_packet = &window[i];
        free(curr_packet->data);
        curr_packet->data = NULL;
    }
    free(window);
    window = NULL;
}

send_packet *senderController::getPacket() {
    return nullptr;
}

void senderController::setMetadata(meta_data * firstPacket) {
    firstPacket->file_size = this->file_len;
    firstPacket->file_dir = this->file_dir;
    firstPacket->file_name = this->file_name;
}
