//
// Created by Constantin on 10.10.2017.
//

#include "TelemetryReceiver.h"
#include <cstdint>
#include <jni.h>
#include "IDT.hpp"
#include "Helper/PositionHelper.hpp"
#include "Helper/StringHelper.hpp"
#include "Helper/CPUPriorities.hpp"

#include <locale>
#include <codecvt>
#include <android/asset_manager_jni.h>

extern "C"{
#include "cFiles/ltm.h"
#include "cFiles/frsky.h"
#include "cFiles/mavlink2.h"
#include "cFiles/smartport.h"
}

#define TAG "TelemetryReceiver"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

static constexpr const wchar_t ICON_BATTERY=(wchar_t)192;
static constexpr const wchar_t ICON_CHIP=(wchar_t)192+1;
static constexpr const wchar_t ICON_HOME=(wchar_t)192+2;
static constexpr const wchar_t ICON_LATITUDE=(wchar_t)192+3;
static constexpr const wchar_t ICON_LONGITUDE=(wchar_t)192+4;
static constexpr const wchar_t ICON_SATELITE=(wchar_t)192+5;


int TelemetryReceiver::getTelemetryPort(const SettingsN &settingsN, int T_Protocol) {
    int port=5700;
    switch (T_Protocol){
        case TelemetryReceiver::NONE:break;
        case TelemetryReceiver::XLTM :port=settingsN.getInt(IDT::T_LTMPort);break;
        case TelemetryReceiver::MAVLINK:port=settingsN.getInt(IDT::T_MAVLINKPort);break;
        case TelemetryReceiver::XSMARTPORT:port=settingsN.getInt(IDT::T_SMARTPORTPort);break;
        case TelemetryReceiver::FRSKY:port=settingsN.getInt(IDT::T_FRSKYPort);break;
    }
    return port;
}

TelemetryReceiver::TelemetryReceiver(const SettingsN& settingsN,const char* DIR):
        T_Protocol(static_cast<PROTOCOL_OPTIONS >(settingsN.getInt(IDT::T_PROTOCOL,1))),T_Port(getTelemetryPort(settingsN,T_Protocol)),
        EZWBS_Protocol(static_cast<EZWB_STATUS_PROTOCOL>(settingsN.getInt(IDT::EZWBS_Protocol))),EZWBS_Port(settingsN.getInt(IDT::EZWBS_Port)),
        MAVLINK_FLIGHTMODE_QUADCOPTER(settingsN.getBoolean(IDT::T_MAVLINK_FLIGHTMODE_QUADCOPTER)),
        BATT_CAPACITY_MAH(settingsN.getInt(IDT::T_BATT_CAPACITY_MAH)),BATT_CELLS_N(settingsN.getInt(IDT::T_BATT_CELLS_N)),
        BATT_CELLS_V_WARNING1_ORANGE(settingsN.getFloat(IDT::T_BATT_CELLS_V_WARNING1_ORANGE)),BATT_CELLS_V_WARNING2_RED(settingsN.getFloat(IDT::T_BATT_CELLS_V_WARNING2_RED)),
        BATT_CAPACITY_MAH_USED_WARNING(settingsN.getInt(IDT::T_BATT_CAPACITY_MAH_USED_WARNING)),
        ORIGIN_POSITION_ANDROID(settingsN.getBoolean(IDT::T_ORIGIN_POSITION_ANDROID)),
        SOURCE_TYPE(static_cast<SOURCE_TYPE_OPTIONS >(settingsN.getInt(IDT::T_SOURCE))),
        ENABLE_GROUND_RECORDING(settingsN.getBoolean(IDT::T_GROUND_RECORDING)),
        GROUND_RECORDING_DIRECTORY(DIR),
        T_PLAYBACK_FILENAME(settingsN.getString(IDT::T_PLAYBACK_FILENAME)){
    resetNReceivedTelemetryBytes();
    std::memset (&uav_td, 0, sizeof(uav_td));
    uav_td.Pitch_Deg=10; //else you cannot see the AH 3D Quad,since it is totally flat
    std::memset (&originData, 0, sizeof(originData));
    originData.Latitude_dDeg=0;
    originData.Longitude_dDeg=0;
    originData.hasBeenSet=false;
    originData.writeByTelemetryProtocol=!ORIGIN_POSITION_ANDROID;
    std::memset (&wifibroadcast_rx_status_forward, 0, sizeof(wifibroadcast_rx_status_forward));
    for (auto &i : wifibroadcast_rx_status_forward.adapter) {
        i.current_signal_dbm=-99;
    }
    wifibroadcast_rx_status_forward.wifi_adapter_cnt=1;
}


