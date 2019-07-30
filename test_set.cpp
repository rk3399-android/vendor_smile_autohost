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
#include "types.h"






using namespace std;

void setvalue(emulator::VehiclePropValue* value){
		value->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::PARKING_BRAKE_ON));
		value->set_area_id(0/*static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1)*/);
		value->add_int32_values(0);
}

int main(int argc, char const *argv[]) { 
	struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    int buffer_size ; 
    int buffer_size_read ; 
    uint8_t msg[]={};
	emulator::EmulatorMessage message;	
	emulator::EmulatorMessage message_recu;
	emulator::EmulatorMessage message_read;
	emulator::EmulatorMessage message_recu_read;
		
	message.set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);	
	setvalue(message.add_value());	
	
	
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
	printf("size_config%zu\n",size);
	printf("messageLen_config%d\n",messageLen);
    send(sock , &messageLen, sizeof(messageLen) , 0 );
    send(sock , (void*)buff, /*messageLen*/size , 0 );
    
   /* recv(sock, (void*)&buffer_size, 4 ,0);
    buffer_size=ntohl(buffer_size);
    cout << "taille du message recu " << buffer_size << endl;
    
    msg[buffer_size];
    recv(sock, (void*)&msg, buffer_size,0);
    message_recu.ParseFromArray((void*)&msg, buffer_size);
    cout << "  msg_type recu: " << message_recu.msg_type() << endl;
    
    message_read.set_msg_type(emulator::MsgType::GET_PROPERTY_ALL_CMD);
    size_t size_read= message_read.ByteSize();
    void *buff_read = malloc(size_read);
    message_read.SerializeToArray(buff_read, size_read);
    
    int32_t messageLen_read = htonl(size_read);
	printf("size_config%zu\n",size_read);
	printf("messageLen_config%d\n",messageLen_read);
    send(sock , &messageLen_read , sizeof(messageLen_read) , 0 );
    send(sock , (void*)buff_read , messageLen_read , 0 );
    
    recv(sock, (void*)&buffer_size_read, 4 ,0);
    buffer_size_read=ntohl(buffer_size_read);
    cout << "message_size recu " << buffer_size_read << endl;
    
    int msg_read[buffer_size_read];
    recv(sock, (void*)&msg_read, buffer_size_read,0);
    message_recu_read.ParseFromArray((void*)&msg_read, buffer_size_read);
    cout << "  msg_type recu: " << message_recu_read.msg_type() << endl;*/
    
    google::protobuf::ShutdownProtobufLibrary();
    
    return 0; 
}	
