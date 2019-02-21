
#ifndef FPV_VR_UDPRECEIVER_H
#define FPV_VR_UDPRECEIVER_H

#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <android/log.h>
#include <iostream>
#include <thread>
#include <atomic>

class UDPReceiver {
public:
    UDPReceiver(const int port,const std::string name,const int CPUPriority,const int buffsize,const std::function<void(uint8_t[],int)> onDataReceivedCallback);
    void startReceiving();
    void stopReceiving();
    long getNReceivedBytes()const;
private:
    void receiveFromUDPLoop();
    std::function<void(uint8_t[],int)> onDataReceivedCallback= nullptr;
    const int mPort;
    const int mCPUPriority;
    const int mBuffsize;
    const std::string mName;
    std::string mIP="0.0.0.0";
    std::atomic<bool> receiving;
    std::atomic<long> nReceivedBytes;
    int mSocket;
    std::thread* mUDPReceiverThread;
};

#endif // FPV_VR_UDPRECEIVER_H