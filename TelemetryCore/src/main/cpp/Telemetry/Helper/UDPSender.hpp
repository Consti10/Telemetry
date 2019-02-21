//
// Created by Constantin on 9/5/2018.
//

#ifndef FPV_VR_UDPSENDER_H
#define FPV_VR_UDPSENDER_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <android/log.h>
#include <iostream>
#include <thread>
#include <atomic>

#define TAGX "UDPSender"
#define LOGDX(...) __android_log_print(ANDROID_LOG_DEBUG, TAGX, __VA_ARGS__)

class UDPSender{
private:
    int mSocket;
    const int mPort;
    struct sockaddr_in myaddr;
public:
    UDPSender(const int port):mPort(port){
        if ((mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            LOGDX("Error creating socket");
            return;
        }
        int enable=1;
        if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
            LOGDX("Error setting reuse");
        }
        memset((uint8_t *) &myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(mPort);
        if (bind(mSocket, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
            LOGDX("Error binding Port; %d", mPort);
            return;
        }
    }
    void send(uint8_t data[],int dataLength){
        auto len=sendto(mSocket,data,dataLength,0,(struct sockaddr *) &myaddr, sizeof(myaddr));
        //LOGDX("Sent:%d",len);
    }
    ~UDPSender(){
        close(mSocket);
    }
};
#endif //FPV_VR_UDPSENDER_H
