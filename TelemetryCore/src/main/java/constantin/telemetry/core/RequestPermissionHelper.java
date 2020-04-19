package constantin.telemetry.core;

import android.Manifest;
import android.content.pm.PackageManager;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// if any of the declared permissions are not granted, when calling checkAndRequestPermissions() they are requested.
// When also forwarding onRequestPermissionsResult() they are requested again until granted
public class RequestPermissionHelper implements ActivityCompat.OnRequestPermissionsResultCallback{
    private static final String TAG="RequestPermissionHelper";
    private final String[] REQUIRED_PERMISSION_LIST;
    private final List<String> missingPermission = new ArrayList<>();
    private static final int REQUEST_PERMISSION_CODE = 12345;
    private AppCompatActivity activity;
    private int nRequests=0;

    public RequestPermissionHelper(final String[] requiredPermissionsList){
        REQUIRED_PERMISSION_LIST=requiredPermissionsList;
    }

    public void checkAndRequestPermissions(AppCompatActivity activity){
        this.activity=activity;
        missingPermission.clear();
        for (final String permission : REQUIRED_PERMISSION_LIST) {
            if (ContextCompat.checkSelfPermission(activity,permission) != PackageManager.PERMISSION_GRANTED) {
                if(ActivityCompat.shouldShowRequestPermissionRationale(activity,permission)){
                    Toast.makeText(activity,"App won't work without required permissions",Toast.LENGTH_LONG).show();
                }
                missingPermission.add(permission);
            }
        }
        if (!missingPermission.isEmpty()) {
            nRequests++;
            //if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            final String[] asArray=missingPermission.toArray(new String[0]);
            Log.d(TAG,"Request: "+ Arrays.toString(asArray));
            if(nRequests>2){
                Toast.makeText(activity,"ERROR permissions",Toast.LENGTH_LONG).show();
                throw new RuntimeException("Permissions not granted");
            }
            ActivityCompat.requestPermissions(activity, asArray, REQUEST_PERMISSION_CODE);
            //}
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        // Check for granted permission and remove from missing list
        Log.d(TAG,"onRequestPermissionsResult");
        if (requestCode == REQUEST_PERMISSION_CODE) {
            for (int i = grantResults.length - 1; i >= 0; i--) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    missingPermission.remove(permissions[i]);
                }
            }
        }
        if (!missingPermission.isEmpty()) {
            checkAndRequestPermissions(activity);
        }
    }
}
