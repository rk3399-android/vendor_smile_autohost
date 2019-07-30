#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include "VehicleHalProto.pb.h"
#include "types.h"

#define PORT 33452
#define INTSIZE 4
#define INFTIM -1


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

void setFuelOn(emulator::VehiclePropValue* value){
	value->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::FUEL_LEVEL_LOW));
	value->add_int32_values(1);
}	

void setFuelOff(emulator::VehiclePropValue* value){
	value->set_prop(static_cast<int32_t>(android::hardware::automotive::vehicle::V2_0::VehicleProperty::FUEL_LEVEL_LOW));
	value->add_int32_values(0);
}
struct auto_event {
	int fd;
	void* data;
};
int main() { 
	
	int							sock; 
    emulator::EmulatorMessage 	message;
    emulator::EmulatorMessage 	message1;
    int 						efd;
    int*						tf;
    int 						ztfd;
    int 						tfd1;
    struct itimerspec*          t;
    struct itimerspec           ts;
    struct itimerspec           ts1;
    struct epoll_event 			ev, events[5];
    ssize_t 					nr_events;
    ssize_t 					i;
    uint64_t 					count;
    
    
    t	 = (itimerspec*)malloc(sizeof(itimerspec) * 2);
    tf	 = (int*)malloc(sizeof(int) * 2);	
    memset(tf, 0, sizeof(int) * 2);
	memset(t, 0, sizeof(itimerspec) * 2);
	for (int h=0;h<2;h++){
		tf[h] = timerfd_create(CLOCK_MONOTONIC, 0);
		printf("tf%d\n",tf[h]);
	}	
	//tf[0] = timerfd_create(CLOCK_MONOTONIC, 0);
	t[0].it_interval.tv_sec = 0;
	t[0].it_interval.tv_nsec = 0;
	t[0].it_value.tv_sec = 5;
	t[0].it_value.tv_nsec = 0;
	timerfd_settime(tf[0], 0, &t[0], NULL);
	efd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = /*tfd*/tf[0];
    //struct auto_event ev1;
    //ev1.fd = tfd;
    //ev1.data = (void*)1;
    //ev.data.ptr = (void*)&ev1;
    epoll_ctl(efd, EPOLL_CTL_ADD, /*tfd*/tf[0], &ev);
	
	//tf[1] = timerfd_create(CLOCK_MONOTONIC, 0);
	t[1].it_interval.tv_sec = 0;
	t[1].it_interval.tv_nsec = 0;
	t[1].it_value.tv_sec = 11;
	t[1].it_value.tv_nsec = 0;
	timerfd_settime(tf[1], 0, &t[1], NULL);
    ev.data.fd = tf[1];
    epoll_ctl(efd, EPOLL_CTL_ADD, /*tfd*/tf[1], &ev);
	
	/*tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	ts1.it_interval.tv_sec = 0;
	ts1.it_interval.tv_nsec = 0;
	ts1.it_value.tv_sec = 5;
	ts1.it_value.tv_nsec = 0;
	timerfd_settime(tfd, 0, &ts1, NULL);*/
	
	
    
	
	/*tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 11;
	ts.it_value.tv_nsec = 0;
	timerfd_settime(tfd, 0, &ts, NULL);*/
	
   // struct auto_event ev2;
    //ev2.fd = tfd;
    //ev2.data = (void*)2;
    //ev.data.ptr = (void*)&ev2;
   // epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &ev);

	message.set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);	
	message1.set_msg_type(emulator::MsgType::SET_PROPERTY_CMD);	
	/*setFuelOn(message.add_value());
	sock = connect_socket();
    sendToServer(sock, &message);*/
    
	 while (1){
        
		printf("haha\n");
        nr_events = epoll_wait(efd, events, 5, -1);
        printf("event %d\n", nr_events);
        
        for (i = 0; i < nr_events; i++) {
			//struct auto_event* ev = (struct auto_event*)events[i].data.ptr;
			//read(ev->fd, &count, sizeof(count));
			//printf("pointeur %p\n", ev->data);
			printf("tfd0 %d,tfd1 %d\n",tf[0],tf[1]);
            if (events[i].data.fd == /*tfd*/tf[0]) {
				printf("ON");
                read(tf[0], &count, sizeof(count));
              /*  setFuelOn(message1.add_value());
				sock = connect_socket();
				sendToServer(sock, &message1);*/
            }
            else if (events[i].data.fd == /*tfd*/tf[1]) {
				printf("off");
               read(tf[1], &count, sizeof(count));
              /*  message1.clear_value();
                setFuelOff(message1.add_value());
				sock = connect_socket();
				sendToServer(sock, &message1);*/
            }
        }
    }
    
	close(tf[0]);
	close(tf[1]);
    
	google::protobuf::ShutdownProtobufLibrary();
    
    return 0; 
}