void TelemetryReceiver::startReceiving(AAssetManager* assetManager) {
    assert(mTelemetryDataReceiver==nullptr);
    assert(mEZWBDataReceiver== nullptr);
    assert(mTestFileReader== nullptr);
    if(ENABLE_GROUND_RECORDING){ //&& SOURCE_TYPE==UDP
        const auto filename=GroundRecorder::findUnusedFilename(GROUND_RECORDING_DIRECTORY,getProtocolAsString());
        //LOGD("%s",filename.c_str());
        mGroundRecorder=new GroundRecorder(filename);
    }
    if(SOURCE_TYPE==UDP){
        if(T_Protocol!=TelemetryReceiver::NONE ){
            std::function<void(uint8_t data[],int data_length)> f= [=](uint8_t data[],int data_length) {
                this->onUAVTelemetryDataReceived(data,data_length);
            };
            mTelemetryDataReceiver=new UDPReceiver(T_Port,"TelemetryReceiver receiver",CPU_PRIORITY_UDPRECEIVER_TELEMETRY,1024,f);
            mTelemetryDataReceiver->startReceiving();
        }
        //ezWB is sending telemetry packets 128 bytes big. To speed up performance, i have a buffer  of 1024 bytes on the receiving end, though. This
        //should not add any additional latency
        if(EZWBS_Protocol==EZWB_16_rc6){
            std::function<void(uint8_t data[],int data_length)> f2 = [=](uint8_t data[],int data_length) {
                this->onEZWBStatusDataReceived(data, data_length);
            };
            mEZWBDataReceiver=new UDPReceiver(EZWBS_Port,"EZ-WB Status receiver",CPU_PRIORITY_UDPRECEIVER_TELEMETRY,1024,f2);
            mEZWBDataReceiver->startReceiving();
        }
    }else if(SOURCE_TYPE==FILE || SOURCE_TYPE==ASSETS){
        if(T_Protocol!=TelemetryReceiver::NONE){
            std::string protocol;
            int waitTimeMS=5;
            if(T_Protocol==TelemetryReceiver::MAVLINK){
                waitTimeMS=1;
            }
            if(SOURCE_TYPE==FILE){
                //we don't check if we are playing the right file type !
                mTestFileReader=new FileReader(T_PLAYBACK_FILENAME,
                                               waitTimeMS,[this](uint8_t* d,int len) { this->onUAVTelemetryDataReceived(d,len); },64);
            }else{
                mTestFileReader=new FileReader(assetManager,"testlog."+getProtocolAsString(),
                                               waitTimeMS,[this](uint8_t* d,int len) { this->onUAVTelemetryDataReceived(d,len); },64);
            }
            mTestFileReader->startReading();
        }
    }
}

void TelemetryReceiver::stopReceiving() {
    if(mTelemetryDataReceiver!= nullptr){
        mTelemetryDataReceiver->stopReceiving();
        delete(mTelemetryDataReceiver);
        mTelemetryDataReceiver= nullptr;
    }
    if(mEZWBDataReceiver!= nullptr){
        mEZWBDataReceiver->stopReceiving();
        delete(mEZWBDataReceiver);
        mEZWBDataReceiver= nullptr;
    }
    if(mTestFileReader!= nullptr){
        mTestFileReader->stopReading();
        delete(mTestFileReader);
        mTestFileReader= nullptr;
    }
    if(mGroundRecorder!= nullptr){
        //LOGD("XX%s",mGroundRecorder->filename.c_str());
        mGroundRecorder->stop();
        delete(mGroundRecorder);
        mGroundRecorder=nullptr;
    }
}

