#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>


#include <iostream>
#include <fstream>
#include <string>
#include "VehicleHalProto.pb.h"
#include "types.h"

#define PORT 33452
#define INTSIZE 4
#define SIZE 20

using namespace std;


int connect_socket(){
	
	struct sockaddr_in 	address; 
    int 				sock = 0; 
    struct sockaddr_in 	serv_addr;
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
		printf("Socket creation error\n"); 
        return -1; 
    } 
    
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port	 = htons(PORT); 

    if(inet_pton(AF_INET, "10.1.75.162", &serv_addr.sin_addr) <= 0){ 
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
	
	int32_t 	messageLen;
	size_t	 	size = message->ByteSize();
    void* 		buff = malloc(size);
    
    if(!(message->SerializeToArray(buff, size))){
		printf("Failed to serialize to array\n");
	}	
	
	messageLen = htonl(size);
	
    if(!(send(sock, &messageLen, sizeof(messageLen), 0))){
		printf("Failed to send message size\n");
	}
	
    if(!(send(sock, (void*)buff, messageLen, 0))){
		printf("Failed to send message\n");
	}		
}

void receiveFromServer(int sock, emulator::EmulatorMessage* message){
	
	int 		buffer_size; 
	int 		size_recv = 0; 
	int 		size_pre = 0;
	int 		size_all = 0;
	uint8_t* 	msg;
	
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
	
    if(!(message->ParseFromArray((void*)msg, buffer_size))) {
		printf("Failed to parse from array\n");
	}	
}

int setONorOFF(char* val){
	
	if(!strcmp((const char*)val,"ON")){
		return 1;
	}
	else if(!strcmp((const char*)val,"OFF")){
		return 0;
	}
	else{
		printf("Invalid value\n");
		return -1;
	}	
}

int32_t requestedrow(){
	
	char row[SIZE];
	
	printf("ROW_1_LEFT or ROW_1_RIGHT? ");
	scanf("%s", row);
	
	if(!strcmp((const char*)row, "ROW_1_LEFT")){
		return static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1_LEFT);
	}
	else if(!strcmp((const char*)row, "ROW_1_RIGHT")){
		return static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1_RIGHT);
	}
	else{
		printf("Invalid row\n");
		return -1;
	}
}

int32_t requestedseatrow(){
	
	char seatrow[SIZE];
	
	printf("ROW_1_LEFT or ROW_1_RIGHT? ");
	scanf("%s", seatrow);
	
	if(!strcmp((const char*)seatrow, "ROW_1_LEFT")){
		return static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaSeat::ROW_1_LEFT);
	}
	else if(!strcmp((const char*)seatrow, "ROW_1_RIGHT")){
		return static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaSeat::ROW_1_RIGHT);
	}
	else{
		printf("Invalid row\n");
		return -1;
	}
}

int32_t requestedwindshiedl(){
	
	char windshiedl[SIZE];
	
	printf("FRONT_WINDSHIELD or REAR_WINDSHIELD? ");
	scanf("%s", windshiedl);
	
	if(!strcmp((const char*)windshiedl, "FRONT_WINDSHIELD")){
		return static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaWindow::FRONT_WINDSHIELD);
	}
	else if(!strcmp((const char*)windshiedl, "REAR_WINDSHIELD")){
		return static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaWindow::REAR_WINDSHIELD);
	}
	else{
		printf("Invalid row\n");
		return -1;
	}
}	

