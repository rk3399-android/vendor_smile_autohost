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

#include <iostream>
#include <fstream>
#include "VehicleHalProto.pb.h"

#define PORT 33452
#define SIZE 20

using namespace std;

struct propertyconfig{
	xmlChar* 	propertyid;
	xmlChar* 	valuetype;
	xmlChar* 	value;
};

struct timerproperty{
	int   			 fd;
	int				 nbproperty;
	propertyconfig** propertyStruct;			
};	

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

	emulator = xmlDocGetRootElement(doc);

	if(emulator == NULL) {
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		xmlFreeNode(emulator);
		return 0;
	}

	if(xmlStrcmp(emulator->name, (const xmlChar*)"emulator")) {
		fprintf(stderr, "document of the wrong type, root node != ");
		xmlFreeDoc(doc);
		xmlFreeNode(emulator);
		return 0;
	}

	return emulator;
}

xmlNodePtr parsescenario(xmlDocPtr doc){

	xmlNodePtr	scenario;

	scenario = xmlDocGetRootElement(doc);

	if(scenario == NULL) {
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		xmlFreeNode(scenario);
		return 0;
	}

	if(xmlStrcmp(scenario->name, (const xmlChar*)"scenario")) {
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
	
	propertyconfig* 	propertyStruct = (propertyconfig*)malloc(sizeof(propertyconfig));
	xmlNodePtr 			property;
	xmlNodePtr 			zonesOrvalues;
	xmlNodePtr 			child;
	xmlChar* 			key;
	xmlChar* 			type;
	xmlChar*** 			keyvalue;
	long				countValue;
	int 				j = 0;
	int 				i = 0; 

	property = properties->xmlChildrenNode;

	while(property != NULL) {
		if(!xmlStrcmp(property->name, (const xmlChar*)"property")){
			key = xmlGetProp(property, (const xmlChar*)"name");
			if(!xmlStrcmp(key, requestedName)){
				propertyStruct->propertyid = xmlGetProp(property, (const xmlChar*)"value");
				zonesOrvalues = property->xmlChildrenNode;
				while(zonesOrvalues != NULL) {
					if(!xmlStrcmp(zonesOrvalues->name, (const xmlChar*)"values")){
						type = xmlGetProp(zonesOrvalues, (const xmlChar*)"type");
						propertyStruct->valuetype = type;
						countValue = xmlChildElementCount(zonesOrvalues);
						child = zonesOrvalues->xmlChildrenNode;
						keyvalue = (xmlChar***)malloc(sizeof(xmlChar**)*countValue);
						memset(keyvalue, 0, sizeof(xmlChar**)*countValue);
						while(child != NULL) {
							if(!xmlStrcmp(child->name, (const xmlChar*)"value")){
								keyvalue[j] = (xmlChar**)malloc(2 * sizeof(**(keyvalue[j])));
								memset(keyvalue[j], 0, 2 * sizeof(**(keyvalue[j])));
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
					if((atoi((char*)(keyvalue[0][1]))<= atoi((char*)(requestedValue))) && (atoi((char*)(requestedValue)) <= atoi((char*)(keyvalue[1][1])))){	
							propertyStruct->value = requestedValue;
					}
				}
				else if(!xmlStrcmp(type, (const xmlChar*)"float")){
					if((atof((char*)(keyvalue[0][1]))<= atof((char*)(requestedValue))) && (atof((char*)(requestedValue)) <= atoi((char*)(keyvalue[1][1])))){	
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

int main() {

	int								sock;
	int								efd;
	int								i;
	int								nbproperty;
	int*							tf;
	xmlDocPtr 						doc;
	xmlDocPtr 						sce;
	xmlNodePtr						scenod;
	xmlNodePtr						cur;
	xmlNodePtr						Time;
	xmlNodePtr						property;
	xmlNodePtr						properties;
	xmlChar* 						key;
	xmlChar* 						name;
	xmlChar* 						value;
	emulator::EmulatorMessage 		message;
	struct epoll_event 				ev;
	struct epoll_event 				events[100];
	ssize_t 						nr_events;
    ssize_t 						j;
    uint64_t 						count;
    long							nbTime;
    struct itimerspec*         		ts;
    timerproperty*					timerpropStruct;
    emulator::VehiclePropValue*		propset;
    char* 							nbCycleChar;
    int								nbCycle;
    
    nbCycleChar = (char*)malloc(sizeof(char)*SIZE);
    nbproperty	= 0;
    i			= 0;
	doc 		= parseDoc((char*)"property.xml");
	cur 		= parseemulator(doc);
	sce 		= parseDoc((char*)"scenario.xml");
	scenod 		= parsescenario(sce);
	nbTime		= xmlChildElementCount(scenod);
	efd 		= epoll_create1(0);
    ev.events 	= EPOLLIN;
	Time		= scenod->xmlChildrenNode;
	properties	= parseProperties(cur);
	
	printf("How many cycles? ");
	scanf("%s", nbCycleChar);
	
	nbCycle 		= atoi(nbCycleChar);
	ts				= (itimerspec*)malloc(sizeof(itimerspec) * nbTime * nbCycle);
	tf				= (int*)malloc(sizeof(int) * nbTime * nbCycle);
	timerpropStruct = (timerproperty*)malloc(sizeof(timerproperty) * nbTime * nbCycle);
	
	memset(tf, 0, sizeof(int)*nbTime*nbCycle);
	memset(ts, 0, sizeof(itimerspec)*nbTime*nbCycle);
	memset(timerpropStruct, 0, sizeof(timerproperty)*nbTime*nbCycle);
	
	message.set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);

	
	while(Time != NULL){
		if(!xmlStrcmp(Time->name, (const xmlChar*)"time")){
			key = xmlGetProp(Time, (const xmlChar*)"temps");
			for(int g = 0; g < nbCycle; g++){
				tf[i+g*nbTime] = timerfd_create(CLOCK_MONOTONIC, 0);
				ts[i+g*nbTime].it_interval.tv_sec = 0;
				ts[i+g*nbTime].it_interval.tv_nsec = 0;
				ts[i+g*nbTime].it_value.tv_sec = atoi((char*)key)+g*80;
				ts[i+g*nbTime].it_value.tv_nsec = 0;
				timerpropStruct[i+g*nbTime].fd = tf[i+g*nbTime];
				timerfd_settime(tf[i+g*nbTime], 0, &ts[i+g*nbTime], NULL);
				ev.data.fd = tf[i+g*nbTime];
				epoll_ctl(efd, EPOLL_CTL_ADD, tf[i+g*nbTime], &ev);
				property = Time->xmlChildrenNode;
				j = 0;
				timerpropStruct[i+g*nbTime].nbproperty = xmlChildElementCount(Time);
				timerpropStruct[i+g*nbTime].propertyStruct = (propertyconfig**)malloc(sizeof(propertyconfig*) * xmlChildElementCount(Time));
				memset(timerpropStruct[i+g*nbTime].propertyStruct, 0, sizeof(propertyconfig*) * xmlChildElementCount(Time)); 
				while(property != NULL){
					if(!xmlStrcmp(property->name, (const xmlChar*)"property")){
						name = xmlGetProp(property, (const xmlChar*)"name");
						value = xmlGetProp(property, (const xmlChar*)"value");
						timerpropStruct[i+g*nbTime].propertyStruct[j] = getPropertyRequested(doc, properties, name, value);
						if(!xmlStrcmp(timerpropStruct[i+g*nbTime].propertyStruct[j]->propertyid, (const xmlChar *)"Invalid")){	
							printf("Invalid property time %d, property %zd\n", i+1, j+1);
							return -1;
						}
						if(!xmlStrcmp(timerpropStruct[i+g*nbTime].propertyStruct[j]->value, (const xmlChar *)"Invalid")){	
							printf("Invalid value time %d, property %zd\n", i+1, j+1);
							return -1;
						}		 
						j++;
					}	
					property =property->next;
				}
			}
			i++;
		}
		Time = Time->next;
	}	

	while(1){
		nr_events = epoll_wait(efd, events, 100, -1);
		for (int k = 0; k < nr_events; k++) {
			for (int m =0; m < nbTime*nbCycle; m++){
				if (events[k].data.fd == timerpropStruct[m].fd) {
					read(timerpropStruct[m].fd, &count, sizeof(count));
					for(int l = 0; l < timerpropStruct[m].nbproperty; l++){
						message.clear_value();
						propset = message.add_value();
						propset->set_prop(atoi((char*)timerpropStruct[m].propertyStruct[l]->propertyid));
						if((!xmlStrcmp(timerpropStruct[m].propertyStruct[l]->valuetype, (const xmlChar*)"enum")) || (!xmlStrcmp(timerpropStruct[m].propertyStruct[l]->valuetype, (const xmlChar*)"int"))){ 
							propset->add_int32_values(atoi((char*)timerpropStruct[m].propertyStruct[l]->value));
						}
						else if(!xmlStrcmp(timerpropStruct[m].propertyStruct[l]->valuetype, (const xmlChar*)"float")){
							propset->add_float_values(atof((char*)timerpropStruct[m].propertyStruct[l]->value));
						}
						sock = connect_socket();
						if(sock == -1){
							return -1;
						}	
						printf("%d\n",sock);
						sendToServer(sock, &message);
						if(close(sock) == -1){
							printf("Error close socket");
						}	
					}
					if (m == nbTime*nbCycle-1){
						return 0;
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


	
