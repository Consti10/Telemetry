//
// Created by Constantin on 10.10.2017.
//

#ifndef OSDTESTER_TELEMETRYRECEIVER_H
#define OSDTESTER_TELEMETRYRECEIVER_H

#include "Network/UDPReceiver.h"
#include "cFiles/UAVTelemetryData.h"
#include "WFBTelemetryData/WFBTelemetryData.h"

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include "Helper/SettingsN.hpp"
#include "Helper/FileReader.hpp"
#include "Helper/GroundRecorderRAW.hpp"

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
    void onUAVTelemetryDataReceived(const uint8_t[],size_t);
    void onEZWBStatusDataReceived(const uint8_t[],size_t);
private:
    enum SOURCE_TYPE_OPTIONS { UDP,FILE,ASSETS };
    enum PROTOCOL_OPTIONS {NONE,XLTM,MAVLINK,XSMARTPORT,FRSKY};
    enum EZWB_STATUS_PROTOCOL{DISABLED,EZWB_16_rc6,OpenHD_1_0_0};
    enum METRIC_SPEED{KMH,MS};
    static int getTelemetryPort(const SettingsN& settingsN, int T_Protocol);
    const std::string GROUND_RECORDING_DIRECTORY;
    //
    SOURCE_TYPE_OPTIONS SOURCE_TYPE;
    PROTOCOL_OPTIONS T_Protocol;
    int T_Port;
    //ez-wb status settings
    EZWB_STATUS_PROTOCOL EZWBS_Protocol;
    int EZWBS_Port;
    std::string T_PLAYBACK_FILENAME;
    bool LTM_FOR_INAV;
    METRIC_SPEED T_METRIC_SPEED_VERTICAL;
    METRIC_SPEED T_METRIC_SPEED_HORIZONTAL;
    void updateSettings(JNIEnv *env,jobject context);
public:
    bool MAVLINK_FLIGHTMODE_QUADCOPTER;
    bool ORIGIN_POSITION_ANDROID;
    bool ENABLE_GROUND_RECORDING;
    //
    int BATT_CAPACITY_MAH;
    int BATT_CELLS_N;
    float BATT_CELLS_V_WARNING1_ORANGE;
    float BATT_CELLS_V_WARNING2_RED;
    float BATT_CAPACITY_MAH_USED_WARNING;
public:
    explicit TelemetryReceiver(const char* DIR);
    /**
     * Start all telemetry receiver. If they are already receiving, nothing happens.
     * Make sure startReceiving() and stopReceivingAndWait() are not called on different threads
     * Also make sure to call stopReceiving() every time startReceiving() is called
     * Change 22.02.2020: Also read all settings from shared preferences (before they were immutable, but this meant you
     * had to re-create the native TelemetryReceiver() every time shared preferences were changed)
     */
    void startReceiving(JNIEnv *env,jobject context,AAssetManager* assetManager);
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
    const std::string getSystemAsString()const;
    const std::string getAllTelemetryValuesAsString()const;
    const std::string getEZWBDataAsString()const;
    const int getNReceivedTelemetryBytes()const;
    const long getNEZWBPacketsParsingFailed()const;
    const int getBestDbm()const;
    const UAVTelemetryData& getUAVTelemetryData()const;
    const wifibroadcast_rx_status_forward_t2& get_ez_wb_forward_data()const;
    const OriginData& getOriginData()const;
    const float getCourseOG_Deg()const;
    const float getHeading_Deg()const;
    float getHeadingHome_Deg()const;

    class MTelemetryValue{
    public:
        std::wstring prefix=std::wstring();
        std::wstring prefixIcon=std::wstring();
        float prefixScale=0.83f;
        std::wstring value=L"";
        std::wstring metric=L"";
        int warning=0; //0==okay 1==orange 2==red and -1==green
        unsigned long getLength()const{
            return prefix.length()+value.length()+metric.length();
        }
        bool hasIcon()const{
            return (!prefixIcon.empty());
        }
        std::wstring getPrefix()const{
            return hasIcon() ? prefixIcon : prefix;
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
        EZWB_RSSI_ADAPTER0,
        EZWB_RSSI_ADAPTER1,
        EZWB_RSSI_ADAPTER2,
        EZWB_RSSI_ADAPTER3,
        //EZWB_RSSI_ADAPTER4,
        //EZWB_RSSI_ADAPTER5,
        XXX
    };
    const MTelemetryValue getTelemetryValue(TelemetryValueIndex index)const;
    const MTelemetryValue getTelemetryValueEZWB_RSSI_ADAPTERS_0to5(int adapter)const;
    const std::wstring getMAVLINKFlightMode()const;
private:
    std::unique_ptr<UDPReceiver> mTelemetryDataReceiver;
    std::unique_ptr<UDPReceiver> mEZWBDataReceiver;
    std::unique_ptr<FileReader> mTestFileReader;
    std::unique_ptr<GroundRecorderRAW> mGroundRecorder;

    long nTelemetryBytes=0;
    long nWIFIBRADCASTBytes=0;
    long nWIFIBROADCASTParsedPackets=0;
    long nWIFIBRADCASTFailedPackets=0;
    OriginData originData;
    AppOSDData appOSDData;
    UAVTelemetryData uav_td;
    wifibroadcast_rx_status_forward_t2 wifibroadcastTelemetryData;
    static constexpr const float KMH_TO_MS=1000.0F/(60.0F*60.0F);
private:
    const std::wstring ICON_BATTERY=std::wstring(1,(wchar_t)192);
    const std::wstring ICON_CHIP=std::wstring(1,(wchar_t)192+1);
    const std::wstring ICON_HOME=std::wstring(1,(wchar_t)192+2);
    const std::wstring ICON_LATITUDE=std::wstring(1,(wchar_t)192+3);
    const std::wstring ICON_LONGITUDE=std::wstring(1,(wchar_t)192+4);
    const std::wstring ICON_SATELITE=std::wstring(1,(wchar_t)192+5);
};


#endif //OSDTESTER_TELEMETRYRECEIVER_H