void TelemetryReceiver::onUAVTelemetryDataReceived(uint8_t data[],int data_length){
    switch (T_Protocol){
        case TelemetryReceiver::XLTM:
            ltm_read(&uav_td,&originData,data,data_length);
            break;
        case TelemetryReceiver::MAVLINK:
            mavlink_read_v2(&uav_td,&originData,data,data_length);
            break;
        case TelemetryReceiver::XSMARTPORT:
            smartport_read(&uav_td,data,data_length);
            break;
        case TelemetryReceiver::FRSKY:
            frsky_read(&uav_td,data,data_length);
            break;
        default:
            LOGD("TelR ERROR %d",T_Protocol);
            assert(false);
            break;
    }
    nTelemetryBytes+=data_length;
    if(T_Protocol==TelemetryReceiver::XLTM){
        uav_td.BatteryPack_P=(int8_t)(uav_td.BatteryPack_mAh/BATT_CAPACITY_MAH*100.0f);
    }
    if(mGroundRecorder!= nullptr){
        mGroundRecorder->writeData(data,data_length);
    }
}

void TelemetryReceiver::onEZWBStatusDataReceived(uint8_t *data, int data_length){
    nWIFIBRADCASTBytes+=data_length;
    if(data_length==WIFIBROADCAST_RX_STATUS_FORWARD_SIZE_BYTES){
        memcpy(&wifibroadcast_rx_status_forward,data,WIFIBROADCAST_RX_STATUS_FORWARD_SIZE_BYTES);
        nWIFIBROADCASTParsedPackets++;
    }else{
        nWIFIBRADCASTFailedPackets++;
    }
}

const int TelemetryReceiver::getNReceivedTelemetryBytes()const {
    return (int)(nTelemetryBytes);
}

void TelemetryReceiver::resetNReceivedTelemetryBytes() {
    nTelemetryBytes=0;
}

void TelemetryReceiver::setHome(double latitude, double longitude,double attitude) {
    //When using LTM we also get the home data by the protocol
    originData.Latitude_dDeg=latitude;
    originData.Longitude_dDeg=longitude;
    //originData.Altitude_m=(float)attitude;
    originData.hasBeenSet=true;
}

const UAVTelemetryData* TelemetryReceiver::getUAVTelemetryData()const{
    return &uav_td;
}

const wifibroadcast_rx_status_forward_t* TelemetryReceiver::get_ez_wb_forward_data() const{
    return &wifibroadcast_rx_status_forward;
}

const OriginData *TelemetryReceiver::getOriginData() const {
    return &originData;
}
const long TelemetryReceiver::getNEZWBPacketsParsingFailed()const {
    return nWIFIBRADCASTFailedPackets;
}

const int TelemetryReceiver::getBestDbm()const{
    //Taken from ez-wifibroadcast OSD.
    int cnt =  wifibroadcast_rx_status_forward.wifi_adapter_cnt;
    cnt = (cnt<=6) ? cnt : 6;
    int best_dbm = -100;
    // find out which card has best signal
    for(int i=0; i<cnt; i++) {
        const int curr_dbm=wifibroadcast_rx_status_forward.adapter[i].current_signal_dbm;
        if (best_dbm < curr_dbm) best_dbm = curr_dbm;
    }
    return best_dbm;
}

void TelemetryReceiver::setDecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms) {
    appOSDData.decoder_fps=currentFPS;
    appOSDData.decoder_bitrate_kbits=currentKiloBitsPerSecond;
    appOSDData.avgParsingTime_ms=avgParsingTime_ms;
    appOSDData.avgWaitForInputBTime_ms=avgWaitForInputBTime_ms;
    appOSDData.avgDecodingTime_ms=avgDecodingTime_ms;
}

void TelemetryReceiver::setOpenGLFPS(float fps) {
    appOSDData.opengl_fps=fps;
}

void TelemetryReceiver::setFlightTime(float timeSeconds) {
    appOSDData.flight_time_seconds=timeSeconds;
}


