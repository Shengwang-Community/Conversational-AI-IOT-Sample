package io.iot.dn.ble.common

import android.Manifest
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.Build
import android.util.Log
import androidx.core.content.ContextCompat
import androidx.core.location.LocationManagerCompat

/**
 * Utility class for Bluetooth Low Energy operations
 */
object BleUtils {
    /**
     * Check if BLE is available and ready to use
     * @param context Android context
     * @return true if BLE is available and all requirements are met
     */
    fun isBleAvailable(context: Context): Boolean {
        return checkBleSupport(context) &&
                checkBleEnabled(context) &&
                checkLocationEnabled(context) &&
                checkBlePermissions(context)
    }

    /**
     * Check if device supports BLE
     * @param context Android context
     * @return true if BLE is supported
     */
    private fun checkBleSupport(context: Context): Boolean {
        Log.d("BleUtils", "checkBleSupport: ${context.packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)}")
        return context.packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)
    }

    /**
     * Check if Bluetooth is enabled
     * @param context Android context
     * @return true if Bluetooth is enabled
     */
    private fun checkBleEnabled(context: Context): Boolean {
        val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        Log.d("BleUtils", "checkBleEnabled: ${bluetoothManager.adapter?.isEnabled}")
        return bluetoothManager.adapter?.isEnabled == true
    }

    /**
     * Check if Location services are enabled
     * @param context Android context
     * @return true if location services are enabled
     */
    private fun checkLocationEnabled(context: Context): Boolean {
        val locationManager = context.getSystemService(Context.LOCATION_SERVICE) as LocationManager
        // For Android 12+ (API 31+), location switch check is not required
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            return true
        }
        
        // On OPPO/OnePlus/Realme devices, location switch check may be unreliable, always return true
        if (isOppoDevice()) {
            Log.d("BleUtils", "OPPO device detected, skipping location check")
            return true
        }

        Log.d("BleUtils", "checkLocationEnabled: ${LocationManagerCompat.isLocationEnabled(locationManager)}")
        return LocationManagerCompat.isLocationEnabled(locationManager)
    }
    
    /**
     * Check if the device is OPPO/OnePlus/Realme
     * @return true if device is OPPO/OnePlus/Realme
     */
    private fun isOppoDevice(): Boolean {
        val manufacturer = Build.MANUFACTURER.lowercase()
        val brand = Build.BRAND.lowercase()
        return manufacturer.contains("oppo") || 
               manufacturer.contains("oneplus") ||
               brand.contains("oppo") ||
               brand.contains("oneplus") ||
               manufacturer.contains("realme") ||
               brand.contains("realme")
    }

    /**
     * Check if required BLE permissions are granted
     * @param context Android context
     * @return true if all required permissions are granted
     */
    fun checkBlePermissions(context: Context): Boolean {
        val hasScanPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_SCAN
            ) == PackageManager.PERMISSION_GRANTED
        } else {
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH
            ) == PackageManager.PERMISSION_GRANTED &&
                    ContextCompat.checkSelfPermission(
                        context,
                        Manifest.permission.BLUETOOTH_ADMIN
                    ) == PackageManager.PERMISSION_GRANTED
        }

        val hasConnectPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_CONNECT
            ) == PackageManager.PERMISSION_GRANTED
        } else {
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH
            ) == PackageManager.PERMISSION_GRANTED
        }

        Log.d("BleUtils", "checkBlePermissions: hasScanPermission: $hasScanPermission")
        Log.d("BleUtils", "checkBlePermissions: hasConnectPermission: $hasConnectPermission")
        return hasScanPermission && hasConnectPermission
    }
}