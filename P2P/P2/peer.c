// Lucas Butler and Susie Nguyen


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>  // for opendir(), readdir(), closedir()
#include <errno.h>   // for errno
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_CMD_LEN 100
#define MAX_FILES 100
#define MAX_FILENAME_LEN 101 // 100 + NULL
#define BUFFER_SIZE 1200
#define SHARED_FOLDER "SharedFiles/"
#define SERVER_PORT "6000"
#define MAX_IP_SIZE 16
#define MAX_PORT_SIZE 6

int lookup_and_connect( const char *host, const char *service );
void joinP2P(int sockfd, uint32_t peerID);
void publishFiles(int sockfd);
bool searchFile(int sockfd, const char* fileName, char* peer_ip_str, char* peer_port) ;
void fetchFile(int sockfd, const char* fileName);
int send_all(int sockfd, char *buf, int len, int flags);
int recv_all(int s, char *buf, int len);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Registry IP/Hostname> <Registry Port> <Peer ID>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* registry_ip = argv[1];
    const char* registry_port = argv[2];
    uint32_t peer_id = strtoul(argv[3], NULL, 10);

    char fetch_ip_str[INET_ADDRSTRLEN];
    char fetch_port[MAX_PORT_SIZE];

    int sockfd = lookup_and_connect(registry_ip, registry_port);
    if (sockfd < 0) {
        fprintf(stderr, "Unable to connect to the registry.\n");
        return EXIT_FAILURE;
    }

    char input[MAX_INPUT_LENGTH];
    while (true) {
        printf("Enter a command: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            fprintf(stderr, "Error reading command. Please try again.\n");
            continue;
        }

        // Remove trailing newline character
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "EXIT") == 0) {
            close(sockfd);
            break;
        } else if (strcmp(input, "JOIN") == 0) {
            joinP2P(sockfd, peer_id);

        } else if (strcmp(input, "PUBLISH") == 0) {
            publishFiles(sockfd);

        } else if (strcmp(input, "SEARCH") == 0) {
            char fileName[MAX_FILENAME_LEN];
            printf("Enter a file name: ");
            if (fgets(fileName, sizeof(fileName), stdin) != NULL) {
                fileName[strcspn(fileName, "\n")] = 0; // Remove newline character
                searchFile(sockfd, fileName, fetch_ip_str, fetch_port);
            }
        } else if (strcmp(input, "FETCH") == 0) {
            char fileName[MAX_FILENAME_LEN];
            printf("Enter a file name: ");
            if (fgets(fileName, sizeof(fileName), stdin) != NULL) {
                fileName[strcspn(fileName, "\n")] = 0;

                // If the search is successful, create a new socket and fetch the file
                if(searchFile(sockfd, fileName, fetch_ip_str, fetch_port)){

                    int peer_sock = lookup_and_connect(fetch_ip_str, fetch_port);
                    if (peer_sock < 0) {
                        fprintf(stderr, "Unable to connect to the peer with requested file.\n");
                        return EXIT_FAILURE;
                    }

                    fetchFile(peer_sock, fileName);
                }
            }
        } else {
            printf("Unknown command: %s\n", input);
        }
    }

    return EXIT_SUCCESS;
}

