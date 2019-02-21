
#include "UDPReceiver.h"
#include "Helper/CPUPriorities.hpp"
#include <arpa/inet.h>

#define TAG "UDPReceiver"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

//#define PRINT_RECEIVED_BYTES

UDPReceiver::UDPReceiver(const int port,const std::string name,const int CPUPriority,const int buffsize,const std::function<void(uint8_t[],int)> onDataReceivedCallback):
        mPort(port),mName(name),mBuffsize(buffsize),mCPUPriority(CPUPriority){
    this->onDataReceivedCallback=onDataReceivedCallback;
    receiving=false;
    nReceivedBytes=0;
}

long UDPReceiver::getNReceivedBytes()const {
    return nReceivedBytes;
}

void UDPReceiver::startReceiving() {
    receiving=true;
    mUDPReceiverThread=new std::thread([this] { this->receiveFromUDPLoop(); });
}

void UDPReceiver::stopReceiving() {
    if(receiving== false){
        LOGD("UDP Receiver %s already stopped",mName.c_str());
    }
    receiving=false;
    shutdown(mSocket,SHUT_RD);
    mUDPReceiverThread->join();
    close(mSocket);
    delete(mUDPReceiverThread);
}

void UDPReceiver::receiveFromUDPLoop() {
    if ((mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        LOGD("Error creating socket");
        return;
    }
    int enable = 1;
    if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        LOGD("Error setting reuse");
    }
    setCPUPriority(mCPUPriority,mName);
    struct sockaddr_in myaddr;
    memset((uint8_t *) &myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(mPort);
    if (bind(mSocket, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        LOGD("Error binding Port; %d", mPort);
        return;
    }
    uint8_t buff[mBuffsize];
    sockaddr_in source;
    socklen_t sourceLen= sizeof(sockaddr_in);
    while (receiving) {
        ssize_t message_length = recvfrom(mSocket, buff, (size_t)mBuffsize, MSG_WAITALL,(sockaddr*)&source,&sourceLen);
        //ssize_t message_length = recv(mSocket, buff, (size_t) mBuffsize, MSG_WAITALL);
        if (message_length > 0) { //else -1 was returned;timeout/No data received
            onDataReceivedCallback(buff, (int) message_length);
            const char* p=inet_ntoa(source.sin_addr);
            nReceivedBytes+=message_length;
            std::string s1=std::string(p);
            if(mIP!=s1){
                mIP=s1;
            }
        }
#ifdef PRINT_RECEIVED_BYTES
        LOGD("%s: received %d bytes\n", mName.c_str(),(int) message_length);
#endif
    }
}


