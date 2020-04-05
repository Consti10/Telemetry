##########################################################################################################
# include this to build the native part
##########################################################################################################

find_library( log-lib
              log )

set(TELEMETRY_PATH ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp/Telemetry)

set(IO_PATH ${V_CORE_DIR}/../VideoTelemetryShared/InputOutput)
set(HELPER_PATH ${V_CORE_DIR}/../VideoTelemetryShared/Helper)
include_directories(${HELPER_PATH})
include_directories(${IO_PATH})

####################
#C-Files
####################
add_library( ltm_frsky_mavlink_smartport
        SHARED
        ${TELEMETRY_PATH}/cFiles/ltm.c
        ${TELEMETRY_PATH}/cFiles/frsky.c
        ${TELEMETRY_PATH}/cFiles/mavlink2.c
        ${TELEMETRY_PATH}/cFiles/smartport.c
        )
target_link_libraries(ltm_frsky_mavlink_smartport
        ${log-lib}
        android
        log)
#######################################################
add_library( TelemetryReceiver
        SHARED
        ${TELEMETRY_PATH}/TelemetryReceiver.cpp
        ${IO_PATH}/FileReader.cpp
        ${IO_PATH}/UDPReceiver.cpp
        )
target_link_libraries(TelemetryReceiver
        ${log-lib}
        android
        log
        mediandk
        ltm_frsky_mavlink_smartport)

include_directories( ${TELEMETRY_PATH})

