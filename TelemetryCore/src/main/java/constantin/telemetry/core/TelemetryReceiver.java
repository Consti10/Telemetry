package constantin.telemetry.core;


import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.AssetManager;
import android.location.Location;
import android.os.Environment;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;

/*
 * Pattern native -> ndk:
 * createInstance() creates a new native instance and return the pointer to it
 * This pointer is hold by java
 * All other native functions take a pointer to a native instance
 */

/**
 * Optionally: Also handles Sending telemetry data
 */

@SuppressWarnings("WeakerAccess")
public class TelemetryReceiver implements HomeLocation.IHomeLocationChanged {
    static {
        System.loadLibrary("TelemetryReceiver");
    }
    private static native long createInstance(Context context);
    private static native void deleteInstance(long instance);
    private static native void startReceiving(long instance, String groundRecordingDirectory, AssetManager assetManager);
    private static native void stopReceiving(long instance);
    //set values from java
    private static native void setDecodingInfo(long instance,float currentFPS, float currentKiloBitsPerSecond,float avgParsingTime_ms,
                                               float avgWaitForInputBTime_ms,float avgDecodingTime_ms);
    private static native void setHomeLocation(long instance,double latitude,double longitude,double attitude);
    //For debugging/testing
    private static native String getStatisticsAsString(long testRecN);
    private static native String getEZWBDataAsString(long testRecN);
    private static native String getTelemetryDataAsString(long testRecN);
    private static native boolean anyTelemetryDataReceived(long testRecN);
    private static native boolean isEZWBIpAvailable(long testRecN);
    private static native String getEZWBIPAdress(long testRecN);
    private static native boolean receivingEZWBButCannotParse(long testRecN);

    private final long nativeInstance;
    private final Context context;
    private final HomeLocation mHomeLocation;

    public TelemetryReceiver(final Context context){
        this.context=context;
        nativeInstance=createInstance(context);
        mHomeLocation=new HomeLocation(context);
    }

    public void delete(){
        deleteInstance(nativeInstance);
    }

    public void startReceiving(){
        startReceiving(nativeInstance,getDirectoryToSaveDataTo(),context.getAssets());
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
    public String getStatisticsAsString(){
        return getStatisticsAsString(nativeInstance);
    }
    public String getEZWBDataAsString(){
        return getEZWBDataAsString(nativeInstance);
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

    private static String createGroundRecordingFilename(){
        @SuppressLint("SimpleDateFormat")
        final String currentDateandTime = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date());
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/TELEMETRY/"+"RECORDING_"+currentDateandTime+".telemetry";
    }

    //also create directory if not already existing
    private static String getDirectoryToSaveDataTo(){
        final String ret= Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/Telemetry/";
        File dir = new File(ret);
        if (!dir.exists()) {
            final boolean mkdirs = dir.mkdirs();
            //System.out.println("mkdirs res"+mkdirs);
        }
        return ret;
    }

}
