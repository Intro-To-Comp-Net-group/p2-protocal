//
// Created by Hanyi Wang on 3/6/20.
//

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <vector>
#include "utils.h"
using namespace std;

int main(int argc, char **argv) {
    if (argc != 5) {
        perror("You have to input 5 arguments! \n");
        exit(0);
    }

    // Get parameters 1
    char *command_to_check = argv[1];
    if (strcmp(command_to_check, "-r") != 0) {
        perror("Invalid command 1, it should be -r! \n");
        exit(0);
    }

    // Get parameters 3
    command_to_check = argv[3];
    if (strcmp(command_to_check, "-f") != 0) {
        perror("Invalid command 3, it should be -f! \n");
        exit(0);
    }

    // Get parameters 2
    int find_pos = -1;
    string recv_host_port = string(argv[2]);
    find_pos = recv_host_port.find(':');
    if (recv_host_port.empty() || find_pos == -1) {
        perror("Invalid recv_host_port input type! \n");
        exit(0);
    }
    string recv_host = recv_host_port.substr(0, find_pos);
    struct hostent *host = gethostbyname(recv_host.c_str());
    uint16_t recv_port = atoi(recv_host_port.substr(find_pos + 1).c_str());
    if (recv_port < 18000 || recv_port > 18200) {
        perror("Receiver port number should be within 18000 and 18200! \n");
        exit(0);
    }

    // Get parameters 4
    string file_dir_name = string(argv[4]);
    find_pos = file_dir_name.find('/');
    if (file_dir_name.empty() || find_pos == -1) {
        perror("Invalid file_dir_name input type! \n");
        exit(0);
    }
    // check input file existence
    if (!check_file_existence(file_dir_name)) {
        perror("File not exists! \n");
        exit(0);
    }
    string file_dir = file_dir_name.substr(0, find_pos);
    string file_name = file_dir_name.substr(find_pos + 1);

    // Open File
    ifstream inFile;
    inFile.open(file_dir_name.c_str(), ios::binary | ios::in);
    inFile.seekg(0, std::ios::end);
    long file_len = inFile.tellg();
    inFile.seekg(0, std::ios::beg);  // set the file to the beginning
    long curr_file_pos = 0;

    // Create socket
    int send_sock = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket
    if (send_sock < 0) {
        perror("Unable to create socket! \n");
        exit(1);
    }

    struct sockaddr_in sender_sin;
    memset(&sender_sin, 0, sizeof(sender_sin));
    sender_sin.sin_family = AF_INET;
    sender_sin.sin_addr.s_addr = *(unsigned int *) host->h_addr_list[0];
    sender_sin.sin_port = htons(recv_port);
    socklen_t sender_sin_len = sizeof(sender_sin);

    bind(send_sock, (struct sockaddr *) &sender_sin, sizeof(sockaddr));
    fcntl(send_sock, F_SETFL, O_NONBLOCK);  // Non blocking mode

    char buff[BUFFER_SIZE];

    // Start sending Meta Data
    char meta_buff[PACKET_DATA_LEN + sizeof(int) + sizeof(bool)];
    meta_data *meta_packet = (meta_data *) meta_buff;
    meta_packet->seq_num = META_DATA_FLAG;
    meta_packet->file_len = file_len;
    meta_packet->file_name = file_name;
    meta_packet->file_dir = file_dir;
    meta_packet->checksum = get_checksum(meta_buff+ sizeof(int)*2 + sizeof(unsigned short), PACKET_DATA_LEN);

    cout << "SEND META: SEQ: " << meta_packet->seq_num << " LEN: "<< meta_packet->file_len << " DIR: "
         << meta_packet->file_dir << " Name: " << meta_packet->file_name << endl;

    int curr_seq = 0;
    int last_ack_num = -1;

    struct timeval timestamp;
    struct timeval meta_time;
    int time_gap;
    bool meta_first_time = true;

    gettimeofday(&timestamp, NULL);
    uint32_t start_sec = timestamp.tv_sec;
    uint32_t start_usec = timestamp.tv_usec;

    char ack_buff[ACK_BUFF_LEN];
    int received_ack_len;
    while (true) {
        // Need Timeout
        gettimeofday(&meta_time,NULL);
        time_gap = (meta_time.tv_sec-start_sec)*1000000+(meta_time.tv_usec - start_usec);
        if (meta_first_time || time_gap > TIMEOUT) {
            if (meta_first_time) meta_first_time = false;
            sendto(send_sock, meta_buff, sizeof(meta_buff), 0, (struct sockaddr *) &sender_sin, sender_sin_len);
            cout << "SEND META DATA" << endl;
        }
        received_ack_len = recvfrom(send_sock, ack_buff, sizeof(ack_buff), MSG_DONTWAIT,(struct sockaddr*)&sender_sin, &sender_sin_len);
        if (received_ack_len > 0) {
            bool is_meta = *(bool *) (ack_buff + sizeof(int));
            if (is_meta) {
                cout << "META ACK RECEIVED" << endl;
                break;
            }
        }
    }

    // Init window
    vector<sender_window_node *> window;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sender_window_node * node = (sender_window_node *) malloc(sizeof(sender_window_node));  // Do we need it?
        node->packet = (char *) malloc(BUFFER_SIZE * sizeof(char));
        node->is_last = false;
        node->is_received = false;
        window.push_back(node);
    }

    bool isAllSent = false;

    while (true) {
        // RECEIVE
        received_ack_len = recvfrom(send_sock, ack_buff, sizeof(ack_buff), MSG_DONTWAIT,(struct sockaddr*)&sender_sin, &sender_sin_len);
        if (received_ack_len > 0) {
            // Extract info of received ACK_packet
            int ack = *(int*) ack_buff;
            cout << "RECEIVED ACK PACKET " << ack << " LAST ACK: " << last_ack_num +1<<  endl;
            if (inWindow(ack, last_ack_num)) {
                if (ack == last_ack_num + 1) {
                    last_ack_num += 1;
                    if (last_ack_num == MAX_SEQ_LEN - 1) {
                        last_ack_num = -1;
                    }
                }
            }
            if (isAllSent && ack == last_ack_num) break;
        }

        // SEND
        sender_window_node * node_to_send;
        if (!isAllSent && inWindow(curr_seq, last_ack_num)) {
            node_to_send = window[curr_seq % WINDOW_SIZE];
            memset(node_to_send->packet, 0, BUFFER_SIZE * sizeof(char));
            long pending_len = file_len - curr_file_pos;
            cout <<"FILE LEN: " << file_len <<" CURRENT POSITION: " << curr_file_pos << " PENDING: " << pending_len << endl;
            // Create data packet
            if (pending_len <= PACKET_DATA_LEN) {
                node_to_send->is_last = true;
                node_to_send->seq_num = curr_seq;
                // Last node
                *(int *) (node_to_send->packet) = curr_seq;   // Seq_num
                *(int *) (node_to_send->packet + sizeof(int)) = pending_len;   // Packet_len
                *(bool *) (node_to_send->packet + 2*sizeof(int)) = true;   // is_last_packet
                inFile.read(node_to_send->packet + PACKET_HEADER_LEN,pending_len);
                *(unsigned short *) (node_to_send->packet + 2 * sizeof(int) + sizeof(bool)) =
                        get_checksum(node_to_send->packet + PACKET_HEADER_LEN, pending_len);
                curr_file_pos += pending_len;
                if (curr_file_pos == file_len) {
                    isAllSent = true;
                }
            } else {
                node_to_send->is_last = false;
                node_to_send->seq_num = curr_seq;
                // Internal Node
                *(int *) (node_to_send->packet) = curr_seq;   // Seq_num
                *(int *) (node_to_send->packet + sizeof(int)) = PACKET_DATA_LEN;   // Packet_len
                *(bool *) (node_to_send->packet + 2*sizeof(int)) = false;   // is_last_packet
                inFile.read(node_to_send->packet + PACKET_HEADER_LEN, PACKET_DATA_LEN);
                *(unsigned short *) (node_to_send->packet + 2 * sizeof(int) + sizeof(bool)) =
                        get_checksum(node_to_send->packet + PACKET_HEADER_LEN, PACKET_DATA_LEN);
                isAllSent = false;
                curr_file_pos += PACKET_DATA_LEN;
            }

            ////TODO: TIMEOUT

            // Send data packet
            memcpy(buff, node_to_send->packet, BUFFER_SIZE);
            int show2 = *(int *) buff;
            int show_len2 = *(int *) (buff + sizeof(int));
            bool show_last2 = *(bool *) (buff + 2*sizeof(int));
            unsigned short show_checksum = *(unsigned short *) (buff + 2*sizeof(int) + sizeof(bool));
            cout << "SEQ NUM: " << curr_seq << " SEND SEQUENCE NUM:" << show2 << " LEN: "<< show_len2 << " LAST?: " << show_last2 << " checksum " << show_checksum <<endl;
            if (sendto(send_sock, buff, BUFFER_SIZE, 0, (struct sockaddr *) &sender_sin, sender_sin_len)>0) {
                curr_seq += 1;
                if (curr_seq == MAX_SEQ_LEN) curr_seq = 0;
            }
        }
    }

    inFile.close();
    close(send_sock);
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sender_window_node * node = window[i];
        free(node->packet);
        free(node);
    }
    return 0;
}