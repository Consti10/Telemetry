//
// Created by Constantin on 10.10.2017.
//

#ifndef OSDTESTER_TELEMETRYRECEIVER_H
#define OSDTESTER_TELEMETRYRECEIVER_H

#include "UDPReceiver.h"
#include "cFiles/shared_c_objective.h"
#include <atomic>
#include <fstream>
#include <iostream>
#include "Helper/SettingsN.hpp"
#include "Helper/FileReader.hpp"
#include "GroundRecorder.h"

//#define RECEIVE_FROM_TESTLOG

/*
 * This data is only generated on the android side and does not depend
 * on the uav telemetry stream
 */
typedef struct {
    float decoder_fps=0;
    float decoder_bitrate_kbits=0;
    float opengl_fps=0;
    float flight_time_seconds=0;
    float avgParsingTime_ms=0;
    float avgWaitForInputBTime_ms=0;
    float avgDecodingTime_ms=0;
} AppOSDData;


class TelemetryReceiver{
public:
    TelemetryReceiver(const TelemetryReceiver&) = delete;
    void operator=(const TelemetryReceiver&) = delete;
private:
    //these are called via lambda by the UDP receiver(s)
    void onUAVTelemetryDataReceived(uint8_t[],int);
    void onEZWBStatusDataReceived(uint8_t[], int);
public:
    static constexpr int T_PROTOCOL_NONE=0;
    static constexpr int T_PROTOCOL_LTM=1;
    static constexpr int T_PROTOCOL_MAVLINK=2;
    static constexpr int T_PROTOCOL_SMARTPORT=3;
    static constexpr int T_PROTOCOL_FRSKY=4;
    static constexpr int T_PROTOCOL_MAVLINK2=5;
    const int T_Protocol;
    const int T_Port;
    static int getTelemetryPort(const SettingsN& settingsN, int T_Protocol);

    //ez-wb status settings
    static constexpr int EZWB_PROTOCOL_DISBALE=0;
    static constexpr int EZWB_PROTOCOL_16_rc6=1;
    const int EZWBS_Protocol;
    const int EZWBS_Port;
    const bool MAVLINK_FLIGHTMODE_QUADCOPTER;
    const bool ORIGIN_POSITION_ANDROID;
    const bool ENABLE_GROUND_RECORDING;
    //
    const int BATT_CAPACITY_MAH;
    const int BATT_CELLS_N;
    const float BATT_CELLS_V_WARNING1_ORANGE;
    const float BATT_CELLS_V_WARNING2_RED;
    const float BATT_CAPACITY_MAH_USED_WARNING;
    enum SOURCE_TYPE_OPTIONS { UDP,FILE,ASSETS };
    const SOURCE_TYPE_OPTIONS SOURCE_TYPE;
public:
    explicit TelemetryReceiver(const SettingsN& settingsN);
    /**
     * Start all telemetry receiver. If they are already receiving, nothing happens.
     * Make sure startReceiving() and stopReceivingAndWait() are not called on different threads
     * Also make sure to call stopReading() every time startReading() is called
     */
    void startReceiving(AAssetManager* assetManager,const char* groundRecordingDirectory);
    /**
     * Stop all telemetry receiver if they are currently running
     * Make sure startReading() and stopReading() are not called on different threads
     */
    void stopReceiving();
    //
    void setDecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms);
    void setOpenGLFPS(float fps);
    void setFlightTime(float timeSeconds);
    void setHome(double latitude,double longitude,double attitude);
    void resetNReceivedTelemetryBytes();
    //
    const std::string getStatisticsAsString()const;
    const std::string getProtocolAsString()const;
    const std::string getAllTelemetryValuesAsString()const;
    const std::string getEZWBDataAsString()const;
    const int getNReceivedTelemetryBytes()const;
    const long getNEZWBPacketsParsingFailed()const;
    const int getBestDbm()const;
    const UAVTelemetryData* getUAVTelemetryData()const;
    const float getCourseOG_Deg()const;
    const float getHeading_Deg()const;
    const wifibroadcast_rx_status_forward_t* get_ez_wb_forward_data()const;
    const OriginData* getOriginData()const;
    float getHeadingHome_Deg()const;

    class MTelemetryValue{
    public:
        std::wstring prefix=L"";
        bool prefixIsIcon=false;
        float prefixScale=0.83f;
        std::wstring value=L"";
        std::wstring metric=L"";
        int warning=0; //0==okay 1==orange 2==red and -1==green
        unsigned long getLength(){
            return prefix.length()+value.length()+metric.length();
        }
    };
    enum TelemetryValueIndex{
        DECODER_FPS,
        DECODER_BITRATE,
        DECODER_LATENCY_DETAILED,
        DECODER_LATENCY_SUM,
        OPENGL_FPS,
        FLIGHT_TIME,
        //
        RX_1,
        BATT_VOLTAGE,
        BATT_CURRENT,
        BATT_USED_CAPACITY,
        BATT_PERCENTAGE,
        HOME_DISTANCE,
        VS,
        HS_GROUND,
        HS_AIR,
        LONGITUDE,
        LATITUDE,
        ALTITUDE_GPS,
        ALTITUDE_BARO,
        SATS_IN_USE,
        FLIGHT_STATUS_MAV_ONLY,
        //
        EZWB_DOWNLINK_VIDEO_RSSI,
        EZWB_DOWNLINK_VIDEO_RSSI2,
        EZWB_UPLINK_RC_RSSI,
        EZWB_UPLINK_RC_BLOCKS,
        EZWB_STATUS_AIR,
        EZWB_STATUS_GROUND,
        EZWB_BLOCKS,
        EZWB_ADAPTER1_RSSI,
        EZWB_ADAPTER2_RSSI,
        EZWB_ADAPTER3_RSSI,
        EZWB_ADAPTER4_RSSI,
        EZWB_ADAPTER5_RSSI,
        EZWB_ADAPTER6_RSSI,
        XXX
    };
    const MTelemetryValue getTelemetryValue(TelemetryValueIndex index)const;
    const MTelemetryValue getTelemetryValueEZWB_RSSI_ADAPTERS_1to6(int adapter)const;

    const std::wstring getMAVLINKFlightMode()const;
private:
    UDPReceiver* mTelemetryDataReceiver= nullptr;
    UDPReceiver* mEZWBDataReceiver= nullptr;
    FileReader* mTestFileReader= nullptr;
    GroundRecorder* mGroundRecorder=nullptr;

    long nTelemetryBytes=0;
    long nWIFIBRADCASTBytes=0;
    long nWIFIBROADCASTParsedPackets=0;
    long nWIFIBRADCASTFailedPackets=0;
    OriginData originData;
    AppOSDData appOSDData;
    UAVTelemetryData uav_td;
    wifibroadcast_rx_status_forward_t wifibroadcast_rx_status_forward;
};


#endif //OSDTESTER_TELEMETRYRECEIVER_H