const TelemetryReceiver::MTelemetryValue TelemetryReceiver::getTelemetryValue(TelemetryValueIndex index) const {
    MTelemetryValue ret = TelemetryReceiver::MTelemetryValue();
    ret.warning=0;
    switch (index){
        case BATT_VOLTAGE:{
            ret.prefix=ICON_BATTERY;
            ret.prefixIsIcon=true;
            ret.prefixScale=1.2f;
            ret.value= doubleToString(uav_td.BatteryPack_V, 5, 2);
            ret.metric=L"V";
            float w1=BATT_CELLS_V_WARNING1_ORANGE*BATT_CELLS_N;
            float w2=BATT_CELLS_V_WARNING2_RED*BATT_CELLS_N;
            if(uav_td.BatteryPack_V<w1){
                ret.warning=1;
            }if(uav_td.BatteryPack_V<w2){
                ret.warning=2;
            }
        }
            break;
        case BATT_CURRENT:{
            ret.prefix=ICON_BATTERY;
            ret.prefixIsIcon=true;
            ret.prefixScale=1.2f;
            ret.value= doubleToString(uav_td.BatteryPack_A, 5, 2);
            ret.metric=L"A";
        }
            break;
        case BATT_USED_CAPACITY:{
            ret.prefix=ICON_BATTERY;
            ret.prefixIsIcon=true;
            ret.prefixScale=1.2f;
            float val=uav_td.BatteryPack_mAh;
            ret.value=intToString((int)val,5);
            ret.metric=L"mAh";
            if(val>BATT_CAPACITY_MAH_USED_WARNING){
                ret.warning=2;
            }
        }
            break;
        case BATT_PERCENTAGE:{
            ret.prefix=ICON_BATTERY;
            ret.prefixIsIcon=true;
            ret.prefixScale=1.2f;
            float perc;
            float capacity=BATT_CAPACITY_MAH;
            if(T_Protocol==TelemetryReceiver::MAVLINK){
                perc=uav_td.BatteryPack_P;
            }else{
                perc=(capacity-uav_td.BatteryPack_mAh)/capacity*100.0f;
            }
            //LOGV3("%d",uav_td.BatteryPack_mAh);
            ret.value=intToString((int)std::round(perc),3);
            ret.metric=L"%";
            if(perc<20.0f){
                ret.warning=1;
            }if(perc<10){
                ret.warning=2;
            }
        }
            break;
        case ALTITUDE_GPS:{
            ret.prefix=L"Alt(GPS)";
            ret.value=doubleToString(uav_td.AltitudeGPS_m,3,1);
            ret.metric=L"m";
        }
            break;
        case ALTITUDE_BARO:{
            ret.prefix=L"Alt(B)";
            ret.value=doubleToString(uav_td.AltitudeBaro_m,3,1);
            ret.metric=L"m";
        }
            break;
        case LONGITUDE:{
            ret.prefix=ICON_LONGITUDE;
            ret.prefixIsIcon=true;
            ret.value= doubleToString(uav_td.Longitude_dDeg, 10, 8);
            ret.metric=L"";
        }
            break;
        case LATITUDE:{
            ret.prefix=ICON_LATITUDE;
            ret.prefixIsIcon=true;
            ret.value= doubleToString(uav_td.Latitude_dDeg, 10, 8);
            ret.metric=L"";
        }
            break;
        case HS_GROUND:{
            ret.prefix=L"HS";
            ret.value= doubleToString(uav_td.SpeedGround_KPH, 5, 2);
            ret.metric=L"km/h";
        }
            break;
        case HS_AIR:{
            ret.prefix=L"HS";
            ret.value= doubleToString(uav_td.SpeedAir_KPH, 5, 2);
            ret.metric=L"km/h";
        }
            break;
        case FLIGHT_TIME:{
            ret.prefix=L"Time";
            float time=appOSDData.flight_time_seconds;
            if(time<60){
                ret.value=intToString((int)std::round(time),4);
                ret.metric=L"sec";
            }else{
                ret.value=intToString((int)std::round(time/60),4);
                ret.metric=L"min";
            }
        }
            break;
        case HOME_DISTANCE:{
            ret.prefix+=ICON_HOME;
            ret.prefixIsIcon=true;
            if(!originData.hasBeenSet){
                ret.value=L"No origin";
                ret.metric=L"";
            }else{
                int distanceM=(int)distance_between(uav_td.Latitude_dDeg,uav_td.Longitude_dDeg,originData.Latitude_dDeg,originData.Longitude_dDeg);
                if(distanceM>1000){
                    ret.value=doubleToString(distanceM/1000.0,5,1);
                    ret.metric=L"km";
                }else{
                    ret.value=intToString(distanceM,5);
                    ret.metric=L"m";
                }
            }
        }
            break;
        case DECODER_FPS:{
            ret.prefix=L"Dec";
            ret.value=intToString((int)std::round(appOSDData.decoder_fps),4);
            ret.metric=L"fps";
        }
            break;
        case DECODER_BITRATE:{
            ret.prefix=L"Dec";
            float kbits=appOSDData.decoder_bitrate_kbits;
            if(kbits>1024){
                float mbits=kbits/1024.0f;
                ret.value= doubleToString(mbits, 6, 1);
                ret.metric=L"mb/s";
            }else{
                ret.value= doubleToString(kbits, 6, 1);
                ret.metric=L"kb/s";
            }
        }
            break;
        case OPENGL_FPS:{
            ret.prefix=L"OGL";
            ret.value=intToString((int)std::round(appOSDData.opengl_fps),4);
            ret.metric=L"fps";
        }
            break;
        case EZWB_DOWNLINK_VIDEO_RSSI:{
            ret.prefix=L"ezWB";
            ret.value=intToString(getBestDbm(),5);
            ret.metric=L"dBm";
        }
            break;
        case RX_1:{
            ret.prefix=L"RX1";
            ret.value=intToString((int)uav_td.RSSI1_Percentage_dBm,4);
            if(T_Protocol==TelemetryReceiver::MAVLINK){
                ret.metric=L"%";
            }else{
                ret.metric=L"dBm";
            }
        }
            break;
        case SATS_IN_USE:{
            ret.prefix+=ICON_SATELITE;
            ret.prefixIsIcon=true;
            ret.value=intToString(uav_td.SatsInUse,3);
            ret.metric=L"";
        }
            break;
        case VS:{
            ret.prefix=L"VS";
            ret.value= doubleToString(uav_td.SpeedClimb_KPH, 5, 2);
            ret.metric=L"km/h";
        }
            break;
        case DECODER_LATENCY_DETAILED:{
            ret.prefix=L"Dec";
            ret.value=doubleToString(appOSDData.avgParsingTime_ms,2,1)+L","+
                      doubleToString(appOSDData.avgWaitForInputBTime_ms,2,1)+L","+
                      doubleToString(appOSDData.avgDecodingTime_ms,2,1);
            ret.metric=L"ms";
        }
            break;
        case DECODER_LATENCY_SUM:{
            ret.prefix=L"Dec";
            float total=appOSDData.avgParsingTime_ms+appOSDData.avgWaitForInputBTime_ms+appOSDData.avgDecodingTime_ms;
            ret.value=doubleToString(total,4,2);
            ret.metric=L"ms";
        }
            break;
        case FLIGHT_STATUS_MAV_ONLY:{
            if(T_Protocol==TelemetryReceiver::MAVLINK){
                ret.value=getMAVLINKFlightMode();
            }else{
                ret.value=L"MAV only";
            }
            ret.metric=L"";
        }
            break;
        case EZWB_UPLINK_RC_RSSI:{
            ret.value=intToString(wifibroadcast_rx_status_forward.current_signal_air,5);
            ret.metric=L"dBm";
        }
            break;
        case EZWB_UPLINK_RC_BLOCKS:{
            std::wstring s1 = intToString((int) wifibroadcast_rx_status_forward.lost_packet_cnt_rc, 6);
            std::wstring s2 = intToString((int) wifibroadcast_rx_status_forward.lost_packet_cnt_telemetry_up , 6);
            ret.value=s1+L"/"+s2;
            ret.metric=L"";
        }
            break;
        case EZWB_STATUS_AIR:{
            std::wstring s2= intToString(wifibroadcast_rx_status_forward.cpuload_air, 2);
            std::wstring s3=L"%";
            std::wstring s4=intToString(wifibroadcast_rx_status_forward.temp_air,3);
            std::wstring s5=L"°";
            ret.value=s2+s3+L" "+s4+s5;
            ret.prefix+=ICON_CHIP;
            ret.prefixIsIcon=true;
        }
            break;
        case EZWB_STATUS_GROUND:{
            std::wstring s2= intToString(wifibroadcast_rx_status_forward.cpuload_gnd, 2);
            std::wstring s3=L"%";
            std::wstring s4=intToString(wifibroadcast_rx_status_forward.temp_gnd,3);
            std::wstring s5=L"°";
            ret.value=s2+s3+L" "+s4+s5;
            ret.prefix+=ICON_CHIP;
            ret.prefixIsIcon=true;
        }
            break;
        case EZWB_BLOCKS:{
            std::wstring s1 = intToString((int) wifibroadcast_rx_status_forward.damaged_block_cnt, 6);
            std::wstring s2 = intToString((int) wifibroadcast_rx_status_forward.lost_packet_cnt, 6);
            ret.value=s1+L"/"+s2;
            ret.prefix=L"";
        }
            break;
        case EZWB_ADAPTER1_RSSI:{
            ret=getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(1);
        }break;
        case EZWB_ADAPTER2_RSSI:{
            ret=getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(2);
        }break;
        case EZWB_ADAPTER3_RSSI:{
            ret=getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(3);
        }break;
        case EZWB_ADAPTER4_RSSI:{
            ret=getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(4);
        }break;
        case EZWB_ADAPTER5_RSSI:{
            ret=getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(5);
        }break;
        case EZWB_ADAPTER6_RSSI:{
            ret=getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(0);
        }break;
        default:
            ret.prefix=L"P";
            ret.value=L"V";
            ret.metric=L"M";
            break;
    }
    return ret;
}

