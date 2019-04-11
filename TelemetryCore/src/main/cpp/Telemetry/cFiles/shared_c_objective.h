//
// Created by Constantin on 12.01.2018.
//

#ifndef FPV_VR_EZ_WIFIBROADCAST_MISC_H
#define FPV_VR_EZ_WIFIBROADCAST_MISC_H

#include <stdio.h>
#include <android/log.h>
#include <stdbool.h>

/***
 * This once can be included in the c ( parser) an cpp (TelemetryReceiver) parts
 */


/*
 * Only set by LTM and MAVLINK, optionally set by android gps
 */
typedef struct {
    //float Altitude_m;
    double Longitude_dDeg;
    double Latitude_dDeg;
    bool hasBeenSet;
    bool writeByTelemetryProtocol; //false if we use the ANDROID GPS for the origin
} OriginData;

/**
 * This struct was once similar to the uav_telemetry_data in EZ-WB OSD, but it evolved a lot over time
 * and now has almost no similarities.
 */
typedef struct {
    uint32_t validmsgsrx;

    float BatteryPack_V;  //Resolution only mVolt
    float BatteryPack_A;  //Resolution only mAmpere.
    float BatteryPack_mAh; //Already used capacity, in mAh
    float BatteryPack_P; //remaining battery, in percentage.Only sent by MAVLINK -
    // else it has to be calculated manually via already used capacity and battery capacity

    float AltitudeGPS_m; // GPS altitude in m. Relative (to origin) if possible
    float AltitudeBaro_m;// barometric altitude in m. Relative (to origin) if possible

    double Longitude_dDeg; //decimal degrees, Longitude of Aircraft
    double Latitude_dDeg; //decimal degrees Latitude of Aircraft

    float Roll_Deg; //Roll of the aircraft, in degrees
    float Pitch_Deg; //Pitch of the aircraft, in degrees

    float Heading_Deg; //Heading of the aircraft, in degrees
    float CourseOG_Deg; //course over ground (often reported by the compass) of the aircraft, in degrees

    float SpeedGround_KPH; // ( km/h) ==VS1
    float SpeedAir_KPH; // ( km/h) ==VS2
    float SpeedClimb_KPH; //(km/h) ==HS

    int SatsInUse; //n of Satelites visible for GPS on aircraft
    float RSSI1_Percentage_dBm; //RSSI of receiver if not using EZ-WB (rarely transmitted by telemetry protocol). When using MAVLINK in %, else in dBm

    int FlightMode_MAVLINK;
    bool FlightMode_MAVLINK_armed;

} UAVTelemetryData;



#define WIFIBROADCAST_RX_STATUS_FORWARD_SIZE_BYTES 94
typedef struct {
    uint32_t received_packet_cnt;
    int8_t current_signal_dbm;
    int8_t type; // 0 = Atheros, 1 = Ralink
} __attribute__((packed)) wifi_adapter_rx_status_forward_t;
typedef struct {
    uint32_t damaged_block_cnt; // number bad blocks video downstream
    uint32_t lost_packet_cnt; // lost packets video downstream
    uint32_t skipped_packet_cnt; // skipped packets video downstream
    uint32_t received_packet_cnt; // packets received video downstream
    uint32_t kbitrate; // live video kilobitrate per second video downstream
    uint32_t kbitrate_measured; // max measured kbitrate during tx startup
    uint32_t kbitrate_set; // set kilobitrate (measured * bitrate_percent) during tx startup
    uint32_t lost_packet_cnt_telemetry_up; // lost packets telemetry uplink
    uint32_t lost_packet_cnt_telemetry_down; // lost packets telemetry downlink
    uint32_t lost_packet_cnt_msp_up; // lost packets msp uplink (not used at the moment)
    uint32_t lost_packet_cnt_msp_down; // lost packets msp downlink (not used at the moment)
    uint32_t lost_packet_cnt_rc; // lost packets rc link
    int8_t current_signal_air; // signal strength in dbm at air pi (telemetry upstream and rc link)
    int8_t joystick_connected; // 0 = no joystick connected, 1 = joystick connected
    uint8_t cpuload_gnd; // CPU load Ground Pi
    uint8_t temp_gnd; // CPU temperature Ground Pi
    uint8_t cpuload_air; // CPU load Air Pi
    uint8_t temp_air; // CPU temperature Air Pi
    uint32_t wifi_adapter_cnt; // number of wifi adapters
    wifi_adapter_rx_status_forward_t adapter[6]; // see struct above
} __attribute__((packed)) wifibroadcast_rx_status_forward_t;


#endif //FPV_VR_EZ_WIFIBROADCAST_MISC_H
