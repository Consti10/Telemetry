
#ifndef FPV_VR_MAVLINK2_H
#define FPV_VR_MAVLINK2_H

#include "mavlink_v2/mavlink.h"
#include "shared_c_objective.h"

void mavlink_read_v2(UAVTelemetryData *td,OriginData *originData,const uint8_t *data, const int data_length,const bool hackV1Messages);

#endif
