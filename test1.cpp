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

void getallvalue(emulator::EmulatorMessage* message){
	
	message->set_msg_type(emulator::MsgType::GET_PROPERTY_ALL_CMD);
}

void getallconfig(emulator::EmulatorMessage* message){
	
	message->set_msg_type(emulator::MsgType::GET_CONFIG_ALL_CMD);
}


int main(int argc, char const *argv[]) { 
	struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    int buffer_size_value ; 
    int buffer_size_config ; 
    emulator::EmulatorMessage message_value;
    emulator::EmulatorMessage message_config;
	emulator::EmulatorMessage message_recu_value;
	emulator::EmulatorMessage message_recu_config;
	
	

    getallvalue(&message_value);
	getallconfig(&message_config);
    
    size_t size_value= message_value.ByteSize();
    void *buff_value = malloc(size_value);
    message_value.SerializeToArray(buff_value, size_value);

	
	size_t size_config= message_config.ByteSize();
    void *buff_config = malloc(size_config);
    message_config.SerializeToArray(buff_config, size_config);


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
    
	int32_t messageLen_value = htonl(size_value);
	
    send(sock , &messageLen_value , sizeof(messageLen_value) , 0 );
    send(sock , (void*)buff_value , messageLen_value , 0 );
    
    
    recv(sock, (void*)&buffer_size_value, 4 ,0);
    buffer_size_value=ntohl(buffer_size_value);
    cout << "message recu " << buffer_size_value << endl;
    
    uint8_t msg_value[buffer_size_value];
    int size_recv=0;
    do{
     size_recv += recv(sock, (void*)&(msg_value[size_recv]),  buffer_size_value-size_recv ,0);
	}while(size_recv!=buffer_size_value);
	
    bool ret=message_recu_value.ParseFromArray((void*)&msg_value, buffer_size_value);
    if (!ret) {
		cout << "Error at parsing" << endl;
	}
    
    cout << "  msg_type recu: " << message_recu_value.msg_type() << endl;
    cout << "  value size recu: " << message_recu_value.value_size() << endl;
    for(int i=0; i<message_recu_value.value_size(); i++){
		printf( "  value[%d] prop: 0x%x\n",i,message_recu_value.mutable_value(i)->prop());
	}
  
	
	int32_t messageLen_config = htonl(size_config);
	int send_size;
	printf("size_config%d\n",size_config);
	printf("messageLen_config%d\n",messageLen_config);
    send_size=send(sock , &messageLen_config , sizeof(messageLen_config) , 0 );
    send(sock , (void*)buff_config , messageLen_config , 0 );
    cout << "message send" << send_size<< endl;
    
    recv(sock, (void*)&buffer_size_config, 4 ,0);
    buffer_size_config=ntohl(buffer_size_config);
    cout << "message recu " << buffer_size_config << endl;
    
    uint8_t msg_config[buffer_size_config];
    recv(sock, (void*)&msg_config, buffer_size_config,0);
    message_recu_config.ParseFromArray((void*)&msg_config, buffer_size_config);
    cout << "  msg_type recu: " << message_recu_config.msg_type() << endl;
    cout << "  config size recu: " << message_recu_config.config_size() << endl;
    for(int i=0; i<message_recu_config.config_size(); i++){
		printf( "  config[%d] prop: 0x%x\n",i,message_recu_config.mutable_config(i)->prop());
	}
	
	
    
    google::protobuf::ShutdownProtobufLibrary();
    
    return 0; 
} 
