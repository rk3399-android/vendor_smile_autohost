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

int main(int argc, char const *argv[]) { 
	struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    int buffer_size ;  
	emulator::EmulatorMessage message;	
	emulator::EmulatorMessage message_recu;
	emulator::VehiclePropGet* propget;
	
	message.set_msg_type(emulator::GET_PROPERTY_CMD);
	propget=message.add_prop();
	propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty:: HVAC_AUTO_ON));
	propget->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
	
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
    send(sock , (void*)buff, size , 0 );
    
    recv(sock, (void*)&buffer_size, 4 ,0);
    buffer_size=ntohl(buffer_size);
    cout << "taille du message recu " << buffer_size << endl;
    
    uint8_t msg[buffer_size];
    recv(sock, (void*)&msg, buffer_size,0);
    message_recu.ParseFromArray((void*)&msg, buffer_size);
    cout << "  msg_type recu: " << message_recu.msg_type() << endl;
    cout << "  status recu: " << message_recu.status() << endl;
    cout << "prop size recu: " << message_recu.prop_size() << endl;
    cout << "config size recu: " << message_recu.config_size() << endl;
    cout << "value size recu: " << message_recu.value_size() << endl;
    cout << "value prop: " << message_recu.mutable_value(0)->prop()<< endl;
    cout << "value type: " << message_recu.mutable_value(0)->value_type()<< endl;
    cout << "value timestamp: " << message_recu.mutable_value(0)->timestamp()<< endl;
    cout << "value area id: " << message_recu.mutable_value(0)->area_id()<< endl;
    cout << "value int32 size : " << message_recu.mutable_value(0)->int32_values_size() << endl;
    cout << "value recu: " << message_recu.mutable_value(0)->int32_values(0) << endl;
    cout << "value int64 size : " << message_recu.mutable_value(0)->int64_values_size() << endl;
    cout << "value float size : " << message_recu.mutable_value(0)->float_values_size() << endl;
    cout << "value has string : " << message_recu.mutable_value(0)->has_string_value() << endl;
    cout << "value has byte : " << message_recu.mutable_value(0)->has_bytes_value() << endl;
    
    google::protobuf::ShutdownProtobufLibrary();
    
    return 0; 
 }   
