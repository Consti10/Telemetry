package constantin.telemetry.core;


import android.content.Context;
import android.location.Location;

import constantin.telemetry.core.HomeLocation;

/*
 * Pattern native -> ndk:
 * createInstance() creates a new native instance and return the pointer to it
 * This pointer is hold by java
 * All other native functions take a pointer to a native instance
 */

/**
 * Optionally: Also handles Sending telemetry data
 */

public class TelemetryReceiver implements HomeLocation.IHomeLocationChanged {
    static {
        System.loadLibrary("TelemetryReceiver");
    }
    private static native long createInstance(Context context);
    private static native void deleteInstance(long instance);
    private static native void startReceiving(long instance);
    private static native void stopReceiving(long instance);
    //set values from java
    private static native void setDecodingInfo(long instance,float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,
                                               float avgWaitForInputBTime_ms,float avgDecodingTime_ms);
    private static native void setHomeLocation(long instance,double latitude,double longitude,double attitude);
    //For debugging/testing
    private static native boolean anyTelemetryDataReceived(long testRecN);
    private static native String getTelemetryInfoString(long testRecN);
    private static native String getEZWBInfoString(long testRecN);
    private static native String getTelemetryDataAsString(long testRecN);
    private static native boolean isEZWBIpAvailable(long testRecN);
    private static native String getEZWBIPAdress(long testRecN);
    private static native boolean receivingEZWBButCannotParse(long testRecN);

    private final long nativeInstance;
    private final HomeLocation mHomeLocation;


    public TelemetryReceiver(final Context context){
        nativeInstance=createInstance(context);
        mHomeLocation=new HomeLocation(context);
    }

    public void delete(){
        deleteInstance(nativeInstance);
    }

    public void startReceiving(){
        startReceiving(nativeInstance);
        mHomeLocation.resume(this);
    }

    public void stopReceiving(){
        mHomeLocation.pause();
        stopReceiving(nativeInstance);
    }

    public final long getNativeInstance(){
        return nativeInstance;
    }

    public boolean anyTelemetryDataReceived(){
        return anyTelemetryDataReceived(nativeInstance);
    }
    public String getTelemetryInfoString(){
        return getTelemetryInfoString(nativeInstance);
    }
    public String getEZWBInfoString(){
        return getEZWBInfoString(nativeInstance);
    }
    public boolean isEZWBIpAvailable(){
        return isEZWBIpAvailable(nativeInstance);
    }
    public String getEZWBIPAdress(){
        return getEZWBIPAdress(nativeInstance);
    }
    public String getTelemetryDataAsString(){
        return getTelemetryDataAsString(nativeInstance);
    }
    public boolean receivingEZWBButCannotParse(){
        return receivingEZWBButCannotParse(nativeInstance);
    }

    public void setDecodingInfo(float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,float avgWaitForInputBTime_ms,float avgDecodingTime_ms) {
        setDecodingInfo(nativeInstance,currentFPS,currentKiloBitsPerSecond,avgParsingTime_ms,avgWaitForInputBTime_ms,avgDecodingTime_ms);
    }

    @Override
    public void onHomeLocationChanged(Location location) {
        setHomeLocation(nativeInstance,location.getLatitude(),location.getLongitude(),location.getAltitude());
    }



}
