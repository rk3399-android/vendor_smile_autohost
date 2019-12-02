#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include "socket.h"

#define PORT 33452
#define INTSIZE 4

int connect_socket(char* ip){

	int                sock = 0; 
	struct sockaddr_in serv_addr;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Socket creation error\n");
		return -1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0){
		printf("Invalid address/ Address not supported\n");
		return -1;
	}

	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
		printf("Connection Failed\n"); 
		return -1; 
	}

	return sock;
}

void sendToServer(int sock, emulator::EmulatorMessage* message){

	int32_t     messageLen;
	size_t      size = message->ByteSize();
	void*       buff = malloc(size);

	if(!(message->SerializeToArray(buff, size))){
		printf("Failed to serialize to array\n");
	}

	messageLen = htonl(size);

	if(!(send(sock, &messageLen, sizeof(messageLen), 0))){
		printf("Failed to send message size\n");
	}

	if(!(send(sock, (void*)buff, size, 0))){
		printf("Failed to send message\n");
	}
}

void receiveFromServer(int sock, emulator::EmulatorMessage* message){

	int      buffer_size;
	int      size_recv = 0;
	int      size_pre = 0;
	int      size_all = 0;
	uint8_t* msg;

	if(!(recv(sock, (void*)&buffer_size, INTSIZE, 0))){
		printf("Failed to receive message size\n");
	}

	buffer_size = ntohl(buffer_size);
	msg         = (uint8_t*)malloc(sizeof(uint8_t)*buffer_size);

	memset(msg, 0, sizeof(uint8_t)*buffer_size);

	do{
		size_recv = recv(sock, (void*)&(msg[size_recv]),  buffer_size-size_recv ,0);
		if(!size_recv){
			printf("Failed to receive message\n");
		}
		size_all = size_recv + size_pre;
		size_pre = size_recv;
	}while(size_all != buffer_size);

	if(!(message->ParseFromArray((void*)msg, buffer_size))){
		printf("Failed to parse from array\n");
	}
}
