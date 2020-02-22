package constantin.telemetry.core;


import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.location.Location;
import android.os.Environment;
import android.preference.PreferenceManager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;

import java.io.File;

import static android.content.Context.MODE_PRIVATE;

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
public class TelemetryReceiver implements HomeLocation.IHomeLocationChanged, LifecycleObserver {

    public static final int SOURCE_TYPE_UDP=0;
    public static final int SOURCE_TYPE_FILE=1;
    public static final int SOURCE_TYPE_ASSETS=2;


    static {
        System.loadLibrary("TelemetryReceiver");
    }
    private static native long createInstance(Context context,String groundRecordingDirectory);
    private static native void deleteInstance(long instance);
    private static native void startReceiving(long instance,AssetManager assetManager);
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

    //settings are initialized when creating the native instance !

    //Only use with AppCombatActivity for lifecycle listener
    //receives data in between onPause()<-->onResume()
    public TelemetryReceiver(final AppCompatActivity activity){
        activity.getLifecycle().addObserver(this);
        this.context=activity.getApplicationContext();
        nativeInstance=createInstance(context,getDirectoryToSaveDataTo());
        mHomeLocation=new HomeLocation(activity,this);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    private void startReceiving(){
        startReceiving(nativeInstance,context.getAssets());
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    private void stopReceiving(){
        stopReceiving(nativeInstance);
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    private void deleteNativeInstance(){
        deleteInstance(nativeInstance);
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


    //also create directory if not already existing
    private static String getDirectoryToSaveDataTo(){
        final String ret= getDirectory()+"Telemetry/";
        File dir = new File(ret);
        if (!dir.exists()) {
            final boolean mkdirs = dir.mkdirs();
            //System.out.println("mkdirs res"+mkdirs);
        }
        return ret;
    }

    private static String getDirectory(){
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/";
    }

    @SuppressLint("ApplySharedPref")
    public static void initializePreferences(final Context context,final boolean readAgain){
        PreferenceManager.setDefaultValues(context,"pref_telemetry",MODE_PRIVATE,R.xml.pref_telemetry,readAgain);
        final SharedPreferences pref_telemetry=context.getSharedPreferences("pref_telemetry", MODE_PRIVATE);
        final String filename=pref_telemetry.getString(context.getString(R.string.T_PLAYBACK_FILENAME),context.getString(R.string.T_PLAYBACK_FILENAME_DEFAULT_VALUE));
        if(filename.equals(context.getString(R.string.T_PLAYBACK_FILENAME_DEFAULT_VALUE))){
            pref_telemetry.edit().putString(context.getString(R.string.T_PLAYBACK_FILENAME),
                    getDirectory()+"Telemetry/"+"filename.format").commit();
        }
    }

    public static boolean PLAYBACK_FILE_EXISTS(final String filename, final SharedPreferences sharedPreferences, final Context context){
        //check if the file exists
        final String pathAndFile= Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/Telemetry/"+filename;
        System.out.println(pathAndFile);
        File tempFile = new File(filename);
        final boolean exists = tempFile.exists();
        if(exists){
            //check if the file type matches the selected telemetry protocol
            return getFileExtension(filename).equals(getNameForProtocol(sharedPreferences.getInt(context.getString(R.string.T_PROTOCOL),0)));
        }
        return false;
    }

    //return true if file name extension does not match the selected telemetry protocol
    public static boolean checkFileExtensionMatchTelemetryProtocol(final Context context){
        final SharedPreferences pref_telemetry=context.getSharedPreferences("pref_telemetry",MODE_PRIVATE);
        final String filename=pref_telemetry.getString(context.getString(R.string.T_PLAYBACK_FILENAME),context.getString(R.string.T_PLAYBACK_FILENAME_DEFAULT_VALUE));
        final String extension=getFileExtension(filename);
        final String protocol=getNameForProtocol(pref_telemetry.getInt(context.getString(R.string.T_PROTOCOL),0));
        System.out.println("LA"+filename+" "+extension+" "+protocol);
        return extension.equals(protocol);
    }

    //int value=sharedPreferences.getInt(getActivity().getString(R.string.T_Protocol),0);
    public static String getNameForProtocol(int protocol){
        switch (protocol){
            case 0:return "none";
            case 1:return "ltm";
            case 2:return "mavlink";
            case 3:return "smartport";
            case 4:return "frsky";
            default:return null;
        }
    }

    private static String getFileExtension(String name) {
        int lastIndexOf = name.lastIndexOf(".");
        if (lastIndexOf == -1 || (lastIndexOf+1>=name.length())) {
            return ""; // empty extension
        }
        return name.substring(lastIndexOf+1);
    }



}
