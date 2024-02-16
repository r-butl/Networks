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

#define SERVER_PORT "5432" // This must match on client and server
#define BUF_SIZE 256 // This can be smaller. What size?

/*
 * Lookup a host IP address and connect to it using service. Arguments match the first two
 * arguments to getaddrinfo(3).
 *
 * Returns a connected socket descriptor or -1 on error. Caller is responsible for closing
 * the returned socket.
 */
int lookup_and_connect( const char *host, const char *service );

int main( int argc, char *argv[] ) {
	char *host;
	char buf[BUF_SIZE];
	int s;
	int len;
	int bytes_sent_recv;
	uint32_t a, b;
	uint32_t answer;

	
	if ( argc == 2 ) {
		host = argv[1];
	}
	else {
		fprintf( stderr, "usage: %s host\n", argv[0] );
		exit( 1 );
	}

	// Lookup IP and connect to server 
	if ( ( s = lookup_and_connect( host, SERVER_PORT ) ) < 0 ) {
		exit( 1 );
	}
	

	while(1) {de 

		// Get two numbers (a and b) from the user
		printf("Enter two number with a space: a b\n");
		scanf("%u %u", &a, &b);

		printf("%d %d\n", a, b);
		a = htonl(a);
		b = htonl(b);
		
		// Copy the numbers into a buffer (buf)
		memcpy(buf, &a, sizeof(a));
		memcpy(buf + sizeof(a), &b, sizeof(b));

		// Send the buffer to the server using the connected socket. Only send the bytes (8 bytes) for a and b!
		if( (bytes_sent_recv = send(s, &buf, 2 * sizeof(a), 0)) == -1){	
			perror("client: send error");
			close( s );
			exit( 1 );
		}else{
			printf("%d bytes sent...\n", bytes_sent_recv);
		}

		// Receive the sum from the server into a buffer
		if( (bytes_sent_recv = recv(s, &buf, 1 * sizeof(a), 0)) == -1){
			perror("client: recv error");
			close( s );
			exit( 1 );
		}else{
			printf("%d bytes recv...\n", bytes_sent_recv);
		}

		// Copy the sum out of the buffer into a variable (answer)
		memcpy(&answer, buf, sizeof(answer));

		answer = ntohl(answer);
		// Print the sum
		printf("The answer is... %u\n", answer);

	}

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
