package constantin.telemetry.example;

import androidx.appcompat.app.AppCompatActivity;
import constantin.telemetry.core.ASettingsTelemetry;
import constantin.telemetry.core.TestReceiverTelemetry;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private TestReceiverTelemetry testReceiverTelemetry;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Button button=findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent().setClass(getApplicationContext(), ASettingsTelemetry.class));
            }
        });
        TextView tv1 = findViewById(R.id.textView);
        TextView tv2 = findViewById(R.id.textView2);
        TextView tv3 = findViewById(R.id.textView3);
        testReceiverTelemetry=new TestReceiverTelemetry(this, tv1, tv2, tv3);
    }

    @Override
    protected void onResume(){
        super.onResume();
        testReceiverTelemetry.startReceiving();
    }

    @Override
    protected void onPause(){
        super.onPause();
        testReceiverTelemetry.stopReceiving();
    }
}
