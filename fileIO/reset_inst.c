/* This code is an updated version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some code comes from
 * man pages, mostly getaddrinfo(3). */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#define SERVER_PORT "80"
#define MAX_LINE 1000

/*
 * Lookup a host IP address and connect to it using service. Arguments match the first two
 * arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible for closing
 * the returned socket.
 */
int lookup_and_connect( const char *host, const char *service );

int count_occurances( char * string, char * pattern, int string_len);

int main( int argc, char *argv[] ) {
	char *host = "www.ecst.csuchico.edu";
	char *request = "GET /~kkredo/reset_instructions.pdf HTTP/1.0\r\n\r\n";
	char *tag = "<h1>";
	char buf[MAX_LINE];						
	int s = 0;
	int byte_count = 0;
	int recv_status = 0;
	int chunk_size = 0;	// Assuming chunks are in bytes and that tag <h1> is 4 bytes
	char *output_file = "local_file";
	int output_file_desc;
	// Retrieve package chunk size as a command line argument		|
	if ( argc == 2 ) {						//	|
		chunk_size = atoi(argv[1]);				//	|
		printf("chunk_size:%d\n", chunk_size);			//	|
		// Bounds checking for chunk_size				|
		if ( chunk_size < strlen(tag) || chunk_size > MAX_LINE ){//	<----------------
			perror("Invalid chunk size: 0 < chunk_size < 1001\n");
			exit( 1 );
		}
	
	}
	else {
		fprintf( stderr, "usage: %s chunk_size\n", argv[0] );
		exit( 1 );
	}

	
	// Lookup IP and connect to server 
	if ( ( s = lookup_and_connect( host, SERVER_PORT ) ) < 0 ) {
		perror("Error connecting to host...\n");
		exit( 1 );
	}else{	
		printf("Connected to host %s at port %s...\n", host, SERVER_PORT);
		
	}
	
	// Main loop: get and send lines of text 
	if ( send( s, request, strlen(request), 0 ) == -1 ) {
		perror( "stream-talk-client: send" );
		close( s );
		exit( 1 );
	}else{
		printf("Sent request: '%s'...\n", request);
	}

	// Open up an output file and use it to write data
	printf("Opening output file '%s'\n", output_file); 
    	if( ( output_file_desc = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) ) == -1){
        	perror("Error opening file...");
        	return -1;
    	}

	// Recieve bytes from the sender
	while( (  recv_status = recv(s, buf, chunk_size, 0) ) > 0){
	//	printf("recieving...\n");
		byte_count = byte_count + recv_status;
	//	for(int i=0; i < recv_status; i++) printf("%c", buf[i]);
		        // Write the bytes to the output file
       	 	
		if( (write(output_file_desc, buf, recv_status) ) < 1 ){
            		perror("Error writing data...\n");
            		return -1;
        	}
	}

	printf("Total bytes: %d\n", byte_count);	
	if(recv_status == -1){
		perror( "stream-talk-client: recv" );
		close( s );
		exit( 1 );
	}else{
		printf( "Success...\n");		
	}
		
	close(output_file_desc);	// close the file
	close( s ); 	// close the socket
	printf("Number of bytes: %d\n", byte_count);
	return 0;
}

int lookup_and_connect( const char *host, const char *service ) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Translate host name into peer's IP address */
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_INET;
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