void joinP2P(int sockfd, uint32_t peerID) {
    // Message format: [Action=0][Peer ID in network byte order]
    char buffer[5]; // 1 byte for action, 4 bytes for Peer ID
    buffer[0] = 0; // Action for JOIN is 0
    // Copy the Peer ID into the buffer starting at buffer[1]
    uint32_t network_peerID = htonl(peerID); // Convert the Peer ID to network byte order
    memcpy(buffer + 1, &network_peerID, sizeof(network_peerID));

    // Send the JOIN request to the registry
    if (send_all(sockfd, buffer, sizeof(buffer), 0) == -1) {
        perror("send_all failed during JOIN");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    // As per the assignment specification, no response is expected for JOIN.
    printf("JOIN request sent for Peer ID %u\n", peerID);
}

void publishFiles(int sockfd) {
    DIR *dir;
    struct dirent *ent;
    char buffer[BUFFER_SIZE];
    size_t offset = 5; // Start after the Action and Count fields
    int count = 0; // Number of files

    buffer[0] = 1; // Action for PUBLISH

    // Open the "SharedFiles" directory
    if ((dir = opendir(SHARED_FOLDER)) != NULL) {
        // Read each entry in the directory
        while ((ent = readdir(dir)) != NULL) {
            // Skip directories and the file system navigation entries "." and ".."
            if (ent->d_type == DT_REG) {
                size_t name_len = strlen(ent->d_name);

                // Check if we have enough space in the buffer for this filename and its NULL terminator
                if (offset + name_len + 1 > BUFFER_SIZE) {
                    fprintf(stderr, "Buffer size exceeded, too many files or filenames too long.\n");
                    break;
                }

                // Copy the filename into the buffer and add a NULL terminator
                memcpy(buffer + offset, ent->d_name, name_len);
                offset += name_len;
                buffer[offset++] = '\0'; // NULL terminator for the filename

                count++;
            }
        }
        closedir(dir);

        // Set the file count in the buffer (second byte onwards)
        uint32_t network_count = htonl(count);
        memcpy(buffer + 1, &network_count, sizeof(network_count));

        // Send the PUBLISH request to the registry
        if (send_all(sockfd, buffer, offset, 0) == -1) {
            perror("send_all failed during PUBLISH");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("PUBLISH request sent with %d files\n", count);
    } else {
        // Could not open directory
        perror("Unable to open the shared files directory");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

bool searchFile(int sockfd, const char* fileName, char* peer_ip_str, char* peer_port) {
    unsigned char requestBuffer[BUFFER_SIZE] = {0};  // Initialize buffer to all zeros
    unsigned char responseBuffer[BUFFER_SIZE] = {0}; // Initialize buffer to all zeros

    char str_ip[INET_ADDRSTRLEN];
    uint32_t peerID;
    uint32_t peerIP;
    uint16_t peerPort;
    
    requestBuffer[0] = 0x02; // Action for SEARCH is 2

    int fileNameLength = strlen(fileName);
    if (fileNameLength > BUFFER_SIZE - 2) {
        printf("File name is too long.\n");
        return false;
    }

    memcpy(requestBuffer + 1, fileName, fileNameLength); // Copy filename to buffer
    // No need to null-terminate the filename as we initialized the buffer to zeros

    // Send the SEARCH request
    ssize_t bytes_sent = send(sockfd, requestBuffer, fileNameLength + 2, 0); // +1 for action, +1 for null terminator
    if (bytes_sent == -1) {
        perror("Error sending SEARCH request");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Wait for SEARCH response
    ssize_t response_len = recv(sockfd, responseBuffer, sizeof(responseBuffer), 0);
    if (response_len == -1) {
        perror("Error receiving SEARCH response");
        close(sockfd);
        exit(EXIT_FAILURE);
    } else if (response_len == 0) {
        fprintf(stderr, "Connection closed by peer during SEARCH\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memcpy(&peerID, responseBuffer, sizeof(peerID));
    peerID = htonl(peerID);

    memcpy(&peerIP, responseBuffer + sizeof(peerID), sizeof(peerIP));
    inet_ntop(AF_INET, &(peerIP), str_ip, INET_ADDRSTRLEN);

    memcpy(&peerPort, responseBuffer + sizeof(peerID) + sizeof(peerIP), sizeof(peerPort));
    peerPort = ntohs(peerPort);

    if ((strcmp(str_ip, "0.0.0.0") == 0)){
        printf("\nFile not indexed by registry\n");
        printf("\n");
        return false;

    }else{
        printf("\nFile found at");
        printf("\n  Peer %u\n", peerID);
        printf("  %s", str_ip);
        printf(":%d\n", peerPort);
        printf("\n");
        memcpy(peer_ip_str, str_ip, INET_ADDRSTRLEN);   // return the ip
        sprintf(peer_port, "%d", peerPort);   // return the port number
        return true;
    }
}

void fetchFile(int sockfd, const char* fileName){

    char requestBuffer[BUFFER_SIZE] = {0};  // Initialize buffer to all zeros
    char responseBuffer[BUFFER_SIZE] = {0}; // Initialize buffer to all zeros

    requestBuffer[0] = 0x03; // Action for FETCH is 3
    int fileNameLength = strlen(fileName);
    if (fileNameLength > BUFFER_SIZE - 2) {
        printf("File name is too long.\n");
        return;
    }
    memcpy(requestBuffer + 1, fileName, fileNameLength); // Copy filename to buffer

        // Open up the file for output
    FILE *file = fopen(fileName, "w");
    if ( file == NULL ){
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Send the FETCH request
    ssize_t bytes_sent = send(sockfd, requestBuffer, fileNameLength + 2, 0); // +1 for action, +1 for null terminator
    if (bytes_sent == -1) {
        perror("Error sending SEARCH request");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Recieve data from the response and write to the file    
    ssize_t response_len = 1;
    while( response_len > 0){
        response_len = recv_all(sockfd, responseBuffer, sizeof(responseBuffer));

        if (response_len == -1) {
            perror("Error receiving FETCH response");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else if (response_len == 0) {
            fprintf(stderr, "Connection closed by peer during FETCH\n");
            break;
        } 
        
        if(fwrite(responseBuffer, 1, response_len, file) < response_len){
            perror("Error writing to file");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    printf("File %s downloaded successfully.\n", fileName);
    fclose(file);
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

int lookup_and_connect( const char *host, const char *service ) {
        struct addrinfo hints;
        struct addrinfo *rp, *result;
        int s;

        /* Translate host name into peer's IP address */
        memset( &hints, 0, sizeof( hints ) );
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;

        if ( ( s = getaddrinfo( host, service, &hints, &result ) ) != 0 ) {
                fprintf( stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror( s ) );
                return -1;
        }

        /* Iterate through the address list and try to connect */
        for ( rp = result; rp != NULL; rp = rp->ai_next ) {
                if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
                        continue;
                }

                if ( connect( s, rp->ai_addr, rp->ai_addrlen ) != -1 ) {
                        break;
                }

                close( s );
        }

        if ( rp == NULL ) {
                perror( "stream-talk-client: connect" );
                return -1;
        }
        
        freeaddrinfo( result );

        return s;
}