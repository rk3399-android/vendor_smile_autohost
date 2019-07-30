#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <iostream>
#include <fstream>
#include "VehicleHalProto.pb.h"

#define PORT 33452
#define INTSIZE 4
#define SIZE 20

using namespace std;

struct propertyconfig{
	xmlChar* 	propertyname;
	xmlChar* 	propertyvalue;
	xmlChar**  	zone;
	xmlChar*  	zonetype;
	int 		countzone;
	xmlChar*** 	value;
	xmlChar*  	valuetype;
	int 		countvalue;
};

struct zonevalue{
	xmlChar*** 	row;
	xmlChar*** 	seat;
	xmlChar*** 	window;
	int 		countrow;
	int 		countseat;
	int 		countwindow;
};

int connect_socket(){

	struct sockaddr_in address;
	int                sock = 0;
	struct sockaddr_in serv_addr;
	char*			   addressIP = getenv("ANDROID_EMU_IPV4");	
	char 			   adresse_entree[SIZE];

	
	if(	addressIP == NULL){
		printf("donner l'adresse IPv4: ");
		scanf("%s", adresse_entree);
		addressIP = adresse_entree;
	}
			
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Socket creation error\n");
        return -1;
    }

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port	 = htons(PORT);
	
	if(inet_pton(AF_INET, addressIP, &serv_addr.sin_addr) <= 0){
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

	if(!(send(sock, (void*)buff, size, 0))){
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

xmlDocPtr parseDoc(char *docname){

	xmlDocPtr doc;

	doc = xmlParseFile(docname);

	if(doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		xmlFreeDoc(doc);
		return 0;
	}

	return doc;
}

xmlNodePtr parseemulator(xmlDocPtr doc){

	xmlNodePtr	emulator;
	xmlChar* 	key;

	emulator = xmlDocGetRootElement(doc);

	if(emulator == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		xmlFreeNode(emulator);
		return 0;
	}

	if(xmlStrcmp(emulator->name, (const xmlChar*) "emulator")) {
		fprintf(stderr,"document of the wrong type, root node != ");
		xmlFreeDoc(doc);
		xmlFreeNode(emulator);
		return 0;
	}

	return emulator;
}

xmlNodePtr parseProperties(xmlNodePtr properties){

	properties = properties->xmlChildrenNode;

	while(properties != NULL){
		if(!xmlStrcmp(properties->name, (const xmlChar*)"properties")){
			return properties;
		}
		properties = properties->next;
	}

	xmlFreeNode(properties);

	return 0;
}
/*getPropertyRequested permet d'envoyer la définition de la ppté d'entrées*/
propertyconfig* getPropertyRequested(xmlDocPtr doc, xmlNodePtr properties, char* prop){

	propertyconfig* 	propertyStruct = (propertyconfig*)malloc(sizeof(propertyconfig));
	xmlNodePtr 			property;
	xmlNodePtr 			zonesOrvalues;
	xmlNodePtr 			child;
	xmlChar* 			key;
	xmlChar* 			type;
	xmlChar** 			keyzone;
	xmlChar*** 			keyvalue;
	long				countZone;
	long				countValue;
	int 				i = 0;
	int 				j = 0;

	property = properties->xmlChildrenNode;

	while(property != NULL) {
		if(!xmlStrcmp(property->name, (const xmlChar*)"property")){
			key = xmlGetProp(property, (const xmlChar*)"name");
			if(!xmlStrcmp(key, (const xmlChar *)prop)){
				propertyStruct->propertyname = xmlGetProp(property, (const xmlChar*)"name");
				propertyStruct->propertyvalue = xmlGetProp(property, (const xmlChar*)"value");
				zonesOrvalues = property->xmlChildrenNode;
				while(zonesOrvalues != NULL) {
					if(!xmlStrcmp(zonesOrvalues->name, (const xmlChar*)"zones")){
						countZone = xmlChildElementCount(zonesOrvalues);
						propertyStruct->countzone = countZone;
						keyzone = (xmlChar**)malloc(sizeof(xmlChar*)*countZone);
						memset(keyzone, 0, sizeof(xmlChar*)*countZone);
						child = zonesOrvalues->xmlChildrenNode;
						while(child != NULL) {
							if(!xmlStrcmp(child->name, (const xmlChar*)"row")){
								keyzone[i] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
								propertyStruct->zonetype = (xmlChar*)child->name;
								i++;
							}
							if(!xmlStrcmp(child->name, (const xmlChar*)"window")){
								keyzone[i] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
								propertyStruct->zonetype = (xmlChar*)child->name;
								i++;
							}
							if(!xmlStrcmp(child->name, (const xmlChar*)"seat")){
								keyzone[i] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
								propertyStruct->zonetype = (xmlChar*)child->name;
								i++;
							}
							child = child->next;
						}
						propertyStruct->zone = keyzone;
						xmlFreeNode(child);
						xmlFree(key);
					}
					if(!xmlStrcmp(zonesOrvalues->name, (const xmlChar*)"values")){
						type = xmlGetProp(zonesOrvalues, (const xmlChar*)"type");
						propertyStruct->valuetype = type;
						countValue = xmlChildElementCount(zonesOrvalues);
						propertyStruct->countvalue = countValue;
						child = zonesOrvalues->xmlChildrenNode;
						keyvalue = (xmlChar***)malloc(sizeof(xmlChar**)*countValue);
						memset(keyvalue, 0, sizeof(xmlChar**)*countValue);
						while(child != NULL) {
							if(!xmlStrcmp(child->name, (const xmlChar*)"value")){
								keyvalue[j] = (xmlChar**)malloc(2*sizeof(**(keyvalue[j])));
								memset(keyvalue[j], 0, 2*sizeof(**(keyvalue[j])));
								keyvalue[j][0] = xmlGetProp(child, (const xmlChar*)"name");
								keyvalue[j][1] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
								j++;
							}
							child = child->next;
						}
						propertyStruct->value = keyvalue;
					}
					zonesOrvalues = zonesOrvalues->next;
				}
				return propertyStruct;
			}
			else{
				propertyStruct->propertyname = (xmlChar*)(&("Invalid"));
			}
		}
		property = property->next;
	}

	return propertyStruct;
}
/*convZoneValue renvoie un tableau avec le nom des zones et la valeur correspondante*/
zonevalue* convZoneValue(xmlDocPtr doc, xmlNodePtr emulator){

	xmlNodePtr 	zones;
	xmlNodePtr 	rsws;
	xmlNodePtr 	child;
	long 	 	count;
	xmlChar*** 	keyrow;
	xmlChar*** 	keyseat;
	xmlChar*** 	keywindow;
	int			i			= 0;
	int			j			= 0;
	int			k			= 0;
	zonevalue*  zoneTovalue = (zonevalue*)malloc(sizeof(zonevalue));

	zones = emulator->xmlChildrenNode;

	while(zones != NULL) {
		if(!xmlStrcmp(zones->name, (const xmlChar*)"zones")){
			rsws = zones->xmlChildrenNode;
			while(rsws != NULL){
				if(!xmlStrcmp(rsws->name, (const xmlChar*)"rows")){
					count = xmlChildElementCount(rsws);
					zoneTovalue->countrow = count;
					keyrow = (xmlChar***)malloc(sizeof(xmlChar**)*count);
					memset(keyrow, 0, sizeof(xmlChar**)*count);
					child = rsws->xmlChildrenNode;
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*)"row")) {
							keyrow[i] = (xmlChar**)malloc(2*sizeof(**(keyrow[i])));
							memset(keyrow[i], 0, 2*sizeof(**(keyrow[i])));
							keyrow[i][0] = xmlGetProp(child, (const xmlChar*)"name");
							keyrow[i][1] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
							i++;
						}
					child = child->next;
					}
					zoneTovalue->row = keyrow;
					xmlFreeNode(child);
				}
				if(!xmlStrcmp(rsws->name, (const xmlChar*)"seats")){
					count = xmlChildElementCount(rsws);
					zoneTovalue->countseat = count;
					keyseat = (xmlChar***)malloc(sizeof(xmlChar**)*count);
					memset(keyseat, 0, sizeof(xmlChar**)*count);
					child = rsws->xmlChildrenNode;
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*)"seat")) {
							keyseat[j] = (xmlChar**)malloc(2*sizeof(**(keyseat[j])));
							memset(keyseat[j], 0, 2*sizeof(**(keyseat[j])));
							keyseat[j][0] = xmlGetProp(child, (const xmlChar*)"name");
							keyseat[j][1] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
							j++;
						}
					child = child->next;
					}
					zoneTovalue->seat = keyseat;
					xmlFreeNode(child);
				}
				if(!xmlStrcmp(rsws->name, (const xmlChar*)"windows")){
					count = xmlChildElementCount(rsws);
					zoneTovalue->countwindow=count;
					keywindow = (xmlChar***)malloc(sizeof(xmlChar**)*count);
					memset(keywindow, 0, sizeof(xmlChar**)*count);
					child = rsws->xmlChildrenNode;
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*)"window")) {
							keywindow[k] = (xmlChar**)malloc(2*sizeof(**(keywindow[k])));
							memset(keywindow[k], 0, 2*sizeof(**(keywindow[k])));
							keywindow[k][0] = xmlGetProp(child, (const xmlChar*)"name");
							keywindow[k][1] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
							k++;
						}
					child = child->next;
					}
					zoneTovalue->window = keywindow;
					xmlFreeNode(child);
				}
				rsws = rsws->next;
			}

		}
		zones = zones->next;
	}

	return zoneTovalue;
}
/*getZoneValue permet d'envoyer la valeur pour la zone de la ppté corresondante*/
xmlChar* getZoneValue(zonevalue* zoneTovalue, xmlChar* zonetype, xmlChar* areaname){

	if(!xmlStrcmp(zonetype, (const xmlChar*)"row")){
		for(int i=0; i < zoneTovalue->countrow; i++){
			if(!xmlStrcmp(areaname, (zoneTovalue->row)[i][0])){
				return (zoneTovalue->row)[i][1];
			}
		}
	}

	if(!xmlStrcmp(zonetype, (const xmlChar*)"seat")){
		for(int i=0; i < zoneTovalue->countseat; i++){
			if(!xmlStrcmp(areaname, (zoneTovalue->seat[i][0]))){
				return (zoneTovalue->seat)[i][1];
			}
		}
	}

	if(!xmlStrcmp(zonetype, (const xmlChar*)"window")){
		for(int i=0; i < zoneTovalue->countwindow; i++){
			if(!xmlStrcmp(areaname, (zoneTovalue->window[i][0]))){
				return (zoneTovalue->window)[i][1];
			}
		}
	}

	printf("Not found zone value in zones\n");
	return (xmlChar*)(&("Invalid"));
}
/*requestRow permet de demander à l'utilisateur d'entrer la zone voulue s'il y a plusieurs zones definies dans la définition de la ppté et de renvoyer la valeur correspondante  */
int requestRow(propertyconfig* propertyStruct, zonevalue* zoneTovalue){

	char* 		row = (char*)malloc(sizeof(char)*SIZE);
	xmlChar* 	zoneValue;

	if(propertyStruct->countzone == (int)NULL){
		zoneValue = 0;
		printf("Is area global? If not, define your area\n");
		return 0;
	}

	if(propertyStruct->countzone == 1){
		zoneValue = getZoneValue(zoneTovalue, propertyStruct->zonetype, (propertyStruct->zone)[0]);
		return atoi((char*)zoneValue);
	}

	if(propertyStruct->countzone > 1){
		printf("Which row? ");
		scanf("%s", row);

		for(int i=0; i < propertyStruct->countzone; i++){
			if(!xmlStrcmp((propertyStruct->zone)[i], (const xmlChar*)row)){
				zoneValue =	getZoneValue(zoneTovalue, propertyStruct->zonetype, (propertyStruct->zone)[i]);
				return atoi((char*)zoneValue);
			}
		}
	}

	return -1;
}

