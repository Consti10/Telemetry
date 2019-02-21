package constantin.telemetry.core;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.widget.TextView;
import android.widget.Toast;

import constantin.telemetry.core.TelemetryReceiver;

public class TestReceiverTelemetry extends Thread{

    private final TextView receivedTelemetryDataTV;
    private final TextView ezwbForwardDataTV;
    private final TextView dataAsStringTV;
    private final Context context;
    private TelemetryReceiver telemetryReceiver;


    public TestReceiverTelemetry(Context c, TextView receivedTelemetryDataTV, TextView ezwbForwardDataTV,TextView dataAsStringTV){
        this.context=c;
        this.receivedTelemetryDataTV=receivedTelemetryDataTV;
        this.ezwbForwardDataTV=ezwbForwardDataTV;
        this.dataAsStringTV=dataAsStringTV;
    }

    public void startReceiving(){
        telemetryReceiver =new TelemetryReceiver(context);
        telemetryReceiver.startReceiving();
        this.start();
    }

    public void stopReceiving(){
        this.interrupt();
        try {this.join();} catch (InterruptedException e) {e.printStackTrace();}
        telemetryReceiver.stopReceiving();
        telemetryReceiver.delete();
    }

    @SuppressLint("ApplySharedPref")
    private void onEZWBIpDetected(String ip){
        //System.out.println("Called from native:"+ip);
        /*if(SJ.EnableAHT(context) && !SJ.UplinkEZWBIpAddress(context).contentEquals(ip)){
            Toaster.makeToast(context,"Head tracking IP address set to:"+ip,true);
            SharedPreferences pref_connect = context.getSharedPreferences("pref_connect", MODE_PRIVATE);
            SharedPreferences.Editor editor=pref_connect.edit();
            editor.putString(context.getString(R.string.UplinkEZWBIpAddress),ip).commit();
        }*/
    }

    //to have as less work on the UI thread and GPU, we check if the content of the string has changed
    //before updating the TV.
    private static void updateViewIfStringChanged(final Context context,final TextView tv, final String newContent,final boolean colorRed,final boolean changeColor){
        final String prev=tv.getText().toString();
        if(!prev.contentEquals(newContent)){
            ((Activity)context).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    tv.setText(newContent);
                    if(changeColor){
                        if(colorRed){
                            tv.setTextColor(Color.RED);
                        }else{
                            tv.setTextColor(Color.GREEN);
                        }
                    }
                }
            });
        }
    }

    @Override
    public void run(){
        setName("TestReceiverTelemetry TV String refresher");
        long lastCheckMS = System.currentTimeMillis() - 2*1000;
        while (!isInterrupted()){
            //if the receivedVideoDataTV is !=null, we should update its content with the
            //number of received bytes usw
            if(receivedTelemetryDataTV !=null){
                final String s= telemetryReceiver.getTelemetryInfoString();
                updateViewIfStringChanged(context,receivedTelemetryDataTV,s,!telemetryReceiver.anyTelemetryDataReceived(),true);
            }
            if(ezwbForwardDataTV!=null){
                final String s= telemetryReceiver.getEZWBInfoString();
                updateViewIfStringChanged(context,ezwbForwardDataTV,s,false,false);
            }
            if(dataAsStringTV!=null){
                final String s = telemetryReceiver.getTelemetryDataAsString();
                dataAsStringTV.setText(s);
            }
            if(telemetryReceiver.isEZWBIpAvailable()){
                onEZWBIpDetected(telemetryReceiver.getEZWBIPAdress());
            }
            //Every 3.5s we check if we are receiving video data, but cannot parse the data. Probably the user did mix up
            //rtp and raw. Make a warning toast
            if(System.currentTimeMillis()- lastCheckMS >=3500){
                final boolean errorEZ_WB= telemetryReceiver.receivingEZWBButCannotParse();
                lastCheckMS =System.currentTimeMillis();
                if(errorEZ_WB){
                    Toast.makeText(context,"You are receiving ez-wifibroadcast telemetry data, but FPV-VR cannot parse it. Probably you are using" +
                            " app version 1.5 or higher with ez-wb. 1.6 or lower. Please upgrade to ez-wb 2.0. This also allows you to read all useful" +
                            " telemetry data from your EZ-WB rx pi on android.",Toast.LENGTH_LONG).show();
                }
            }
            //Refresh every 200ms
            try {sleep(200);} catch (InterruptedException e) {return;}
        }
    }

    public interface EZWBIpAddressDetected{
        void onEZWBIpDetected(String ip);
    }
}