const TelemetryReceiver::MTelemetryValue
TelemetryReceiver::getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(int adapter) const {
    MTelemetryValue ret = TelemetryReceiver::MTelemetryValue();
    ret.warning=0;
    if(adapter>6 || adapter<0){
        adapter=0;
        LOGD("Adapter error");
    }
    std::wstring s1=intToString((int)wifibroadcast_rx_status_forward.adapter[adapter].current_signal_dbm,4);
    std::wstring s2=L"dBm [";
    std::wstring s3=intToString((int)wifibroadcast_rx_status_forward.adapter[adapter].received_packet_cnt,7);
    std::wstring s4=L"]";
    ret.value=s1+s2+s3+s4;
    return ret;
}


const std::string TelemetryReceiver::getStatisticsAsString()const {
    std::ostringstream ostream;
    if(SOURCE_TYPE==UDP){
        if(T_Protocol!=TelemetryReceiver::NONE){
            ostream<<"\nListening for "+getProtocolAsString()+" telemetry on port "<<T_Port;
            ostream<<"\nReceived: "<<nTelemetryBytes<<"B"<<" | Packets:"<<uav_td.validmsgsrx;
            ostream<<"\n";
        }else{
            ostream<<"UAV telemetry disabled\n";
        }
        if(EZWBS_Protocol==EZWB_16_rc6){
            ostream<<"\nListening for WIFIBROADCAST telemetry on port "<<EZWBS_Port;
            ostream<<"\nReceived: "<<nWIFIBRADCASTBytes<<"B | Packets:"<<nWIFIBROADCASTParsedPackets;
            ostream<<"\n";
        }else{
            ostream<<"\nWIFIBROADCAST telemetry disabled\n";
        }
    }else{
        ostream<<"Source type==File. ("+getProtocolAsString()+") Select UDP as data source\n";
        ostream<<"nTelemetryBytes"<<nTelemetryBytes<<"\n";
        ostream<<"nParsedBytes"<<mTestFileReader->getNReceivedbytes()<<"\n";
        ostream<<"Packets:"<<uav_td.validmsgsrx<<"\n";
    }
    return ostream.str();
}

