package io.example.dn.permission

/**
 * Callback interface for handling BLE permission results.
 */
interface BlePermissionCallback {
    /**
     * Called when all required BLE permissions are granted by the user.
     */
    fun onPermissionsGranted()

    /**
     * Called when one or more required BLE permissions are denied by the user.
     */
    fun onPermissionsDenied()
}