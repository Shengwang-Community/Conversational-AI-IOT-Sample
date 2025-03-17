package io.example.dn.viewmodel

import android.Manifest
import android.app.Application
import android.util.Log
import androidx.annotation.RequiresPermission
import androidx.lifecycle.AndroidViewModel
import io.iot.dn.ble.common.BleUtils
import io.iot.dn.ble.error.BleError
import io.iot.dn.ble.manager.BleManager
import io.iot.dn.ble.model.BleDevice
import io.iot.dn.ble.state.BleConnectionState
import io.iot.dn.ble.state.BleScanState
import io.iot.dn.wifi.manager.WifiManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * ViewModel for managing BLE device scanning, connection and data transfer
 */
class BleViewModel(application: Application) : AndroidViewModel(application) {
    private val bleManager = BleManager(application)
    private val wifiManager = WifiManager(application)

    // List of discovered BLE devices
    private val _devices = MutableStateFlow<Set<BleDevice>>(emptySet())
    val devices = _devices.asStateFlow()

    // Scanning state
    private val _isScanning = MutableStateFlow(false)
    val isScanning = _isScanning.asStateFlow()

    // Status message
    private val _message = MutableStateFlow("")
    val message = _message.asStateFlow()

    // Configuration status
    private val _configStatus = MutableStateFlow("")
    val configStatus = _configStatus.asStateFlow()

    // WiFi password
    private val _wifiPassword = MutableStateFlow("88888888")
    val wifiPassword = _wifiPassword.asStateFlow()

    // WiFi info
    private val _wifiInfo = MutableStateFlow<String>("Unknown")
    val wifiInfo = _wifiInfo.asStateFlow()

    private val viewModelScope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    init {
        bleManager.addListener(object : io.iot.dn.ble.callback.BleListener {
            override fun onScanStateChanged(state: BleScanState) {
                _message.value = "Scan status: $state"
                when (state) {
                    BleScanState.IDLE -> {
                        _message.value = ""
                        _isScanning.value = false
                    }

                    BleScanState.SCANNING -> {
                        _message.value = "Scanning..."
                        _isScanning.value = true
                    }

                    BleScanState.STOPPED -> {
                        _message.value = "Scan stopped"
                        _isScanning.value = false
                    }

                    BleScanState.TIMEOUT -> {
                        _message.value = "Scan timeout"
                        _isScanning.value = false
                    }

                    BleScanState.FAILED -> {
                        _message.value = "Scan failed"
                        _isScanning.value = false
                    }

                    BleScanState.FOUND -> {

                    }
                }
            }

            override fun onDeviceFound(device: BleDevice) {
                // Check if device already exists
                if (_devices.value.any { it.address == device.address }) {
                    return
                }
                Log.d(TAG, "onDeviceFound: ${device.name}, ${device.address}")
                if (device.name.startsWith("X1-") || device.name.startsWith("bk-")) {
                    _devices.value += device
                }
            }

            override fun onConnectionStateChanged(state: BleConnectionState) {
                Log.d(TAG, "onConnectionStateChanged: $state")
                _message.value = "Connection status: $state"

            }

            override fun onDataReceived(uuid: String, data: ByteArray) {
                Log.d(TAG, "onDataReceived: $uuid, ${data.toString(Charsets.UTF_8)}")

            }

            override fun onMessageSent(
                serviceUuid: String,
                characteristicUuid: String,
                success: Boolean,
                error: String?
            ) {
                super.onMessageSent(serviceUuid, characteristicUuid, success, error)
                Log.d(TAG, "onMessageSent: $serviceUuid, $characteristicUuid, $success, $error")
            }
        })
    }

    /**
     * Start scanning for BLE devices
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_SCAN)
    fun startScan() {
        viewModelScope.launch {
            bleManager.startScan(null)
        }
    }

    /**
     * Stop scanning for BLE devices
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_SCAN)
    fun stopScan() {
        viewModelScope.launch {
            bleManager.stopScan()
        }
    }

    /**
     * Connect to a BLE device
     * @param device The device to connect to
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun connect(device: BleDevice) {
        viewModelScope.launch {
            try {
                checkBluetoothPermission()
                bleManager.connect(device.device)
                _message.value = "Connection successful"
            } catch (e: BleError) {
                _message.value = "Connection failed: ${e.message}"
            }
        }
    }

    /**
     * Configure device with WiFi settings
     * @param device The BLE device to configure
     * @param ssid WiFi SSID
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun configureDevice(
        device: BleDevice,
        ssid: String,
        password: String,
    ) {
        viewModelScope.launch {
            try {
                val ret = bleManager.distributionNetwork(device.device, ssid, password)
                _message.value = if (ret) "Configuration successful" else "Configuration failed"
            } catch (e: BleError) {
                _message.value = e.message ?: "Unknown error"
            }
        }
    }

    /**
     * Check if Bluetooth permissions are granted
     */
    private fun checkBluetoothPermission() {
        if (!BleUtils.checkBlePermissions(getApplication())) {
            throw BleError.BluetoothPermissionDenied()
        }
    }

    /**
     * Check if WiFi permissions are granted
     */
    fun checkWiFiPermission(): Boolean {
        return wifiManager.checkWifiPermissions()
    }

    /**
     * Get current WiFi network information
     */
    fun getCurrentWifiInfo() {
        viewModelScope.launch {
            try {
                val info = wifiManager.getCurrentWifiInfo()
                _wifiInfo.value = "${info?.ssid}"
            } catch (e: Exception) {
                _wifiInfo.value = "Failed to get WiFi info: ${e.message}"
            }
        }
    }

    /**
     * Set WiFi password
     * @param password The WiFi password to set
     */
    fun setWifiPassword(password: String) {
        _wifiPassword.value = password
    }

    /**
     * Disconnect from current BLE device
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun disconnect() {
        viewModelScope.launch {
            bleManager.disconnect()
        }
    }

    override fun onCleared() {
        super.onCleared()
        viewModelScope.cancel()
    }

    /**
     * Get the device ID of connected BLE device
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun getDeviceId() {
        viewModelScope.launch {
            try {
                var ret = bleManager.getDeviceId()
                _message.value = "Device ID: $ret"
            } catch (e: Exception) {
                _message.value = e.message ?: "Error getting device ID"
            }
        }
    }

    companion object {
        private const val TAG = "BleViewModel"
    }
}