const std::wstring TelemetryReceiver::getMAVLINKFlightMode() const {
    std::wstring mode;
    if(MAVLINK_FLIGHTMODE_QUADCOPTER){
        switch (uav_td.FlightMode_MAVLINK) {
            case 0:mode = L"STAB";break;
            case 1:mode = L"ACRO";break;
            case 2:mode = L"ALTHOLD";break;
            case 3:mode = L"AUTO";break;
            case 4:mode = L"GUIDED";break;
            case 5:mode = L"LOITER";break;
            case 6:mode = L"RTL";break;
            case 7:mode = L"CIRCLE";break;
            case 9:mode = L"LAND";break;
            case 11:mode = L"DRIFT";break;
            case 13:mode = L"SPORT";break;
            case 14:mode = L"FLIP";break;
            case 15:mode = L"AUTOTUNE";break;
            case 16:mode = L"POSHOLD";break;
            case 17:mode = L"BRAKE";break;
            case 18:mode = L"THROW";break;
            case 19:mode = L"AVOIDADSB";break;
            case 20:mode = L"GUIDEDNOGPS";break;
            default:mode = L"-----";break;
        }
    }else{
        switch (uav_td.FlightMode_MAVLINK) {
            case 0: mode = L"MAN"; break;
            case 1: mode = L"CIRC"; break;
            case 2: mode = L"STAB"; break;
            case 3: mode = L"TRAI"; break;
            case 4: mode = L"ACRO"; break;
            case 5: mode = L"FBWA"; break;
            case 6: mode = L"FBWB"; break;
            case 7: mode = L"CRUZ"; break;
            case 8: mode = L"TUNE"; break;
            case 10: mode = L"AUTO"; break;
            case 11: mode = L"RTL"; break;
            case 12: mode = L"LOIT"; break;
            case 15: mode = L"GUID"; break;
            case 16: mode = L"INIT"; break;
            default:
                mode = L"-----";
                break;
        }
    }
    if(!uav_td.FlightMode_MAVLINK_armed){
        mode=L"["+mode+L"]";
    }
    return mode;
}

