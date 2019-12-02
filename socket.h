#ifndef SOCKET_H
#define SOCKET_H

#include "VehicleHalProto.pb.h"

int connect_socket(char* ip);
void sendToServer(int sock, emulator::EmulatorMessage* message);
void receiveFromServer(int sock, emulator::EmulatorMessage* message);

#endif