int getProperty(emulator::EmulatorMessage* message, char* prop){
	
	emulator::VehiclePropGet* propget = message->add_prop();
	
	message->set_msg_type(emulator::MsgType::GET_PROPERTY_CMD);
	
	if(!strcmp((const char*)prop, "HVAC_AUTO_ON")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_AUTO_ON));
		propget->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));				
	}
	else if(!strcmp((const char*)prop, "HVAC_AC_ON")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_AC_ON));
		propget->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));				
	}
	else if(!strcmp((const char*)prop, "HVAC_RECIRC_ON")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_RECIRC_ON));
		propget->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));				
	}
	else if(!strcmp((const char*)prop, "HVAC_FAN_SPEED")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_FAN_SPEED));
		propget->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
	}
	else if(!strcmp((const char*)prop, "HVAC_FAN_DIRECTION")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_FAN_DIRECTION));
		propget->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
	}	
	else if(!strcmp((const char*)prop, "HVAC_TEMPERATURE_SET")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_TEMPERATURE_SET));
		propget->set_area_id(requestedrow());
	}
	else if(!strcmp((const char*)prop, "HVAC_SEAT_TEMPERATURE")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_SEAT_TEMPERATURE));
		propget->set_area_id(requestedseatrow());
	}	
	else if(!strcmp((const char*)prop, "HVAC_DEFROSTER")){
		propget->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_DEFROSTER));
		propget->set_area_id(requestedwindshiedl());	
	}	
	else{
		printf("Invalid property\n");
		return -1;
	}
	return 0;				
}
			
	
int setProperty(emulator::EmulatorMessage* message, char* prop, char* val){
	
	emulator::VehiclePropValue* propset = message->add_value();
	
	message->set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);
	
	if(!strcmp((const char*)prop, "HVAC_AUTO_ON")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_AUTO_ON));
		propset->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
		propset->add_int32_values(setONorOFF(val));				
	}
	else if(!strcmp((const char*)prop, "HVAC_AC_ON")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_AC_ON));
		propset->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
		propset->add_int32_values(setONorOFF(val));				
	}
	else if(!strcmp((const char*)prop, "HVAC_TEMPERATURE_SET")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_TEMPERATURE_SET));
		propset->set_area_id(requestedrow());
		
		if(atoi(val) >= 16 && atoi(val) <= 29){
			propset->add_float_values(atoi(val));
		}
		else{
			printf("Need a value bewteen 16 and 29\n");
			return -1;
		}						
	}
	else if(!strcmp((const char*)prop, "HVAC_RECIRC_ON")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_RECIRC_ON));
		propset->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
		propset->add_int32_values(setONorOFF(val));
	}	
	else if(!strcmp((const char*)prop, "HVAC_DEFROSTER")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_DEFROSTER));
		propset->set_area_id(requestedwindshiedl());
		propset->add_int32_values(setONorOFF(val));
	}
	else if(!strcmp((const char*)prop, "HVAC_SEAT_TEMPERATURE")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_SEAT_TEMPERATURE));
		propset->set_area_id(requestedseatrow());
		
		if(!strcmp((const char*)val, "0")){
			propset->add_int32_values(0);
		}
		else if(!strcmp((const char*)val, "1")){
			propset->add_int32_values(1);
		}
		else if(!strcmp((const char*)val, "2")){
			propset->add_int32_values(2);
		}
		else if(!strcmp((const char*)val, "3")){
			propset->add_int32_values(3);
		}
		else{
			printf("Need a value between 0 and 3\n");
			return -1;
		}
	}	
	else if(!strcmp((const char*)prop, "HVAC_FAN_DIRECTION")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_FAN_DIRECTION));
		propset->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
		
		if(!strcmp((const char*)val, "FACE")){
			propset->add_int32_values(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleHvacFanDirection::FACE));
		}
		else if(!strcmp((const char*)val, "FLOOR")){
			propset->add_int32_values(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleHvacFanDirection::FLOOR));
		}
		else if(!strcmp((const char*)val, "FACE_AND_FLOOR")){
			propset->add_int32_values(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleHvacFanDirection::FACE_AND_FLOOR));
		}
		else if(!strcmp((const char*)val, "DEFROST_AND_FLOOR")){
			propset->add_int32_values(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleHvacFanDirection::DEFROST_AND_FLOOR));
		}
		else{
			printf("wait for value: FACE, FLOOR, FACE_AND_FLOOR OR DEFROST_AND_FLOOR\n");
			return -1;
		}			
	}
	else if(!strcmp((const char*)prop, "HVAC_FAN_SPEED")){
		propset->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::HVAC_FAN_SPEED));
		propset->set_area_id(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleAreaZone::ROW_1));
		
		if(!strcmp((const char*)val, "1")){
			propset->add_int32_values(1);
		}
		else if(!strcmp((const char*)val, "2")){
			propset->add_int32_values(2);
		}
		else if(!strcmp((const char*)val, "3")){
			propset->add_int32_values(3);
		}
		else if(!strcmp((const char*)val, "4")){
			propset->add_int32_values(4);
		}
		else if(!strcmp((const char*)val, "5")){
			propset->add_int32_values(5);
		}
		else if(!strcmp((const char*)val, "6")){
			propset->add_int32_values(6);
		}
		else{
			printf("Need a value between 1 and 6\n");
			return -1;
		}
	}
	else{
		printf("Invalid property\n");
		return -1;
	}
	return 0;
}	
int getOrset(emulator::EmulatorMessage* message){
	
	char type[SIZE];
	char pro[SIZE];
	char val[SIZE];
	
	printf("get or set? ");
	scanf("%s", type);
	
	printf("property? ");
	scanf("%s", pro);
	
	if(!strcmp((const char*)&type, "get")){
		if(getProperty(message, (char*)&pro)==-1){
			return 0;
		}		
	}		
	else if(!strcmp((const char*)&type, "set")){
		printf("value? ");
		scanf("%s", val);
		if(setProperty(message, (char*)&pro, (char*)&val)==-1){
			return 0;
		}	
	}	
	else{
		printf("Invalid type\n");
		return 0;
	}	
	return 1;
}

void afficher_resultat(emulator::EmulatorMessage* message){
	
	int msg_type = message->msg_type();
	
	switch(msg_type){
		case emulator::MsgType::GET_PROPERTY_RESP:
			for(int i = 0; i < message->value_size(); i++){
				for(int j = 0; j < message->mutable_value(i)->int32_values_size(); j++){
					printf("Int32 value received: %d\n", message->mutable_value(i)->int32_values(j));
				}
				for(int j = 0; j < message->mutable_value(i)->int64_values_size(); j++){
					printf("Int64 value received: %lld\n", message->mutable_value(i)->int64_values(j));
				}
				for(int j = 0; j < message->mutable_value(i)->float_values_size(); j++){
					printf("Float value received: %f\n", message->mutable_value(i)->float_values(j));
				}	
				if(message->mutable_value(i)->has_string_value()){
					printf("String value received: %s\n", message->mutable_value(i)->string_value().c_str());
				}
				if(message->mutable_value(i)->has_bytes_value()){
					printf("Bytes value received: %s\n", message->mutable_value(i)->bytes_value().c_str());
				}				
			}				
		break;
		case emulator::MsgType::SET_PROPERTY_RESP :
			printf("You have asked for setting value\n");
		break;
		default:
			printf("Invalid message type\n");			
	}		
}

int main() { 
	
	int							sock; 
    emulator::EmulatorMessage 	message;	
	emulator::EmulatorMessage	message_recv;
   
	if(getOrset(&message)==0){
		return 0;
	}	
	
	sock = connect_socket();
	
    sendToServer(sock, &message);
    receiveFromServer(sock, &message_recv);
    afficher_resultat(&message_recv);
	
	google::protobuf::ShutdownProtobufLibrary();
    
    return 0; 
}
