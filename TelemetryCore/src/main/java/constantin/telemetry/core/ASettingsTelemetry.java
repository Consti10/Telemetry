package constantin.telemetry.core;

import androidx.appcompat.app.AppCompatActivity;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;

import com.mapzen.prefsplus.IntListPreference;

public class ASettingsTelemetry extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new MSettingsFragment())
                .commit();
    }

    public static class MSettingsFragment extends PreferenceFragment implements SharedPreferences.OnSharedPreferenceChangeListener{

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            PreferenceManager preferenceManager=getPreferenceManager();
            preferenceManager.setSharedPreferencesName("pref_telemetry");
            addPreferencesFromResource(R.xml.pref_telemetry);
        }

        @Override
        public void onActivityCreated(Bundle savedInstanceState){
            super.onActivityCreated(savedInstanceState);
            IntListPreference intListPref = (IntListPreference) findPreference(this.getString(R.string.T_Protocol));
            intListPref.setEntries(getResources().getStringArray(R.array.entriesTelemetryProtocol));
        }

        @Override
        public void onResume(){
            super.onResume();
            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
            enableOrDisablePreferences_TelemetryProtocol(getPreferenceScreen().getSharedPreferences());
        }

        @Override
        public void onPause(){
            super.onPause();
            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if(key.contentEquals(getActivity().getString(R.string.T_Protocol))){
                enableOrDisablePreferences_TelemetryProtocol(sharedPreferences);
            }
        }

        private void enableOrDisablePreferences_TelemetryProtocol(SharedPreferences sharedPreferences){
            int val= sharedPreferences.getInt(getActivity().getString(R.string.T_Protocol),0);
            Preference LTMPort=findPreference(getActivity().getString(R.string.T_LTMPort));
            Preference MAVLINKPort=findPreference(getActivity().getString(R.string.T_MAVLINKPort));
            Preference SMARTPORTPort=findPreference(getActivity().getString(R.string.T_SMARTPORTPort));
            Preference FRSKYPort=findPreference(getActivity().getString(R.string.T_FRSKYPort));
            LTMPort.setEnabled(false);
            MAVLINKPort.setEnabled(false);
            SMARTPORTPort.setEnabled(false);
            FRSKYPort.setEnabled(false);
            switch (val){
                case 0:
                    break;
                case 1:LTMPort.setEnabled(true);
                    break;
                case 2:MAVLINKPort.setEnabled(true);
                    break;
                case 3:SMARTPORTPort.setEnabled(true);
                    break;
                case 4:
                    FRSKYPort.setEnabled(true);
                    break;
                case 5:
                    MAVLINKPort.setEnabled(true);
                    break;
                default:
                    break;
            }
        }
    }
}