const std::string TelemetryReceiver::getAllTelemetryValuesAsString() const {
    std::wstringstream ss;
    for( int i = TelemetryValueIndex ::DECODER_FPS; i != TelemetryValueIndex::EZWB_ADAPTER6_RSSI; i++ ){
        const auto indx = static_cast<TelemetryValueIndex>(i);
        const auto val=getTelemetryValue(indx);
        ss<<val.prefix<<" "<<val.value<<" "<<val.metric<<"\n";
    }
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    const std::string converted_str = converter.to_bytes( ss.str());
    return converted_str;
}

const std::string TelemetryReceiver::getEZWBDataAsString()const{
    std::ostringstream ostringstream1;
    const wifibroadcast_rx_status_forward_t *data = get_ez_wb_forward_data();
    ostringstream1 << "damaged_block_cnt:" << data->damaged_block_cnt << "\n";
    ostringstream1 << "lost_packet_cnt:" << data->lost_packet_cnt << "\n";
     ostringstream1 << "skipped_packet_cnt:" << data->skipped_packet_cnt << "\n";
     ostringstream1 << "received_packet_cnt:" << data->received_packet_cnt << "\n";
     ostringstream1 << "kbitrate:" << data->kbitrate << "\n";
     ostringstream1 << "kbitrate_measured:" << data->kbitrate_measured << "\n";
     ostringstream1 << "kbitrate_set:" << data->kbitrate_set << "\n";
     ostringstream1 << "lost_packet_cnt_telemetry_up:" << data->lost_packet_cnt_telemetry_up << "\n";
     ostringstream1 << "lost_packet_cnt_telemetry_down:" << data->lost_packet_cnt_telemetry_down<< "\n";
     ostringstream1 << "lost_packet_cnt_msp_up:" << data->lost_packet_cnt_msp_up << "\n";
     ostringstream1 << "lost_packet_cnt_msp_down:" << data->lost_packet_cnt_msp_down << "\n";
     ostringstream1 << "lost_packet_cnt_rc:" << data->lost_packet_cnt_rc << "\n";
     ostringstream1 << "current_signal_air:" << (int) data->current_signal_air << "\n";
     ostringstream1 << "joystick_connected:" << (int) data->joystick_connected << "\n";
     ostringstream1 << "cpuload_gnd:" << (int) data->cpuload_gnd << "\n";
     ostringstream1 << "temp_gnd:" << (int) data->temp_gnd << "\n";
     ostringstream1 << "cpuload_air:" << (int) data->cpuload_air << "\n";
     ostringstream1 << "temp_air:" << (int) data->temp_air << "\n";
     ostringstream1 << "wifi_adapter_cnt:" << data->wifi_adapter_cnt << "\n";
     for (int i = 0; i < data->wifi_adapter_cnt; i++) {
         ostringstream1 << "Adapter" << i << ":" << (int) data->adapter[i].current_signal_dbm
                        << (int) data->adapter[i].received_packet_cnt << "\n";
     }

     return ostringstream1.str();
}

float TelemetryReceiver::getHeadingHome_Deg() const {
    return (float)course_to(uav_td.Latitude_dDeg,uav_td.Longitude_dDeg,originData.Latitude_dDeg,originData.Longitude_dDeg);
}

const float TelemetryReceiver::getCourseOG_Deg() const {
    return uav_td.CourseOG_Deg;
}

const float TelemetryReceiver::getHeading_Deg() const {
    return uav_td.Heading_Deg;
}

const std::string TelemetryReceiver::getProtocolAsString() const {
    std::stringstream ss;
    switch (T_Protocol){
        case TelemetryReceiver::XLTM:ss<<"ltm";break;
        case TelemetryReceiver::FRSKY:ss<<"frsky";break;
        case TelemetryReceiver::MAVLINK:ss<<"mavlink";break;
        default:assert(false);break;
    }
    return ss.str();
}


