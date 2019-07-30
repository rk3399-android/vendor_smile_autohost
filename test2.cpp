#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#define PORT 33452

#include <iostream>
#include <fstream>
#include <string>
#include "VehicleHalProto.pb.h"

using namespace std;

void getall(emulator::EmulatorMessage* message,emulator::MsgType type){
	message->set_msg_type(type);
}


int main(int argc, char const *argv[]) { 
	struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    int buffer_size;
    emulator::EmulatorMessage message;
	emulator::EmulatorMessage message_recu;

	getall(&message,emulator::MsgType::GET_CONFIG_ALL_CMD);
	
	size_t size= message.ByteSize();
    void *buff = malloc(size);
    message.SerializeToArray(buff, size);
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "10.1.75.162", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    }
    
	int32_t messageLen = htonl(size);
	printf("size_value%d\n",size);
	printf("messageLen_value%d\n",messageLen);
    send(sock , &messageLen , sizeof(messageLen) , 0 );
    send(sock , (void*)buff , messageLen , 0 );
    
    
    recv(sock, (void*)&buffer_size, 4 ,0);
    buffer_size=ntohl(buffer_size);
    cout << "message recu " << buffer_size << endl;
    
    int msg[buffer_size];
    recv(sock, (void*)&msg, buffer_size,0);
    message_recu.ParseFromArray((void*)&msg, buffer_size);
    cout << "  msg_type recu: " << message_recu.msg_type() << endl;
    cout << "  value size recu: " << message_recu.config_size() << endl;
    for(int i=0; i<message_recu.config_size(); i++){
		printf( "  value[%d] prop: 0x%x\n",i,message_recu.mutable_config(i)->prop());
	}
	
	getall(&message,emulator::MsgType::GET_PROPERTY_ALL_CMD);
	size= message.ByteSize();
    buff = malloc(size);
    message.SerializeToArray(buff, size);
    messageLen = htonl(size);
	printf("size_value%d\n",size);
	printf("messageLen_value%d\n",messageLen);
    if(!(send(sock , &messageLen , sizeof(messageLen) , 0 ))){
		printf("pb");
	}	
    if(!(send(sock , (void*)buff , messageLen , 0 ))){
		printf("pb2");
    }
    
    recv(sock, (void*)&buffer_size, 4 ,0);
    buffer_size=ntohl(buffer_size);
    cout << "message recu " << buffer_size << endl;
    
    msg[buffer_size];
    recv(sock, (void*)&msg, buffer_size,0);
    message_recu.ParseFromArray((void*)&msg, buffer_size);
    cout << "  msg_type recu: " << message_recu.msg_type() << endl;
    cout << "  value size recu: " << message_recu.value_size() << endl;
    for(int i=0; i<message_recu.value_size(); i++){
		printf( "  value[%d] prop: 0x%x\n",i,message_recu.mutable_config(i)->prop());
	}
}
	
