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
    fp.open(file_dir_name.c_str(), ios::binary | ios::in);
    fp.seekg(0, std::ios::end);
    this->file_len = fp.tellg();
    fp.seekg(0, std::ios::beg);  // set the file to the beginning
    curr_file_pos = 0;

    allSent = false;
    isComplete = false;

    last_ack_seq = -1;
    curr_seq = 0;

    // Prepare sliding window
    for (int i = 0; i < WINDOW_SIZE; i++) {
        struct send_node * curr_packet = new send_node();
        curr_packet->received_ack = false;
        curr_packet->data = (char*) malloc(PACKET_DATA_LEN);    // CHECK
        window.push_back(curr_packet);
    }
}

senderController::~senderController() {
    for (int i = 0; i < WINDOW_SIZE; i++) {
        struct send_node * curr_packet = window[i];
        free(curr_packet->data);
        curr_packet->data = NULL;
    }
    fp.close();
}

send_node *senderController::getPacket() {
    send_node * packet = window[curr_seq % WINDOW_SIZE];
    long data_left_len = file_len - curr_file_pos;
    if (data_left_len <= PACKET_DATA_LEN) {
        fp.read(packet->data, data_left_len);
        packet->packet_len = data_left_len;
        curr_file_pos += data_left_len;
        allSent = true;
    } else {
        fp.read(packet->data, PACKET_DATA_LEN);
        packet->packet_len = PACKET_DATA_LEN;
        curr_file_pos += PACKET_DATA_LEN;
    }

    packet->received_ack = false;
    packet->seq_num = curr_seq;
    curr_seq += 1;

    return packet;
}

void senderController::setMetadata(meta_data * firstPacket) {
    firstPacket->file_size = this->file_len;
    firstPacket->file_dir = this->file_dir;
    firstPacket->file_name = this->file_name;
}

void senderController::updateWindow(u_short ack_num) {
    if (ack_num <= last_ack_seq || ack_num > last_ack_seq + WINDOW_SIZE) return;
    send_node * node = window[ack_num % WINDOW_SIZE];
    if (node->received_ack) {
        cout << "[recv ack] "<< (ack_num-1)*PACKET_DATA_LEN <<" IGNORED (duplicate)" << endl;
    } else {
        cout << "[recv ack] "<< (ack_num-1)*PACKET_DATA_LEN <<" ACCEPTED" << endl;
    }
    node->received_ack = true;

    // update last_ack
    send_node * check_node = window[(last_ack_seq + 1) % WINDOW_SIZE];
    bool hasNext = check_node->received_ack;
    while (hasNext) {
        last_ack_seq += 1;
        if (last_ack_seq == file_len) {
            isComplete = true;
            return;
        }
        check_node = window[(last_ack_seq + 1) % WINDOW_SIZE];
        hasNext = check_node->received_ack;
    }
    return;
}

bool senderController::isAllSent() {
    return allSent;
}

bool senderController::isFinish() {
    return isComplete;
}

bool senderController::inWindow() {
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
