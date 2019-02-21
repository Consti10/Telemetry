cmake_minimum_required(VERSION 3.6)

find_library( log-lib
              log )
include_directories(libs/other)

##########################################################################################################
#TelemetryReceiver
##########################################################################################################
####################
#C-Files
####################
add_library( ltm_frsky_mavlink_smartport
        SHARED
        src/main/cpp/Telemetry/cFiles/ltm.c
        src/main/cpp/Telemetry/cFiles/frsky.c
        src/main/cpp/Telemetry/cFiles/mavlink2.c
        src/main/cpp/Telemetry/cFiles/smartport.c
        )
target_link_libraries(ltm_frsky_mavlink_smartport
        ${log-lib}
        android
        log)
#######################################################
add_library( TelemetryReceiver
        SHARED
        src/main/cpp/Telemetry/UDPReceiver.cpp
        src/main/cpp/Telemetry/TelemetryReceiver.cpp )
target_link_libraries(TelemetryReceiver
        ${log-lib}
        android
        log
        ltm_frsky_mavlink_smartport)


set_target_properties(TelemetryReceiver
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "../../../../jniLibs/${ANDROID_ABI}")


