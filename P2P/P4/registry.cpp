#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <vector>
#include <arpa/inet.h>
using namespace std;

#define MAX_LINE 256
#define MAX_PENDING 5

struct peer_entry {
    uint32_t id;  // ID of peer
    int socket_descriptor;  // Socket descriptor for connection to peer
    char files[10][100];  // Files published by peer (up to 10 filenames of 100 or less characters)
    int num_files = 0; //number of files published by this peer
    struct sockaddr_in address;  // Contains IP address and port number
};

/*
Create, bind and passive open a socket on a local interface for the provided service.
Argument matches the second argument to getaddrinfo(3).

Returns a passively opened socket or -1 on error. Caller is responsible for calling
accept and closing the socket.
 */
int bind_and_listen( const char *service );

//request functions
void join(int s, unsigned char buf[], vector<peer_entry>& peer_list);
void publish(int s, unsigned char buf[], vector<peer_entry>& peer_list);
void search(int s, unsigned char buf[], vector<peer_entry>& peer_list);

/*
Return the maximum socket descriptor set in the argument.
This is a helper function that might be useful to you.
 */
int find_max_fd(const fd_set *fs);


int main(int argc, char* argv[]){
    //User must input a port number
    if(argc < 2) {
        cout << "Usage: ./registry <port number>\n";
        return 0;
    } 
    const char* server_port = argv[1];

    vector<peer_entry> peer_list; //store peer_entry structs in a vector

	// all_sockets stores all active sockets. Any socket connected to the server should
	// be included in the set. A socket that disconnects should be removed from the set.
	// The server's main socket should always remain in the set.
	fd_set all_sockets;
	FD_ZERO(&all_sockets);

	// call_set is a temporary used for each select call. Sockets will get removed from
	// the set by select to indicate each socket's availability.
	fd_set call_set;
	FD_ZERO(&call_set);

	// listen_socket is the fd on which the program can accept() new connections
	int listen_socket = bind_and_listen(server_port);
    if(listen_socket == -1) {
        cout << "binding of listen_socket failed\n";
        return -1;
    }
	FD_SET(listen_socket, &all_sockets);

	// max_socket should always contain the socket fd with the largest value
	int max_socket = listen_socket;

	while(1) {
		call_set = all_sockets;
		int num_s = select(max_socket+1, &call_set, NULL, NULL, NULL);
		if( num_s < 0 ){
			perror("ERROR in select() call\n");
			return -1;
		}
		// Check each potential socket.
		// Skip standard IN/OUT/ERROR -> start at 3.
		for( int s = 3; s <= max_socket; ++s ){
			// Skip sockets that aren't ready
			if( !FD_ISSET(s, &call_set) )
				continue;

			// A new connection is ready
			if( s == listen_socket ){
                //accept connection
                int new_s = accept(listen_socket, NULL, NULL);

                //add new_s to all_sockets
                FD_SET(new_s, &all_sockets);

                //update max_socket
                max_socket = find_max_fd(&all_sockets);
			}

			// A connected socket is ready
            //ready = select() detected incoming information
			else{
                //If peers will publish only up to 10 filenames, of length 100 each
                // then largest possible message would be 1+4+(10*100) bytes long
                unsigned char buf[1005];

                if ((recv(s, buf, sizeof(buf), 0)) == 0) { //connection was closed

                    printf("Closing socket.\n");
                    
                    // find peer_list entry tied to this socket (if it exists)
                    for(long unsigned int i = 0; i < peer_list.size(); i++) {
                        if(peer_list.at(i).socket_descriptor == s) {
                            peer_list.erase(peer_list.begin()+i);
                            break;
                        }
                    }

                    FD_CLR(s, &all_sockets);
                    close(s);
                    
                    //update max_socket
                    max_socket = find_max_fd(&all_sockets);
                }
                else { //a message was sent
                    //determine type of request from first byte of message
                    if(buf[0] == 0) { //JOIN request
                        join(s, buf, peer_list);  
                    }
                    else if(buf[0] == 1) { //PUBLISH request
                        publish(s, buf, peer_list);  
                    }
                    else if(buf[0] == 2) { //SEARCH request
                        search(s, buf, peer_list);  
                    }
                    else {
                        cout << "ERROR: recieved a message that does not match supported request types\n";
                    }
                }
			}
		}
	}
}

