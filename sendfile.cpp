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

//#include <chrono>
//#include <thread>

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
    int file_len = inFile.tellg();
    inFile.seekg(0, std::ios::beg);  // set the file to the beginning
    int curr_file_pos = 0;

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
    int file_path_len = strlen(file_dir_name.c_str());
    // Start sending Meta Data
    char meta_buff[MAX_FILE_PATH_LEN + 2 * sizeof(int) + sizeof(int) + sizeof(int)];

    cout << " TOTAL FILE PATH LEN " << file_path_len << " FILE PATH NAME IS " << file_dir_name.c_str()
         << " FILE LEN IS " << strlen(file_dir_name.c_str()) << endl;

    // TODO: Creating checksum for meta_data
    memcpy(meta_buff + 3 * sizeof(int) + sizeof(unsigned short), file_dir_name.c_str(), MAX_FILE_PATH_LEN);
    *(int *) meta_buff = META_DATA_FLAG;
    *(int *) (meta_buff + sizeof(int)) = file_len;
    *(int *) (meta_buff + 2 * sizeof(int)) = file_path_len;
    unsigned short meta_checksum = get_checksum(meta_buff + 3 * sizeof(int) + sizeof(unsigned short), file_path_len);
    *(unsigned short *) (meta_buff + 3 * sizeof(int)) = meta_checksum;

    int curr_seq = 0;
    int last_ack_num = -1;

    struct timeval timestamp;
    struct timeval meta_time;
    int time_gap;
    bool meta_first_time = true;

    gettimeofday(&meta_time, NULL);