xmlChar* treatEnumValue(propertyconfig* propertyStruct, char* val){

	for(int i=0; i < propertyStruct->countvalue; i++){
			if(!xmlStrcmp((xmlChar*)val, (propertyStruct->value[i][0]))){
				return (propertyStruct->value[i][1]);
			}
		}
	return (xmlChar*)(&("Invalid"));
}

int32_t treatIntValue(propertyconfig* propertyStruct, char* val){

	if((atoi((char*)(propertyStruct->value[0][1])) <= atoi((char*)val)) && (atoi((char*)val) <= atoi((char*)(propertyStruct->value[1][1])))){
		return atoi(val);
	}
	return -1;
}

float treatFloatValue(propertyconfig* propertyStruct, char* val){

	if((atof((char*)(propertyStruct->value[0][1])) <= atof((char*)val)) && (atof((char*)val) <= atof((char*)(propertyStruct->value[1][1])))){
		return atof(val);
	}
	return -1;
}

int getProperty(emulator::EmulatorMessage* message, char* prop, xmlNodePtr cur, xmlDocPtr doc){

	xmlNodePtr					properties;
	propertyconfig*				propertyStruct;
	zonevalue*					zoneTovalue;
	int							zoneValue;
	emulator::VehiclePropGet*	propget;

	message->set_msg_type(emulator::MsgType::GET_PROPERTY_CMD);

	propget			= message->add_prop();
	properties		= parseProperties(cur);
	propertyStruct	= getPropertyRequested(doc, properties, prop);
	zoneTovalue		= convZoneValue(doc, cur);


	if(properties == 0){
		return -1;
	}

	if(!xmlStrcmp(propertyStruct->propertyname, (const xmlChar *)"Invalid")){
		printf("Invalid property\n");
		return -1;
	}

	zoneValue = requestRow(propertyStruct, zoneTovalue);

	if(zoneValue == -1){
		printf("Invalid row\n");
		return -1;
	}

	propget->set_prop(atoi((char*)propertyStruct->propertyvalue));
	propget->set_area_id(zoneValue);


	return 0;
}

