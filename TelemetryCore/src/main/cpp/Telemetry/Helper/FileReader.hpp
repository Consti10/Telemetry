//
// Created by Constantin on 8/8/2018.
//

#ifndef FPV_VR_FILERECEIVER_H
#define FPV_VR_FILERECEIVER_H

class FileReader{
public:
    FileReader(const std::string fn,const int waitTimeMS,std::function<void(uint8_t[],int)> onDataReceivedCallback):
            filename(fn),waitTimeMS(waitTimeMS){
        this->onDataReceivedCallback=onDataReceivedCallback;
    }
    void startReading(){
        receiving=true;
        mThread=new std::thread([this] { this->receiveLoop(); });
    }
    void stopReading(){
        receiving=false;
        if(mThread->joinable()){
            mThread->join();
        }
        delete(mThread);
    }
private:
    std::function<void(uint8_t[],int)> onDataReceivedCallback;
    std::thread* mThread;
    std::atomic<bool> receiving;
    const std::string filename;
    const int waitTimeMS;
    void receiveLoop(){
        std::ifstream file (filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
        if (file.is_open()) {
            auto size = file.tellg();
            uint8_t* memblock = new uint8_t [size];
            file.seekg (0, std::ios::beg);
            file.read ((char*)memblock, size);
            file.close();
            __android_log_print(ANDROID_LOG_VERBOSE, "TR","the entire file content is in memory");
            int offset=0;
            const int chunkSize=8;
            while(receiving){
                onDataReceivedCallback(&memblock[offset],chunkSize);
                offset+=chunkSize;
                if(offset>size){
                    offset=0;
                    //return;
                }
                std::this_thread::sleep_for(std::chrono::nanoseconds(1000*1000*waitTimeMS));
            }
            delete[] memblock;
        } else {
            __android_log_print(ANDROID_LOG_VERBOSE, "TR", "Cannot open file");
        }
    }
};

#endif //FPV_VR_FILERECEIVER_H
