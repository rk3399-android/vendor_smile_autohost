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

void getallconfig(emulator::EmulatorMessage* message){
	
	message->set_msg_type(emulator::MsgType::GET_PROPERTY_ALL_CMD);
}

int main(int argc, char const *argv[]) { 
	struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    int buffer_size_config ; 
    emulator::EmulatorMessage message_config;
    emulator::EmulatorMessage message_recu_config;
   
    getallconfig(&message_config);
    
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
    
    int32_t messageLen_config = htonl(size_config);
	
	printf("size_config%zu\n",size_config);
	printf("messageLen_config%d\n",messageLen_config);
    send(sock , &messageLen_config , sizeof(messageLen_config) , 0 );
    send(sock , (void*)buff_config , messageLen_config , 0 );
    
    recv(sock, (void*)&buffer_size_config, 4 ,0);
    
    buffer_size_config=ntohl(buffer_size_config);
  //  cout << "message recu " << buffer_size_config << endl;
    
    uint8_t msg_config[buffer_size_config];
    int size_recv=0;
	int size_=0;
	int size_all=0;
	
    do{
     //size_recv += recv(sock, (void*)&(msg_config[size_recv]),  buffer_size_config-size_recv ,0);
     size_recv = recv(sock, (void*)&(msg_config[size_recv]),  buffer_size_config-size_recv ,0);
		//cout << "size_recv = " << size_recv<< endl;
		if(!size_recv){
			printf("failed to receive message");
		}
		size_all = size_recv+size_;
		size_=size_recv;	
     //cout << "size_recv = " << size_recv<< endl;
	}while(size_all!=buffer_size_config);
   
    //cout << "size_recv = " << size_recv<<endl;
    bool ret = message_recu_config.ParseFromArray((void*)&msg_config,  buffer_size_config);
    if (!ret) {
		cout << "Error at parsing" << endl;
	}
   // cout << "msg_type recu: " << message_recu_config.msg_type() << endl;
   // cout << "value size recu: " << message_recu_config.value_size() << endl;
    
    for(int i=0; i<message_recu_config.value_size(); i++){
		printf( "value[%d] prop: 0x%x\n",i,message_recu_config.mutable_value(i)->prop());
		int type=message_recu_config.mutable_value(i)->value_type();
		//printf( "\tvalue[%d] hasvaluetype  0x%d\n",i,type);
		switch (type){
			case 1048576: //dans le cas d'un string
				printf( "\tvalue  value string %s\n",&message_recu_config.mutable_value(i)->string_value());
				break;
			case 2097152: // dans le cas d'un boolean
				//printf( "value[%d]  value bool size %d\n",i,message_recu_config.mutable_value(i)->int32_values_size());
				for(int j=0;j<message_recu_config.mutable_value(i)->int32_values_size();j++){
					printf( "\tvalue boolean %d\n",message_recu_config.mutable_value(i)->int32_values(j));
				}	
				break;	
			case 4194304: // dans le cas d'un int
				//printf( "value[%d]  value int size %d\n",i,message_recu_config.mutable_value(i)->int32_values_size());
				printf( "\tvalue int %d\n",message_recu_config.mutable_value(i)->int32_values(0));
				
				break;
			case 6291456: // dans le cas d'un float
				//printf( "value[%d]  value float size %d\n",i,message_recu_config.mutable_value(i)->float_values_size());
				printf( "\tvalue float %f\n",message_recu_config.mutable_value(i)->float_values(0));
				break;
			default:
				printf("type complex\n");
				//printf( "value[%d]  value float size %d\n",i,message_recu_config.mutable_value(i)->float_values_size());
					
		}			
				
	}
    
    google::protobuf::ShutdownProtobufLibrary();
    
    return 0; 
} 
