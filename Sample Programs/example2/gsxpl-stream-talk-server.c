/* This code is an updated version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some code comes from
 * man pages, mostly getaddrinfo(3). */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_PORT "5432"
#define MAX_LINE 256
#define MAX_PENDING 5

/*
 * Create, bind and passive open a socket on a local interface for the provided service.
 * Argument matches the second argument to getaddrinfo(3).
 *
 * Returns a passively opened socket or -1 on error. Caller is responsible for calling
 * accept and closing the socket.
 */
int bind_and_listen( const char *service );

int main( void ) {

	printf("Port Number: %s\n", SERVER_PORT);
	char buf[MAX_LINE];
	int s, new_s;
	int len;

	/* Bind socket to local interface and passive open */
	if ( ( s = bind_and_listen( SERVER_PORT ) ) < 0 ) {
		printf("Bind failed...\n");
		exit( 1 );
	}else{
		printf("Bind accepted, listening...\n");
	}

	/* Wait for connection, then receive and print text */
	while ( 1 ) {
		if ( ( new_s = accept( s, NULL, NULL ) ) < 0 ) {
			perror( "stream-talk-server: accept" );
			close( s );
			exit( 1 );
		}else{
			printf("New connection accepted...\n");
		}
		while ( ( len = recv( new_s, buf, sizeof( buf ), 0 ) ) ) {
			if ( len < 0 ) {
				perror( "streak-talk-server: recv" );
				close( new_s );
				exit( 1 );
			}

			fputs( buf, stdout );
		}
	}

	printf("Closing...\n");
	close( s );

	return 0;
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
	}else{
		printf("Address received...\n");
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
