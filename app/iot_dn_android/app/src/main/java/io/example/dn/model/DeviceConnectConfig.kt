package io.example.dn.model

/**
 * Configuration for connecting a device
 *
 * @property wifiConfig The WiFi configuration information for the device connection
 */
data class DeviceConnectConfig(
    val wifiConfig: WifiConfig
)
