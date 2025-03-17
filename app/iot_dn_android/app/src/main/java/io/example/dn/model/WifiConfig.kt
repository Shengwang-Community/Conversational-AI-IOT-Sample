package io.example.dn.model

/**
 * WiFi configuration information
 * 
 * @property ssid The SSID (network name) of the WiFi network
 * @property pwd The password for the WiFi network, can be null for open networks
 */
data class WifiConfig(
    val ssid: String,
    val pwd: String? = null
)