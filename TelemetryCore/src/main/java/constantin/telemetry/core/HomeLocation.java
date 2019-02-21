package constantin.telemetry.core;


import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.os.Looper;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;

import androidx.annotation.NonNull;

import static android.content.Context.MODE_PRIVATE;

/**
 * After resume(), onHomeLocationChanged() is called successive until ether pause() or a
 * accuracy of SUFFICIENT_ACCURACY_M is achieved
 */

public class HomeLocation {
    private final FusedLocationProviderClient mFusedLocationClient;
    private final LocationCallback mLocationCallback;
    private IHomeLocationChanged mIHomeLocationChanged;
    private final int SUFFICIENT_ACCURACY_M=10;
    private final boolean OSD_ORIGIN_POSITION_ANDROID;

    public HomeLocation(final Context context){
        mFusedLocationClient = LocationServices.getFusedLocationProviderClient(context);
        mLocationCallback = new LocationCallback() {
            @Override
            public void onLocationResult(LocationResult locationResult) {
                super.onLocationResult(locationResult);
                final Location mCurrentLocation = locationResult.getLastLocation();
                newLocation(mCurrentLocation);
                if(mCurrentLocation.hasAccuracy()&&mCurrentLocation.getAccuracy()<SUFFICIENT_ACCURACY_M){
                    mFusedLocationClient.removeLocationUpdates(mLocationCallback);
                }
            }
        };
        final SharedPreferences pref_telemetry = context.getSharedPreferences("pref_telemetry",MODE_PRIVATE);
        OSD_ORIGIN_POSITION_ANDROID=pref_telemetry.getBoolean(context.getString(R.string.T_ORIGIN_POSITION_ANDROID),false);
    }

    @SuppressLint("MissingPermission")
    public synchronized void resume(IHomeLocationChanged IHomeLocationChanged){
        if(!OSD_ORIGIN_POSITION_ANDROID)return;
        mIHomeLocationChanged = IHomeLocationChanged;
        LocationRequest mLocationRequest = new LocationRequest();
        mLocationRequest.setInterval(500); // in ms
        mLocationRequest.setFastestInterval(500);
        mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
        mFusedLocationClient.requestLocationUpdates(mLocationRequest, mLocationCallback, Looper.myLooper());
    }

    public synchronized void pause(){
        mFusedLocationClient.removeLocationUpdates(mLocationCallback);
        mIHomeLocationChanged =null;
    }

    private synchronized void newLocation(@NonNull final Location location){
        printLocation(location);
        if(mIHomeLocationChanged !=null){
            mIHomeLocationChanged.onHomeLocationChanged(location);
        }
    }

    private void printLocation(Location mCurrentHomeLocation){
        System.out.println("Lat:"+mCurrentHomeLocation.getLatitude()+" Lon:"+mCurrentHomeLocation.getLongitude()+" Alt:"
                +mCurrentHomeLocation.getAltitude()+" Accuracy:"+mCurrentHomeLocation.getAccuracy()+" Provider:"
                +mCurrentHomeLocation.getProvider());
    }

    public interface IHomeLocationChanged {
        void onHomeLocationChanged(Location location);
    }
}
