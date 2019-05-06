//
// Created by Consti10 on 29/04/2019.
//

#ifndef FPV_VR_PRIVATE_WFBBACKWARDSCOMPATIBILITY_H
#define FPV_VR_PRIVATE_WFBBACKWARDSCOMPATIBILITY_H

#include "WFBTelemetryData.h"

//the newer struct has more values then the old one
//but is mostly backwards compatible
void writeDataBackwardsCompatible(wifibroadcast_rx_status_forward_t2 *n,
                                  const wifibroadcast_rx_status_forward_t *o){
    n->damaged_block_cnt=o->damaged_block_cnt;
    n->lost_packet_cnt=o->lost_packet_cnt;
    n->skipped_packet_cnt=o->skipped_packet_cnt;
    n->received_packet_cnt=o->received_packet_cnt;
    n->kbitrate=o->kbitrate;
    n->kbitrate_measured=o->kbitrate_measured;
    n->kbitrate_set=o->kbitrate_set;
    n->lost_packet_cnt_telemetry_up=o->lost_packet_cnt_telemetry_up;
    n->lost_packet_cnt_telemetry_down=o->lost_packet_cnt_telemetry_down;
    n->lost_packet_cnt_msp_up=o->lost_packet_cnt_msp_up;
    n->lost_packet_cnt_msp_down=o->lost_packet_cnt_msp_down;
    n->lost_packet_cnt_rc=o->lost_packet_cnt_rc;

    n->joystick_connected=o->joystick_connected;
    n->cpuload_gnd=o->cpuload_gnd;
    n->temp_gnd=o->temp_gnd;
    n->cpuload_air=o->cpuload_air;
    n->temp_air=o->temp_air;
    n->wifi_adapter_cnt=o->wifi_adapter_cnt;
    for(int i=0;i<6;i++){
        const wifi_adapter_rx_status_forward_t* o1=&o->adapter[i];
        wifi_adapter_rx_status_forward_t2* n1=&n->adapter[i];
        n1->received_packet_cnt=o1->received_packet_cnt;
        n1->current_signal_dbm=o1->current_signal_dbm;
        n1->type=o1->type;
    }
}

#endif //FPV_VR_PRIVATE_WFBBACKWARDSCOMPATIBILITY_H