//    char buff[BUFFER_SIZE];   // NEED IT?
    char ack_buff[ACK_BUFF_LEN];
    int received_ack_len;
    while (true) {
        // TODO Timeout
        gettimeofday(&timestamp, NULL);
        time_gap = (timestamp.tv_sec - meta_time.tv_sec) * 1000000 + (timestamp.tv_usec - meta_time.tv_usec);
        if (meta_first_time && time_gap > TIMEOUT) {
            if (meta_first_time) meta_first_time = false;
            sendto(send_sock, meta_buff, sizeof(meta_buff), 0, (struct sockaddr *) &sender_sin, sender_sin_len);
            cout << "META DATA SENT" << endl;
            gettimeofday(&meta_time, NULL);
        }
        received_ack_len = recvfrom(send_sock, ack_buff, sizeof(ack_buff), MSG_DONTWAIT,
                                    (struct sockaddr *) &sender_sin, &sender_sin_len);
        if (received_ack_len > 0) {
            bool is_meta = *(bool *) (ack_buff + sizeof(int));
            if (is_meta && check_ack_checksum(ack_buff)) {
                cout << "META ACK RECEIVED" << endl;
                break;
            } else {
                // Fast re-send
                sendto(send_sock, meta_buff, sizeof(meta_buff), 0, (struct sockaddr *) &sender_sin, sender_sin_len);
                cout << "META DATA RESEND" << endl;
                gettimeofday(&meta_time, NULL);
            }
        }
    }

    // Init window
    vector<sender_window_node *> window;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sender_window_node *node = (sender_window_node *) malloc(
                BUFFER_SIZE + sizeof(bool) + sizeof(int) + sizeof(bool));  // Do we need it?
        node->packet = (char *) malloc(BUFFER_SIZE * sizeof(char));
        node->is_last = false;
        node->is_received = false;
        gettimeofday(&node->send_time, NULL);
        window.push_back(node);
    }

    bool isAllSent = false;
    bool lastReceived = false;
    int LAST_ACK_FLAG = -1;

    int send_count = 0;
    int recv_count = 0;

    bool endFlag = false;

    while (true) {
        // RECEIVE
        received_ack_len = recvfrom(send_sock, ack_buff, sizeof(ack_buff), MSG_DONTWAIT,
                                    (struct sockaddr *) &sender_sin, &sender_sin_len);
        if (received_ack_len > 0) {
            // Extract info of received ACK_packet
            int ack = *(int *) ack_buff;
            bool received_last_ack = *(bool *) (ack_buff + sizeof(int) + sizeof(bool) + sizeof(unsigned short));

            if (received_last_ack) {
                lastReceived = true;
                LAST_ACK_FLAG = ack;
            }

            if (!check_ack_checksum(ack_buff)) {
                continue;
            }

            if (inWindow(ack, last_ack_num)) {
                recv_count += 1;

                sender_window_node *node = window[ack % (WINDOW_SIZE)];
                node->is_received = true;

                int tmp_q_head = last_ack_num;
                while (window[(last_ack_num + 1) % (WINDOW_SIZE)]->is_received && inWindow(ack, tmp_q_head)) {
                    window[(last_ack_num + 1) % (WINDOW_SIZE)]->is_received = false;
                    window[(last_ack_num + 1) % (WINDOW_SIZE)]->is_send = false;
                    last_ack_num += 1;
                    if (last_ack_num == MAX_SEQ_LEN - 1) {
                        last_ack_num = -1;
                    }

                    if (isAllSent && lastReceived) {
                        if ((LAST_ACK_FLAG == MAX_SEQ_LEN - 1 && last_ack_num == -1) ||
                            (LAST_ACK_FLAG == last_ack_num)) {
                            endFlag = true;
                            break;
                        }
                    }
                }
                if (endFlag) {
                    break;
                }

            }
            cout << "RECEIVED ACK PACKET " << ack << " head of queue: " << last_ack_num + 1 << " isallsent? "
                 << isAllSent << " lastACK received? " << received_last_ack << endl;
//            if (isAllSent && received_last_ack) {
//                cout << endl;
//                cout << LAST_ACK_FLAG << " AND " << last_ack_num <<endl;
//                cout << endl;
//                if ((LAST_ACK_FLAG == MAX_SEQ_LEN - 1 && last_ack_num == -1) ||
//                    (LAST_ACK_FLAG == last_ack_num))
//                    break;
//            }

        }

        // SEND
        // find the head of the send window

        // timeout resend
        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);
        for (int i = 0; i < WINDOW_SIZE; i++) {
            if (window[i]->is_received == false && window[i]->is_send) {
                int time_diff = (cur_time.tv_sec - window[i]->send_time.tv_sec) * 1000000 +
                                (cur_time.tv_usec - window[i]->send_time.tv_usec);
                if (time_diff > TIMEOUT) {

                    if (sendto(send_sock, window[i]->packet, BUFFER_SIZE, 0, (struct sockaddr *) &sender_sin,
                               sender_sin_len) > 0) {
                        gettimeofday(&window[i]->send_time, NULL);
                        cout << "Resend: seq_num: " << *(int *) window[i]->packet << endl;
                    } else {
                        cout << "SEND FAIL " << endl;
                    }
                }
            }
        }

        sender_window_node *node_to_send;
        if (!isAllSent && inWindow(curr_seq, last_ack_num)) {
            send_count += 1;

            node_to_send = window[curr_seq % (WINDOW_SIZE)];
            memset(node_to_send->packet, 0, BUFFER_SIZE * sizeof(char));
            int pending_len = file_len - curr_file_pos;
//            cout << "FILE LEN: " << file_len << " CURRENT POSITION: " << curr_file_pos << " PENDING: " << pending_len<< endl;
            // Create packet
            if (pending_len <= PACKET_DATA_LEN) {
                node_to_send->is_last = true;
                node_to_send->seq_num = curr_seq;

                *(int *) (node_to_send->packet) = curr_seq;   // Seq num
                *(int *) (node_to_send->packet + sizeof(int)) = pending_len;   // Packet_len
                *(bool *) (node_to_send->packet + 2 * sizeof(int)) = true;   // is_last_packet
                inFile.read(node_to_send->packet + PACKET_HEADER_LEN, pending_len);
                *(unsigned short *) (node_to_send->packet + 2 * sizeof(int) + sizeof(bool)) =
                        get_checksum(node_to_send->packet + PACKET_HEADER_LEN, pending_len);

                gettimeofday(&node_to_send->send_time, NULL);

                curr_file_pos += pending_len;
                if (curr_file_pos == file_len) {
                    isAllSent = true;
                }
            } else {
                node_to_send->is_last = false;
                node_to_send->seq_num = curr_seq;

                *(int *) (node_to_send->packet) = curr_seq;   // Seq num
                *(int *) (node_to_send->packet + sizeof(int)) = PACKET_DATA_LEN;   // Packet_len
                *(bool *) (node_to_send->packet + 2 * sizeof(int)) = false;   // is_last_packet
                inFile.read(node_to_send->packet + PACKET_HEADER_LEN, PACKET_DATA_LEN);
                *(unsigned short *) (node_to_send->packet + 2 * sizeof(int) + sizeof(bool)) =
                        get_checksum(node_to_send->packet + PACKET_HEADER_LEN, PACKET_DATA_LEN);


                gettimeofday(&node_to_send->send_time, NULL);

                isAllSent = false;
                curr_file_pos += PACKET_DATA_LEN;
            }


            // Send data packet
//            memcpy(buff, node_to_send->packet, BUFFER_SIZE);    // NEED IT?

            // DEBUG PRINT LOG
            int show2 = *(int *) node_to_send->packet;
            int show_len2 = *(int *) (node_to_send->packet + sizeof(int));
            bool show_last2 = *(bool *) (node_to_send->packet + 2 * sizeof(int));
            unsigned short show_checksum = *(unsigned short *) (node_to_send->packet + 2 * sizeof(int) + sizeof(bool));
            cout << "SEQ NUM: " << curr_seq << " SEND SEQUENCE NUM:" << show2 << " LEN: " << show_len2 << " LAST?: "
                 << show_last2 << endl;

            if (sendto(send_sock, node_to_send->packet, BUFFER_SIZE, 0, (struct sockaddr *) &sender_sin,
                       sender_sin_len) > 0) {
//                curr_seq = (curr_seq + 1) % (MAX_SEQ_LEN);
                node_to_send->is_send = true;

                curr_seq += 1;
                if (curr_seq == MAX_SEQ_LEN) {
                    curr_seq = 0;
                }
            } else cout << "SEND FAIL " << endl;

        }
    }

    // TODO DEAL WITH THE ENDING
    cout << "Send " << send_count << " data packet and receive: " << recv_count << " ack" << endl;

    cout << "[send complete!]" << endl;
    inFile.close();
    close(send_sock);
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sender_window_node *node = window[i];
        free(node->packet);
        free(node);
    }
    return 0;
}