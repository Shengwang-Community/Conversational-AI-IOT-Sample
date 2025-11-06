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

    // 用于存储WiFi列表的数据
    private val _wifiList = MutableStateFlow<List<String>>(emptyList())
    val wifiList = _wifiList.asStateFlow()

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
                if (device.name.startsWith("X1-") || device.name.startsWith("R1-")) {
                    _devices.value += device
                }
            }

            override fun onConnectionStateChanged(state: BleConnectionState) {
                Log.d(TAG, "onConnectionStateChanged: $state")
                _message.value = "Connection status: $state"

            }

            override fun onDataReceived(uuid: String, data: ByteArray) {
                Log.d(TAG, "onDataReceived: $uuid, ${formatByteArray(data)}")
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
     * @param password WiFi password
     * @param token Authentication token
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun configureDevice(ssid: String, password: String, token: String, url: String) {
        viewModelScope.launch {
            try {
                val ret = bleManager.distributionNetwork(ssid, password, token, url)
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
     * Get WiFi list
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun getWifiList() {
        viewModelScope.launch {
            try {
                val ret = bleManager.queryWifiList()
                // 将JSON字符串转换为Array<String>
                val wifiArray = parseWifiListFromJson(ret)
                _wifiList.value = wifiArray.toList()
            } catch (e: BleError) {
                _wifiList.value = emptyList()
            }
        }
    }

    /**
     * Parse WiFi list from JSON string to Array<String>
     * @param jsonString JSON string like ["HUAWEI-G108S1","NXIOT",...]
     * @return Array of WiFi SSIDs
     */
    private fun parseWifiListFromJson(jsonString: String): Array<String> {
        return try {
            // 移除首尾的方括号，然后按逗号分割
            val trimmed = jsonString.trim()
            if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                val content = trimmed.substring(1, trimmed.length - 1)
                if (content.isBlank()) {
                    emptyArray()
                } else {
                    // 分割并移除每个元素周围的引号
                    content.split(",")
                        .map { it.trim().removeSurrounding("\"") }
                        .filter { it.isNotBlank() }
                        .toTypedArray()
                }
            } else {
                // 如果不是JSON格式，尝试按逗号分割
                jsonString.split(",")
                    .map { it.trim() }
                    .filter { it.isNotBlank() }
                    .toTypedArray()
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to parse WiFi list JSON: $jsonString", e)
            emptyArray()
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
     * Set WiFi info (SSID)
     * @param ssid The WiFi SSID to set
     */
    fun setWifiInfo(ssid: String) {
        _wifiInfo.value = ssid
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

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    fun bleApn() {
        viewModelScope.launch {
            val ret = bleManager.startBleAPN()
            _message.value = "BLE APN: $ret"
        }
    }

    companion object {
        private const val TAG = "BleViewModel"

        /**
         * Format byte array for logging with mixed format: decimal before '[', string after '['
         */
        private fun formatByteArray(data: ByteArray): String {
            if (data.isEmpty()) {
                return "[]"
            }

            val utf8String = try {
                String(data, Charsets.UTF_8)
            } catch (e: Exception) {
                return "Invalid UTF-8: ${data.joinToString(" ") { it.toString() }}"
            }

            // Find the position of '[' character
            val bracketIndex = utf8String.indexOf('[')

            return if (bracketIndex > 0) {
                // Split the data: binary part (decimal) + string part
                val binaryPart = data.take(bracketIndex).toByteArray()
                val stringPart = data.drop(bracketIndex).toByteArray()

                val binaryDecimal = binaryPart.joinToString(" ") { it.toString() }
                val stringContent = try {
                    String(stringPart, Charsets.UTF_8)
                } catch (e: Exception) {
                    "Invalid string part"
                }

                "Binary: [$binaryDecimal] + String: \"$stringContent\""
            } else if (bracketIndex == 0) {
                // Data starts with '[', all string
                "String: \"$utf8String\""
            } else {
                // No '[' found, treat as binary data
                "Binary: [${data.joinToString(" ") { it.toString() }}]"
            }
        }
    }
}