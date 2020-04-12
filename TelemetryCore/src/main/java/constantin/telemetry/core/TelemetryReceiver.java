package constantin.telemetry.core;


import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.location.Location;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.widget.Toast;

import androidx.activity.ComponentActivity;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.OnLifecycleEvent;

import java.io.File;

import dji.common.battery.BatteryState;
import dji.common.error.DJIError;
import dji.common.flightcontroller.CompassState;
import dji.common.flightcontroller.FlightControllerState;
import dji.common.flightcontroller.LocationCoordinate3D;
import dji.common.gimbal.GimbalMode;
import dji.common.model.LocationCoordinate2D;
import dji.common.product.Model;
import dji.common.util.CommonCallbacks;
import dji.flysafe.LocationCoordinate;
import dji.sdk.base.BaseProduct;
import dji.sdk.camera.VideoFeeder;
import dji.sdk.products.Aircraft;
import dji.sdk.sdkmanager.DJISDKManager;

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
    private static native long createInstance(Context context,String groundRecordingDirectory,long externalGroundRecorder);
    private static native void deleteInstance(long instance);
    private static native void startReceiving(long instance,Context context,AssetManager assetManager);
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
    //new
    private static native void setDJIValues(long instance,double Latitude_dDeg,double Longitude_dDeg,float AltitudeX_m,float Roll_Deg,float Pitch_Deg,
                                            float SpeedClimb_KPH,float SpeedGround_KPH,int SatsInUse);
    private static native void setDJIBatteryValues(long instance,float BatteryPack_P);

    private final long nativeInstance;
    private final Context context;


    //Only use with AppCombatActivity for lifecycle listener
    //receives data in between onPause()<-->onResume()
    public <T extends Activity & LifecycleOwner> TelemetryReceiver(final T parent,long externalGroundRecorder){
        context=parent;
        nativeInstance=createInstance(parent,TelemetrySettings.getDirectoryToSaveDataTo(),externalGroundRecorder);
        //Home location handles lifecycle itself
        final HomeLocation mHomeLocation = new HomeLocation(parent, this);
        parent.getLifecycle().addObserver(this);
        /////
        final BaseProduct product = DJISDKManager.getInstance().getProduct();
        final Aircraft aircraft=(Aircraft)product;

        if (product == null || !product.isConnected() || aircraft==null) {
            Toast.makeText(context,"Cannot start dji telemetry",Toast.LENGTH_LONG).show();
        } else {
            Toast.makeText(context,"starting dji telemetry",Toast.LENGTH_LONG).show();
            product.getGimbal().setMode(GimbalMode.FPV, null);
            product.getBattery().setStateCallback(new BatteryState.Callback() {
                @Override
                public void onUpdate(BatteryState state) {
                    setDJIBatteryValues(nativeInstance,state.getChargeRemainingInPercent());
                }
            });
            aircraft.getFlightController().setStateCallback(new FlightControllerState.Callback() {
                @Override
                public void onUpdate(FlightControllerState state) {
                    final LocationCoordinate2D home=state.getHomeLocation();
                    final LocationCoordinate3D aircraftLocation=state.getAircraftLocation();
                    setDJIValues(nativeInstance,aircraftLocation.getLatitude(),aircraftLocation.getLongitude(),aircraftLocation.getAltitude(),
                            (float)state.getAttitude().roll,(float)state.getAttitude().pitch,state.getVelocityX(),state.getVelocityZ(),state.getSatelliteCount());
                    setHomeLocation(nativeInstance,home.getLatitude(),home.getLongitude(),0);
                }
            });
        }
    }
    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    private void startReceiving(){
        startReceiving(nativeInstance,context,context.getAssets());
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    private void stopReceiving(){
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

    @Override
    protected void finalize() throws Throwable {
        try {
            deleteInstance(nativeInstance);
        } finally {
            super.finalize();
        }
    }



}
