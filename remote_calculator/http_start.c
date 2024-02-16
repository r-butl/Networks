/* This code is an updated version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some code comes from
 * man pages, mostly getaddrinfo(3). */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>


#define bufsize 256
/*
 * Lookup a host IP address and connect to it using service. Arguments match the first two
 * arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible for closing
 * the returned socket.
 */
int lookup_and_connect( const char *host, const char *service );

int main( ) {
	int s;
	const char *host = "www.ecst.csuchico.edu";
	const char *port = "80";
	char buf[bufsize];

	/* Lookup IP and connect to server */
	if ( ( s = lookup_and_connect( host, port ) ) < 0 ) {
		exit( 1 );
	}

	/* Modify the program so it
	 *
	 * 1) connects to www.ecst.csuchico.edu on port 80 (mostly done above)
	 * 2) sends "GET /~kkredo/file.html HTTP/1.0\r\n\r\n" to the server
	 * 3) receives all the data sent by the server (HINT: "orderly shutdown" in recv(2))
	 * 4) prints the total number of bytes received
	 *
	 * */

	const char *request = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";
	if ( send(s, request, strlen(request), 0) < 0){
		perror("Send failed");
		close(s);
		exit(1);
	}else{
		printf("Request sent...\n");
	}

	int status = 0;
	int count = 0;

	while( (status = recv(s, &buf, bufsize, 0) ) != 0  ){
		printf("Bytes: %d\n", status);
		//write(STDERR_FILENO, buf, status);
		count += status;

		if(status < 0 ){
			perror("receive failed");
			close(s);
			exit(1);
		}
	}

	printf("Total bytes: %d\n", count);

	close( s );

	return 0;
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
