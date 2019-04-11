//
// Created by Constantin on 12.01.2018.
//

#include <sstream>
#include "GroundRecorder.h"
#include <filesystem>

GroundRecorder::GroundRecorder(std::string s):filename(s) {
    ofstream.open (filename.c_str());
}

void GroundRecorder::writeData(const uint8_t *data,const int data_length) {
    ofstream.write((char*)data,data_length);
}

void GroundRecorder::stop() {
    ofstream.flush();
    ofstream.close();
}

std::string GroundRecorder::findUnusedFilename(std::string directory,std::string filetype) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::stringstream filenameShort;
    //first,try with date,hours and minutes only
    filenameShort<<directory<<std::put_time(&tm, "%d-%m-%Y %H-%M")<<"."<<filetype;
    std::ifstream infile(filenameShort.str());
    if(!infile.good()){
        return filenameShort.str();
    }
    //else, also use seconds and assume this one is valid
    std::stringstream filenameLong;
    filenameLong<<directory<<std::put_time(&tm, "%d-%m-%Y %H-%M-%S")<<"."<<filetype;
    return filenameLong.str();
}
