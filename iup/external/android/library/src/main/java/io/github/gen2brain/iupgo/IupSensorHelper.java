package io.github.gen2brain.iupgo;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.Keep;

import java.util.HashMap;
import java.util.Map;

public final class IupSensorHelper
{
    private static final int ACCELEROMETER = 0;
    private static final int GYROSCOPE = 1;
    private static final int MAGNETOMETER = 2;
    private static final int GRAVITY = 3;
    private static final int LINEARACCELERATION = 4;
    private static final int ORIENTATION = 5;
    private static final int COMPASS = 6;

    private IupSensorHelper() {}

    private static final Map<Long, SensorEventListener> sListeners = new HashMap<>();

    private static SensorManager manager()
    {
        Context ctx = IupCommon.getContextThemeWrapper();
        return ctx == null ? null : (SensorManager)ctx.getSystemService(Context.SENSOR_SERVICE);
    }

    private static int androidType(int type)
    {
        switch (type)
        {
            case GYROSCOPE: return Sensor.TYPE_GYROSCOPE;
            case MAGNETOMETER: return Sensor.TYPE_MAGNETIC_FIELD;
            case GRAVITY: return Sensor.TYPE_GRAVITY;
            case LINEARACCELERATION: return Sensor.TYPE_LINEAR_ACCELERATION;
            case ORIENTATION:
            case COMPASS: return Sensor.TYPE_ROTATION_VECTOR;
            default: return Sensor.TYPE_ACCELEROMETER;
        }
    }

    @Keep
    public static boolean isAvailable(int type)
    {
        SensorManager sm = manager();
        return sm != null && sm.getDefaultSensor(androidType(type)) != null;
    }

    private static final class Listener implements SensorEventListener
    {
        private final long ih;
        private final int type;
        private final long intervalMs;
        private double headingSin, headingCos;
        private long lastMs;

        Listener(long ih, int type, long intervalMs) { this.ih = ih; this.type = type; this.intervalMs = intervalMs; }

        @Override public void onAccuracyChanged(Sensor s, int accuracy) {}

        @Override public void onSensorChanged(SensorEvent event)
        {
            long timeMs = System.currentTimeMillis() - (System.nanoTime() - event.timestamp) / 1000000L;
            if (type == ORIENTATION || type == COMPASS)
            {
                float[] rotation = new float[9];
                float[] angles = new float[3];
                SensorManager.getRotationMatrixFromVector(rotation, event.values);
                SensorManager.getOrientation(rotation, angles);
                double azimuth = Math.toDegrees(angles[0]);
                if (azimuth < 0) azimuth += 360;
                if (type == ORIENTATION)
                {
                    dispatchReading(ih, azimuth, Math.toDegrees(angles[1]), Math.toDegrees(angles[2]), timeMs);
                    return;
                }
                headingSin += (Math.sin(angles[0]) - headingSin) * 0.05;
                headingCos += (Math.cos(angles[0]) - headingCos) * 0.05;
                if (timeMs - lastMs < intervalMs) return;
                lastMs = timeMs;
                double heading = Math.toDegrees(Math.atan2(headingSin, headingCos));
                if (heading < 0) heading += 360;
                double accuracy = event.values.length > 4 && event.values[4] >= 0 ? Math.toDegrees(event.values[4]) : -1;
                dispatchReading(ih, heading, -1, accuracy, timeMs);
                return;
            }
            dispatchReading(ih, event.values[0], event.values[1], event.values[2], timeMs);
        }
    }

    @Keep
    public static boolean start(long ih, int type, int intervalMs)
    {
        SensorManager sm = manager();
        if (sm == null) { dispatchError(ih, "Sensor service not available"); return false; }

        Sensor sensor = sm.getDefaultSensor(androidType(type));
        if (sensor == null) { dispatchError(ih, "Sensor not available"); return false; }

        SensorEventListener listener = new Listener(ih, type, intervalMs);
        int periodUs = type == COMPASS ? SensorManager.SENSOR_DELAY_GAME : Math.max(intervalMs, 0) * 1000;

        stop(ih);
        if (!sm.registerListener(listener, sensor, periodUs, new Handler(Looper.getMainLooper())))
        {
            dispatchError(ih, "Sensor registration failed");
            return false;
        }
        sListeners.put(ih, listener);
        return true;
    }

    @Keep
    public static void stop(long ih)
    {
        SensorEventListener listener = sListeners.remove(ih);
        if (listener == null) return;
        SensorManager sm = manager();
        if (sm != null) sm.unregisterListener(listener);
    }

    public static native void dispatchReading(long ihandlePtr, double x, double y, double z, long timeMs);
    public static native void dispatchError(long ihandlePtr, String message);
}
