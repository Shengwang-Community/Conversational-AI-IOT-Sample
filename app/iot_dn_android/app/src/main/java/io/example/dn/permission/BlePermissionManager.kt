package io.example.dn.permission

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

/**
 * Manager class for handling BLE-related permissions.
 * Handles requesting and checking BLE permissions on Android devices.
 *
 * @property activity The activity context used for permission requests
 */
class BlePermissionManager(private val activity: ComponentActivity) {
    private var permissionCallback: BlePermissionCallback? = null

    /**
     * ActivityResultLauncher for handling multiple permission requests
     */
    private val requestMultiplePermissionsLauncher: ActivityResultLauncher<Array<String>> =
        activity.registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { permissions ->
            val allGranted = permissions.values.all { it }
            if (allGranted) {
                permissionCallback?.onPermissionsGranted()
            } else {
                permissionCallback?.onPermissionsDenied()
            }
        }

    /**
     * Sets the callback for permission request results
     * @param callback The callback to receive permission results
     */
    fun setCallback(callback: BlePermissionCallback) {
        this.permissionCallback = callback
    }

    /**
     * Checks and requests required BLE permissions if not already granted
     */
    fun checkAndRequestPermissions() {
        val permissionsToRequest = getRequiredPermissions().filter {
            ActivityCompat.checkSelfPermission(activity, it) != PackageManager.PERMISSION_GRANTED
        }.toTypedArray()

        if (permissionsToRequest.isNotEmpty()) {
            ActivityCompat.requestPermissions(activity, permissionsToRequest, PERMISSION_REQUEST_CODE)
        }
    }

    /**
     * Legacy method for handling permission results.
     * Kept for backwards compatibility but not used in new permission flow.
     *
     * @param requestCode The request code passed to requestPermissions
     * @param permissions The requested permissions
     * @param grantResults The grant results for the corresponding permissions
     */
    fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        if (requestCode == PERMISSION_REQUEST_CODE) {
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            if (allGranted) {
                permissionCallback?.onPermissionsGranted()
            } else {
                permissionCallback?.onPermissionsDenied()
            }
        }
    }

    /**
     * Gets the array of required BLE permissions based on Android version
     * @return Array of permission strings needed for BLE operations
     */
    private fun getRequiredPermissions(): Array<String> {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION
            )
        } else {
            arrayOf(
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION
            )
        }
    }

    companion object {
        private const val PERMISSION_REQUEST_CODE = 100
    }
}