//----------------------------------------------------JAVA bindings---------------------------------------------------------------
#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_constantin_telemetry_core_TelemetryReceiver_##method_name

inline jlong jptr(TelemetryReceiver *videoPlayerN) {
    return reinterpret_cast<intptr_t>(videoPlayerN);
}
inline TelemetryReceiver *native(jlong ptr) {
    return reinterpret_cast<TelemetryReceiver *>(ptr);
}

extern "C" {
JNI_METHOD(jlong , createInstance)
(JNIEnv *env,jclass unused,jobject context,jstring groundRecordingDirectory) {
    SettingsN settingsN(env,context,"pref_telemetry");
    const char *str = env->GetStringUTFChars(groundRecordingDirectory, nullptr);
    auto* telemetryReceiver = new TelemetryReceiver(settingsN,str);
    env->ReleaseStringUTFChars(groundRecordingDirectory,str);
    return jptr(telemetryReceiver);
}

JNI_METHOD(void, deleteInstance)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* telemetryReceiver=native(testReceiverN);
    delete (telemetryReceiver);
}

JNI_METHOD(void, startReceiving)
(JNIEnv *env,jclass unused,jlong testReceiverN,jobject assetManager) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    AAssetManager* mgr=AAssetManager_fromJava(env,assetManager);
    testRecN->startReceiving(mgr);
}

JNI_METHOD(void, stopReceiving)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    testRecN->stopReceiving();
}

JNI_METHOD(jstring , getStatisticsAsString)
(JNIEnv *env,jclass unused,jlong telemetryReceiverN) {
    TelemetryReceiver* telRecN=native(telemetryReceiverN);
    jstring ret = env->NewStringUTF( telRecN->getStatisticsAsString().c_str());
    return ret;
}

JNI_METHOD(jstring , getEZWBDataAsString)
(JNIEnv *env,jclass unused,jlong telemetryReceiver) {
    TelemetryReceiver* telRecN=native(telemetryReceiver);
    std::string s = telRecN->getEZWBDataAsString();
    jstring ret = env->NewStringUTF(s.c_str());
    return ret;
}

JNI_METHOD(jstring , getTelemetryDataAsString)
(JNIEnv *env,jclass unused,jlong telemetryReceiver) {
    TelemetryReceiver* telRecN=native(telemetryReceiver);
    const std::string s=telRecN->getAllTelemetryValuesAsString();
    jstring ret = env->NewStringUTF(s.c_str());
    return ret;
}
JNI_METHOD(jboolean , anyTelemetryDataReceived)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    bool ret = (testRecN->getNReceivedTelemetryBytes() > 0);
    return (jboolean) ret;
}

JNI_METHOD(jboolean, receivingEZWBButCannotParse)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    return (jboolean) (testRecN->getNEZWBPacketsParsingFailed() > 0);
}

JNI_METHOD(jboolean , isEZWBIpAvailable)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRecN=native(testReceiverN);
    return (jboolean) (testRecN->getNReceivedTelemetryBytes()>0);
}

JNI_METHOD(jstring, getEZWBIPAdress)
(JNIEnv *env,jclass unused,jlong testReceiverN) {
    TelemetryReceiver* testRec=native(testReceiverN);
    jstring ret = env->NewStringUTF("");
    return ret;
}


JNI_METHOD(void, setDecodingInfo)
(JNIEnv *env,jclass unused,jlong nativeInstance,
        jfloat currentFPS, jfloat currentKiloBitsPerSecond,jfloat avgParsingTime_ms,jfloat avgWaitForInputBTime_ms,jfloat avgDecodingTime_ms) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->setDecodingInfo((float)currentFPS,(float)currentKiloBitsPerSecond,(float)avgParsingTime_ms,(float)avgWaitForInputBTime_ms,(float)avgDecodingTime_ms);
}

JNI_METHOD(void, setHomeLocation)
(JNIEnv *env,jclass unused,jlong nativeInstance,
 jdouble latitude, jdouble longitude, jdouble attitude) {
    TelemetryReceiver* instance=native(nativeInstance);
    instance->setHome((double)latitude,(double)longitude,(double)attitude);


}


}






