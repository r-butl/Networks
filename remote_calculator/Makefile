all: client server http_request

http_request: http_start.c
	gcc -o http_request http_start.c -Wall

client: calc_client.c
	gcc -o client calc_client.c -Wall

server: calc_server.c
	gcc -o server calc_server.c -Wall