int setProperty(emulator::EmulatorMessage* message, char* prop, char* val, xmlNodePtr cur, xmlDocPtr doc){

	float							floatvalue;
	int32_t							intvalue;
	xmlNodePtr						properties;
	propertyconfig*					propertyStruct;
	zonevalue*						zoneTovalue;
	int								zoneValue;
	xmlChar*						enumValue;
	emulator::VehiclePropValue*		propset;

	message->set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);

	propset			= message->add_value();
	properties		= parseProperties(cur);
	propertyStruct	= getPropertyRequested(doc, properties, prop);
	zoneTovalue		= convZoneValue(doc, cur);

	if(properties == 0){
		return -1;
	}

	if(!xmlStrcmp(propertyStruct->propertyname, (const xmlChar *)"Invalid")){
		printf("Invalid property\n");
		return -1;
	}

	zoneValue = requestRow(propertyStruct, zoneTovalue);

	if(zoneValue == -1){
		printf("Invalid row\n");
		return -1;
	}

	propset->set_prop(atoi((char*)propertyStruct->propertyvalue));
	propset->set_area_id(zoneValue);

	if(!xmlStrcmp(propertyStruct->valuetype, (const xmlChar*)"enum")){
		enumValue = treatEnumValue(propertyStruct,val);
		if(!xmlStrcmp(enumValue, (const xmlChar *)"Invalid")){
			printf("Invalid value\n");
			return -1;
		}
		propset->add_int32_values(atoi((char*)enumValue));
	}
	else if(!xmlStrcmp(propertyStruct->valuetype, (const xmlChar*)"int32_t")){
		intvalue = treatIntValue(propertyStruct,val);
		if(intvalue == -1){
			printf("Invalid value, respect min and max\n");
			return -1;
		}
		propset->add_int32_values(intvalue);
	}
	else if(!xmlStrcmp(propertyStruct->valuetype, (const xmlChar*)"float")){
		floatvalue = treatFloatValue(propertyStruct,val);
		if(floatvalue == -1){
			printf("Invalid value, respect min and max\n");
			return -1;
		}
		propset->add_float_values(floatvalue);
	}
	return 0;
}

