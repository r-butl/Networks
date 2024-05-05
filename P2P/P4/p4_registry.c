#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_FILES 10
#define MAX_FILE_LENGTH 100
#define MAX_PEERS 5

int send_all(int sockfd, char *buf, int len, int flags);
int recv_all(int s, char *buf, int len);
int create_server_socket(int port);
void handle_new_connection(int server_sock);
void process_peer_messages(int peer_sock);

typedef struct{
    uint32_t id;                            // ID of peer
    int socket_descriptor;                  // Socket descriptor for connection of peer
    char files[MAX_FILES][MAX_FILE_LENGTH]; // Files published by peer
    struct sockaddr_in address;             // Contains IP address and port number
} peer_entry;

// Global list of peers
peer_entry peers[MAX_PEERS];
int peer_count = 0;

int main(int argc, char *argv[]) {
    int server_sock, portno;
    fd_set active_fd_set, read_fd_set;
    int i;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    portno = atoi(argv[1]);
    server_sock = create_server_socket(portno);

    // Initialize the set of active sockets
    FD_ZERO(&active_fd_set);
    FD_SET(server_sock, &active_fd_set);

    while (1) {
        read_fd_set = active_fd_set;

        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("ERROR on select");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == server_sock) {
                    handle_new_connection(server_sock);
                } else {
                    process_peer_messages(i);
                }
            }
        }
    }

    return 0;
}


int send_all(int s, char *buf, int len, int flags){
    int total = 0;
    int bytesleft = len;
    int n;

    while(total < len){
            n = send(s, buf+total, bytesleft, 0);
            if (n == -1) {break;}
            total += n;
            bytesleft -= n; 
    }
    len = total; 
    return n == -1? -1:total;
}

int recv_all(int s, char *buf, int len){
    int total = 0;
    int n = 0;

    while(total < len){
        n = recv(s, buf + total, len - total, 0);
        if (n <= 0) break; // Includes both error (-1) and orderly shutdown (0)
        total += n;
    }

    return (n == -1) ? -1 : total;
}

int create_server_socket(int port) {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);
    return sockfd;
}

void handle_new_connection(int server_sock) {
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = accept(server_sock, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        perror("ERROR on accept");
        return;
    }

    // Add new peer to the list if possible
    if (peer_count < MAX_PEERS) {
        peers[peer_count].socket_descriptor = newsockfd;
        peers[peer_count].address = cli_addr;
        peer_count++;
        printf("New Connection\n");
    } else {
        close(newsockfd); // No space for new peer
    }
}

void process_peer_messages(int peer_sock) {
    // Implement protocol-specific message handling here
}