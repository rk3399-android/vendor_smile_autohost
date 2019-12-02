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

#include <iostream>
#include <fstream>
#include "VehicleHalProto.pb.h"

#define PORT 33452
#define INTSIZE 4

using namespace std;

struct propertyconfig{
	xmlChar* propertyid;
	xmlChar* valuetype;
	xmlChar* value;
};

struct timerproperty{
	int              fd;
	int              nbproperty;
	propertyconfig** propertyStruct;
};

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

xmlDocPtr parseDoc(char *docname){

	xmlDocPtr doc = xmlParseFile(docname);

	if(doc == NULL ){
		fprintf(stderr,"Document not parsed successfully. \n");
		xmlFreeDoc(doc);
		return 0;
	}

	return doc;
}

xmlNodePtr parseemulator(xmlDocPtr doc){

	xmlNodePtr emulator = xmlDocGetRootElement(doc);

	if(emulator == NULL){
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		xmlFreeNode(emulator);
		return 0;
	}

	if(xmlStrcmp(emulator->name, (const xmlChar*)"emulator")){
		fprintf(stderr, "document of the wrong type, root node != ");
		xmlFreeDoc(doc);
		xmlFreeNode(emulator);
		return 0;
	}

	return emulator;
}

xmlNodePtr parsescenario(xmlDocPtr doc){

	xmlNodePtr scenario = xmlDocGetRootElement(doc);

	if(scenario == NULL){
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		xmlFreeNode(scenario);
		return 0;
	}

	if(xmlStrcmp(scenario->name, (const xmlChar*)"scenario")){
		fprintf(stderr, "document of the wrong type, root node != ");
		xmlFreeDoc(doc);
		xmlFreeNode(scenario);
		return 0;
	}

	return scenario;
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

propertyconfig* getPropertyRequested(xmlDocPtr doc, xmlNodePtr properties, xmlChar* requestedName, xmlChar* requestedValue){

	propertyconfig* propertyStruct = (propertyconfig*)malloc(sizeof(propertyconfig));
	xmlNodePtr      property;
	xmlNodePtr      zonesOrvalues;
	xmlNodePtr      child;
	xmlChar*        key;
	xmlChar*        type;
	xmlChar***      keyvalue;
	long            countValue;
	int             j = 0;
	int             i = 0; 

	property = properties->xmlChildrenNode;

	while(property != NULL){
		if(!xmlStrcmp(property->name, (const xmlChar*)"property")){
			key = xmlGetProp(property, (const xmlChar*)"name");
			if(!xmlStrcmp(key, requestedName)){
				propertyStruct->propertyid = xmlGetProp(property, (const xmlChar*)"value");
				zonesOrvalues = property->xmlChildrenNode;
				while(zonesOrvalues != NULL){
					if(!xmlStrcmp(zonesOrvalues->name, (const xmlChar*)"values")){
						type = xmlGetProp(zonesOrvalues, (const xmlChar*)"type");
						propertyStruct->valuetype = type;
						countValue = xmlChildElementCount(zonesOrvalues);
						child = zonesOrvalues->xmlChildrenNode;
						keyvalue = (xmlChar***)malloc(sizeof(xmlChar**)*countValue);
						memset(keyvalue, 0, sizeof(xmlChar**)*countValue);
						while(child != NULL){
							if(!xmlStrcmp(child->name, (const xmlChar*)"value")){
								keyvalue[j] = (xmlChar**)malloc(2 * sizeof(xmlChar*));
								memset(keyvalue[j], 0, 2 * sizeof(xmlChar*));
								keyvalue[j][0] = xmlGetProp(child, (const xmlChar*)"name");
								keyvalue[j][1] = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
								j++;
							}
							child = child->next;
						}
					}
					zonesOrvalues = zonesOrvalues->next;
				}
				propertyStruct->value = (xmlChar*)(&("Invalid"));
				if(!xmlStrcmp(type, (const xmlChar*)"enum")){
					for(i = 0; i < countValue; i++){
						if(!xmlStrcmp(keyvalue[i][0], requestedValue)){
							propertyStruct->value = keyvalue[i][1];
						}
					}
				}
				else if(!xmlStrcmp(type, (const xmlChar*)"int")){
					if((atoi((char*)(keyvalue[0][1])) <= atoi((char*)(requestedValue))) && (atoi((char*)(requestedValue)) <= atoi((char*)(keyvalue[1][1])))){	
							propertyStruct->value = requestedValue;
					}
				}
				else if(!xmlStrcmp(type, (const xmlChar*)"float")){
					if((atof((char*)(keyvalue[0][1])) <= atof((char*)(requestedValue))) && (atof((char*)(requestedValue)) <= atoi((char*)(keyvalue[1][1])))){	
							propertyStruct->value = requestedValue;
					}
				}
				return propertyStruct;
			}
			else{
				propertyStruct->propertyid = (xmlChar*)(&("Invalid"));
			}
		}
		property = property->next;
	}

	return propertyStruct;
}

int main(int argc, char *argv[]){

	int                         erreur;
	int                         sock;
	int                         efd;
	int                         i          = 0;
	int                         nbproperty = 0;
	int opt;
	char* propertypath;
	char* scenariopath;
	char* ip;

	while((opt = getopt(argc, argv, "p:s:i:")) != -1){
		switch(opt){
			case 'p':
				propertypath = optarg;
			break;
			case 's':
				scenariopath = optarg;
			break;
			case 'i':
				ip = optarg;
			break;
			default: /* '?' */
				fprintf(stderr, "Usage: %s [-p property_path] [-s scenario_path] [-i adresseIP]", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	xmlDocPtr                   doc        = parseDoc(propertypath);
	xmlNodePtr                  cur        = parseemulator(doc);
	xmlDocPtr                   sce        = parseDoc(scenariopath);
	xmlNodePtr                  scenod     = parsescenario(sce);
	long                        nbTime     = xmlChildElementCount(scenod);
	xmlNodePtr                  Time       = scenod->xmlChildrenNode;
	xmlNodePtr                  properties = parseProperties(cur);
	xmlNodePtr                  property;
	xmlChar*                    key;
	xmlChar*                    name;
	xmlChar*                    value;
	emulator::EmulatorMessage   message;
	emulator::EmulatorMessage   message_recv;
	struct epoll_event          ev;
	struct epoll_event          events[100];
	ssize_t                     nr_events;
	ssize_t                     j;
	uint64_t                    count;
	int*                        tf              = (int*)malloc(sizeof(int) * nbTime);
	struct itimerspec*          ts              = (itimerspec*)malloc(sizeof(itimerspec) * nbTime);
	timerproperty*              timerpropStruct = (timerproperty*)malloc(sizeof(timerproperty) * nbTime);
	emulator::VehiclePropValue* propset;

	efd       = epoll_create(1);
	ev.events = EPOLLIN;

	memset(tf, 0, sizeof(int) * nbTime);
	memset(ts, 0, sizeof(itimerspec) * nbTime);
	memset(timerpropStruct, 0, sizeof(timerproperty) * nbTime);
	memset(&ev.data, 0, sizeof(ev.data));
	message.set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);

	while(Time != NULL){
		if(!xmlStrcmp(Time->name, (const xmlChar*)"time")){
			key = xmlGetProp(Time, (const xmlChar*)"temps");
			tf[i] = timerfd_create(CLOCK_MONOTONIC, 0);
			ts[i].it_interval.tv_sec = 0;
			ts[i].it_interval.tv_nsec = 0;
			ts[i].it_value.tv_sec = atoi((char*)key);
			ts[i].it_value.tv_nsec = 0;
			timerpropStruct[i].fd = tf[i];
			timerfd_settime(tf[i], 0, &ts[i], NULL);
			ev.data.fd = tf[i];
			epoll_ctl(efd, EPOLL_CTL_ADD, tf[i], &ev);

			property = Time->xmlChildrenNode;
			j = 0;
			timerpropStruct[i].nbproperty = xmlChildElementCount(Time);
			timerpropStruct[i].propertyStruct = (propertyconfig**)malloc(sizeof(propertyconfig*) * xmlChildElementCount(Time));
			memset(timerpropStruct[i].propertyStruct, 0, sizeof(propertyconfig*) * xmlChildElementCount(Time)); 
			while(property != NULL){
				if(!xmlStrcmp(property->name, (const xmlChar*)"property")){
					name = xmlGetProp(property, (const xmlChar*)"name");
					value = xmlGetProp(property, (const xmlChar*)"value");
					timerpropStruct[i].propertyStruct[j] = getPropertyRequested(doc, properties, name, value);
					if(!xmlStrcmp(timerpropStruct[i].propertyStruct[j]->propertyid, (const xmlChar *)"Invalid")){
						printf("Invalid property time %d, property %zd\n", i+1, j+1);
						return -1;
					}
					if(!xmlStrcmp(timerpropStruct[i].propertyStruct[j]->value, (const xmlChar *)"Invalid")){
						printf("Invalid value time %d, property %zd\n", i+1, j+1);
						return -1;
					}
					j++;
				}
				property = property->next;
			}
			i++;
		}
		Time = Time->next;
	}

	sock = connect_socket(ip);
	if(sock == -1){
		return -1;
	}
	while(1){
		nr_events = epoll_wait(efd, events, 100, -1);
		printf("nr_events %zd\n",nr_events);
		for(int k = 0; k < nr_events; k++){
			for(int m = 0; m < nbTime; m++){
				if(events[k].data.fd == timerpropStruct[m].fd){
					printf("timerpropStruct.fd%d\n",timerpropStruct[m].fd);
					read(timerpropStruct[m].fd, &count, sizeof(count));
					for(int l = 0; l < timerpropStruct[m].nbproperty; l++){
						message.clear_value();
						propset = message.add_value();
						printf("id %d\n",atoi((char*)timerpropStruct[m].propertyStruct[l]->propertyid));
						propset->set_prop(atoi((char*)timerpropStruct[m].propertyStruct[l]->propertyid));
						if((!xmlStrcmp(timerpropStruct[m].propertyStruct[l]->valuetype, (const xmlChar*)"enum")) || (!xmlStrcmp(timerpropStruct[m].propertyStruct[l]->valuetype, (const xmlChar*)"int"))){ 
							propset->add_int32_values(atoi((char*)timerpropStruct[m].propertyStruct[l]->value));
						}
						else if(!xmlStrcmp(timerpropStruct[m].propertyStruct[l]->valuetype, (const xmlChar*)"float")){
							propset->add_float_values(atof((char*)timerpropStruct[m].propertyStruct[l]->value));
						}
						sendToServer(sock, &message);
						receiveFromServer(sock, &message_recv);
					}
					epoll_ctl(efd, EPOLL_CTL_DEL, tf[m], &ev);
					if(m == nbTime - 1){
						m = 0; 
						for(int d = 0; d < nbTime; d++){
							timerfd_settime(tf[d], 0, &ts[d], NULL);
							ev.data.fd = tf[d];
							epoll_ctl(efd, EPOLL_CTL_ADD, tf[d], &ev);
							nr_events = epoll_wait(efd, events, 100, -1);
							printf("nr_events boucle %zd\n",nr_events);
						}
					}
				}
			}
		}
	}
	
	if(cur == 0){
		return -1;
	}

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
