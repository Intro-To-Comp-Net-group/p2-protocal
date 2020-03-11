//
// Created by Hanyi Wang on 3/6/20.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include "iostream"
#include "receiverController.h"


using namespace std;

int main(int argc, char** argv) {
    if (argc != 3) {
        perror("You have to input 3 arguments! \n");
        exit(0);
    }

    // Get parameters 1
    char *command_to_check = argv[1];
    if (strcmp(command_to_check, "-p") != 0) {
        perror("Invalid command 1, it should be -p! \n");
        exit(0);
    }

    // Get parameters 2
    char * recv_port_str = argv[2];
    if (!recv_port_str) {
        perror("Invalid recv_port input! \n");
        exit(0);
    }
    int recv_port = atoi(recv_port_str);
    if (recv_port < 18000 || recv_port > 18200) {
        perror("Receiver port number should be within 18000 and 18200! \n");
        exit(0);
    }

    int server_sock = socket(AF_INET,SOCK_DGRAM,0);
    if (server_sock < 0) {
        perror("Unable to create socket! \n");
        exit(1);
    }

    struct sockaddr_in server_sin, client_sin;
    socklen_t client_len = sizeof(client_sin);

    memset(&server_sin, 0, sizeof(server_sin));
    server_sin.sin_family = AF_INET;
    server_sin.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sin.sin_port = htons(recv_port);

    if (::bind(server_sock, (struct sockaddr *) &server_sin, sizeof(server_sin)) < 0) { // make clear that it's in the global namespace
        perror("binding socket to address error!! \n");
        exit(1);
    }

    char *buff[BUFFER_SIZE + 16];
    ack_packet * ack = (ack_packet *)malloc(16*sizeof(char));
//    uint16_t last_packet_seq;
//    uint16_t curr_ack = 0;

    int last_packet_seq;
    int curr_ack = 0;
    bool isComplete = false;
    string file_dir, file_name;

    // Start listening
    // First, receive the metadata (first packet)
    while (true) {
        int recv_packet_length = recvfrom(server_sock, buff, sizeof(meta_data),0,(struct sockaddr*) &client_sin, &client_len);
        if (recv_packet_length <= 0) continue;

        meta_data * file_received = (meta_data *) buff;
        last_packet_seq = file_received->file_size;
        file_dir = file_received->file_dir;
        file_name = file_received->file_name;
        cout << "File Size Received:" << file_received->file_size<< endl;
        cout << "File Name Received:" << file_received->file_name<< endl;
        cout << "File Dir Received:" << file_received->file_dir << endl;

        // NEED CHECKSUM
        cout << "RECEIVER: GET META" << endl;
        ack->seq_num = htons(0);
        ack->ack = htons(MAX_SEQ_LEN - 1);
        sendto(server_sock, ack, 16, 0,(struct sockaddr*) &client_sin, client_len);
        cout << "RECEIVER: Send ACK" << endl;
        break;
    }

    // Sliding Window
    vector<window_node *> window;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        window_node * node = new window_node();
        node->isReceived = false;
        node->data = (char *) malloc(PACKET_DATA_LEN);
        window.push_back(node);
    }

    cout << "End of receiving metadata, start receiving packets" << endl;
    ofstream outFile(file_dir + "/" + file_name + ".recv", ios::out | ios::binary);
//    outFile << "Let's start!"<< flush;
//    outFile << "Hey Man!"<< flush;
    window_node * received_packet;
    while (true) {
        memset(buff, 0, BUFFER_SIZE + 16);
        int recv_packet_length = recvfrom(server_sock,buff, BUFFER_SIZE + 16, 0, (struct sockaddr*) &client_sin, &client_len);
        if (recv_packet_length > 0) {
            cout << "RECEIVER: PACKET RECEIVED" << endl;
            // Get seq_num of packet received
            received_packet = (window_node *) buff;
            uint16_t seq_num = ntohs(received_packet->seq_num);
            // Check if this is out of the range of window, ignore and do not send ack back!
            if (seq_num < curr_ack || seq_num >= curr_ack + WINDOW_SIZE) {
                cout << "[recv data] / IGNORED (out-of-window)" <<endl;
                // IGNORE OUT OF BOUND
                continue;
            }
            // If the packet is received? (duplicate) Do not send ack back!
            window_node * packet_in_window = window[seq_num % WINDOW_SIZE];
            if (packet_in_window->isReceived) {
                cout << "[recv data] / IGNORED (duplicate)" <<endl;
                continue;
            }
            // If the expecting packet arrives, write back to file and move window
            packet_in_window->isReceived = true;
            packet_in_window->seq_num = seq_num;
            packet_in_window->data = (char *) malloc(recv_packet_length);
            memcpy(packet_in_window->data, received_packet->data, recv_packet_length);

            if (seq_num == curr_ack) {  // If matches, write that to file and move window
                cout << "[recv data] / ACCEPTED (in-order)" << endl;
                // write back and move
                update_window(window, &isComplete, &curr_ack, last_packet_seq, outFile);
            } else {    // If fall in window, just store it.
                cout << "[recv data] / ACCEPTED (out-of-order)" << endl;
            }
            // Send back ACK
            ack->seq_num = htons(0);
            ack->ack = htons(seq_num);
            sendto(server_sock, ack, 16, 0,(struct sockaddr*) &client_sin, client_len);
            // Check is finished?
            if (isComplete) {
                cout << "[complete!]" <<endl;
                break;
            }

        } else continue;
    }

    outFile.close();

    // Free memory
    for (int i = 0; i < WINDOW_SIZE; i++) {
        window_node * node = window[i];
        delete node->data;
        node->data = nullptr;
        delete node;
        node = nullptr;
    }
    free(ack);
    close(server_sock);
    return 0;
}

