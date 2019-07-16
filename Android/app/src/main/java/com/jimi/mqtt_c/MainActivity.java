package com.jimi.mqtt_c;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;

import com.jimi.mqtt.MQTT;

public class MainActivity extends AppCompatActivity implements MQTT.IotCallBack {
    MQTT.IotContext ctx;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ctx = new MQTT.IotContext(this);
        ctx.send_connect("a","b","c");
        ctx.send_bool(1,true);
        ctx.send_double(2,3.14);
        ctx.send_enum(3,"enum");
        ctx.send_string(4,"str");
        ctx.timer_schedule();

    }

    @Override
    public void iot_on_output(byte[] data) {
        ctx.input_data(data);
    }

    @Override
    public void iot_on_connect(int ret_code) {
        Log.d("mqtt","iot_on_connect:" + ret_code);
    }

    @Override
    public void iot_on_message(int req_flag, int req_id, int tag_id, int data_type, Object data) {
        Log.d("mqtt","iot_on_message:" + req_flag + " " + req_id + " " + tag_id + " " + data + " " + data);
    }
}
