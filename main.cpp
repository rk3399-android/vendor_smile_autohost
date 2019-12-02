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
#include "scenario.h"
#include "interactive.h"

using namespace std;

static xmlNodePtr parseemulator(xmlDocPtr doc){

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

static xmlNodePtr parseProperties(xmlNodePtr properties){

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

int main(int argc, char *argv[]){


	int        opt;
	char*      propertypath = NULL;
	char*      scenariopath = NULL;
	char*      ip = NULL;

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
	if (propertypath == NULL || ip == NULL) {
		fprintf(stderr, "Please provide ip and property.xml path !\n");
		exit(-1);
	}

	xmlDocPtr  doc = parseDoc(propertypath);
	xmlNodePtr cur = parseemulator(doc);
	xmlNodePtr properties = parseProperties(cur);

	if(cur == 0){
		return -1;
	}

	if (scenariopath ==NULL) {
		//Goto interactive;
		start_interactive(doc, cur, ip, properties);
	} else {
		// Run sceanrio
		start_scenario(doc, ip, scenariopath, properties);
	}

	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