int find_max_fd(const fd_set *fs) {
	int ret = 0;
	for(int i = FD_SETSIZE-1; i>=0 && ret==0; --i){
		if( FD_ISSET(i, fs) ){
			ret = i;
		}
	}
	return ret;
}

int bind_and_listen( const char *service ) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Build address data structure */
	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	/* Get local address info */
	if ( ( s = getaddrinfo( NULL, service, &hints, &result ) ) != 0 ) {
		fprintf( stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror( s ) );
		return -1;
	}

	/* Iterate through the address list and try to perform passive open */
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( !bind( s, rp->ai_addr, rp->ai_addrlen ) ) {
			break;
		}

		close( s );
	}
	if ( rp == NULL ) {
		perror( "stream-talk-server: bind" );
		return -1;
	}
	if ( listen( s, MAX_PENDING ) == -1 ) {
		perror( "stream-talk-server: listen" );
		close( s );
		return -1;
	}
	freeaddrinfo( result );

	return s;
}


void join(int s, unsigned char buf[], vector<peer_entry>& peer_list) {
    //extract ID from message
    uint32_t* ID = (uint32_t*)(buf+1);
    peer_entry peer = { //declare new peer_entry
        .id = ntohl(*ID),
        .socket_descriptor = s
    };
    socklen_t len = sizeof(peer.address);
    if (getpeername(s, (struct sockaddr*)&peer.address, &len) != 0) {
        cout << "Error in getpeername on socket " << s << endl;
    }
    else {
        //push new peer into vector
        peer_list.push_back(peer);

        //registry output
        cout << "TEST] JOIN " << peer.id << endl;
    }
}

void publish(int s, unsigned char buf[], vector<peer_entry>& peer_list) {
    uint32_t count = *((uint32_t*)(buf+1));
    count = ntohl(count); //convert to host byte order

    //find peer_list entry tied to this socket
    peer_entry* peer;
    for(long unsigned int i = 0; i < peer_list.size(); i++) {
        if(peer_list.at(i).socket_descriptor == s) {
            peer = &peer_list.at(i);
            break;
        }
    }
    peer->num_files = count;

    //fill peer_entry with published files
    int c = 0;
    for(uint32_t i = 0; i < count; i++) {
        int j = 0;
            do {
                peer->files[i][j] = buf[c+5];
                j++;
                c++;
            } while(buf[c+4] != '\0'); //stop do-while loop if char was NULL
    }

    //print registry output
    cout << "TEST] PUBLISH " << count << " ";
    for(uint32_t i = 0; i < count; i++) {
        cout << peer->files[i];
        if(i != count-1)
            cout << " ";
    }
    cout << endl;
}

void search(int s, unsigned char buf[], vector<peer_entry>& peer_list) {
    //read filename until null terminator
    char filename[100];
    int c = 0;
    do {
        filename[c] = buf[c+1];
        c++;
    } while(buf[c] != '\0');

    //Initialize response message to be all 0s
    unsigned char send_msg[10] = {0};

    bool found = false;
    peer_entry found_peer;
    //look for filename in peer list
    for (long unsigned int i = 0; i < peer_list.size(); i++) {
        found_peer = peer_list[i];
        for (int j = 0; j < found_peer.num_files; j++) {
            if(*(found_peer.files[j]) == *filename) { //if filename found in found_peer
                //found correct peer
                found = true;
                break;
            }
        }
        if(found == true)
            break;
    }

    //put needed data from found_peer into send_msg, in network byte order
    if(found == true) {
        uint32_t ID = htonl(found_peer.id);
        memcpy(send_msg, &ID, 4);
        //address & port are already in network order
        memcpy(send_msg+4, &found_peer.address.sin_addr.s_addr, 4);
        memcpy(send_msg+8, &found_peer.address.sin_port, 2);
    }

    //send message
    if ((send(s, send_msg, sizeof(send_msg), 0)) < 0) {
        cout << "Error sending search response\n";
    }

    //registry output
    cout << "TEST] SEARCH " << filename << " ";
        if(found == true) {
            //convert IP address to human readable
            char ip_address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &found_peer.address.sin_addr, ip_address, INET_ADDRSTRLEN);
            //convert port to host order
            uint32_t port = ntohs(found_peer.address.sin_port);

            cout << found_peer.id << " " << ip_address << ":" << port << endl;
        }
        else {
            cout << "0 0.0.0.0:0" << endl;
        }
}