int getOrset(emulator::EmulatorMessage* message, xmlNodePtr cur, xmlDocPtr doc){

	char type[SIZE];
	char pro[SIZE];
	char val[SIZE];

	printf("get or set? ");
	scanf("%s", type);

	printf("property? ");
	scanf("%s", pro);

	if(!strcmp((const char*)&type, "get")){
		if(getProperty(message, (char*)&pro, cur, doc) == -1){
			xmlFreeDoc(doc);
			return -1;
		}
	}
	else if(!strcmp((const char*)&type, "set")){
		printf("value? ");
		scanf("%s", val);
		if(setProperty(message, (char*)&pro, (char*)&val, cur, doc) == -1){
	 		xmlFreeDoc(doc);
			return -1;
		}
	}
	else{
		printf("Invalid type\n");
		xmlFreeDoc(doc);
		return -1;
	}
	xmlFreeDoc(doc);
	return 0;
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
	xmlNodePtr					cur;
	xmlDocPtr 					doc;
	emulator::EmulatorMessage 	message;
	emulator::EmulatorMessage	message_recv;

	doc = parseDoc((char*)"property.xml");
	cur = parseemulator(doc);
	
	if(cur == 0){
		return -1;
	}
	if(getOrset(&message, cur, doc) == -1){
		return -1;
	}

	sock = connect_socket();
	
	if(sock == -1){
		return -1;
	}	

	sendToServer(sock, &message);
	receiveFromServer(sock, &message_recv);
	afficher_resultat(&message_recv);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
