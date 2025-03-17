package io.example.dn

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.core.app.ActivityCompat
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import io.example.dn.model.DeviceConnectConfig
import io.example.dn.model.WifiConfig
import io.example.dn.permission.BlePermissionCallback
import io.example.dn.permission.BlePermissionManager
import io.example.dn.ui.theme.AndroidTheme
import io.example.dn.viewmodel.BleViewModel
import io.iot.dn.ble.log.BleLogCallback
import io.iot.dn.ble.log.BleLogLevel
import io.iot.dn.ble.log.BleLogger
import io.iot.dn.ble.model.BleDevice

class MainActivity : ComponentActivity() {
    private lateinit var permissionManager: BlePermissionManager
    private val bleViewModel: BleViewModel by viewModels()
    private val anrMonitor = AnrMonitor()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        permissionManager = BlePermissionManager(this)

        BleLogger.init(object : BleLogCallback {
            override fun onLog(level: BleLogLevel, tag: String, message: String) {
                when (level) {
                    BleLogLevel.DEBUG -> Log.d(tag, message)
                    BleLogLevel.INFO -> Log.i(tag, message)
                    BleLogLevel.WARN -> Log.w(tag, message)
                    BleLogLevel.ERROR -> Log.e(tag, message)
                }
            }
        })

        setContent {
            AndroidTheme {
                BleScreen()
            }
        }

        permissionManager.setCallback(object : BlePermissionCallback {
            override fun onPermissionsGranted() {
                // 权限已获取，可以开始扫描

            }

            override fun onPermissionsDenied() {
                // 显示错误提示
                showError("需要蓝牙和定位权限才能使用此功能")
            }
        })

        anrMonitor.start()
    }

    override fun onDestroy() {
        super.onDestroy()
        anrMonitor.stop()
    }

    private fun showError(message: String) {
        // 显示错误信息，可以使用Toast或者Dialog
        setContent {
            AndroidTheme {
                Box(modifier = Modifier.fillMaxSize()) {
                    Text(text = message)
                }
            }
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        permissionManager.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    fun getPermissionManager(): BlePermissionManager {
        return permissionManager
    }
}

@Composable
fun BleScreen(viewModel: BleViewModel = viewModel()) {
    val devices by viewModel.devices.collectAsStateWithLifecycle()
    val isScanning by viewModel.isScanning.collectAsStateWithLifecycle()
    val message by viewModel.message.collectAsStateWithLifecycle()
    val wifiInfo by viewModel.wifiInfo.collectAsStateWithLifecycle()
    val wifiPassword by viewModel.wifiPassword.collectAsStateWithLifecycle()
    val context = LocalContext.current

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // 权限申请按钮
        Row(
            modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            // WiFi权限申请按钮
            Button(
                onClick = {
                    (context as? MainActivity)?.getPermissionManager()?.checkAndRequestPermissions()
                }, modifier = Modifier.weight(1f)
            ) {
                Text("申请权限")
            }
        }

        Spacer(modifier = Modifier.height(8.dp))

        // WiFi权限检查按钮
        Button(
            onClick = {
                val hasWifiPermissions = viewModel.checkWiFiPermission()
                Toast.makeText(
                    context, if (hasWifiPermissions) "已获取WiFi权限" else "未获取WiFi权限", Toast.LENGTH_SHORT
                ).show()
            }, modifier = Modifier.fillMaxWidth()
        ) {
            Text("检查WiFi权限")
        }

        Spacer(modifier = Modifier.height(8.dp))

        // 获取当前WiFi信息按钮
        Button(
            onClick = {
                viewModel.getCurrentWifiInfo()
            }, modifier = Modifier.fillMaxWidth()
        ) {
            Text("获取当前WiFi信息")
        }

        // 显示WiFi信息
        if (wifiInfo != "未知") {
            Card(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 8.dp)
            ) {
                Text(
                    text = wifiInfo, modifier = Modifier.padding(16.dp), style = MaterialTheme.typography.bodyMedium
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // WiFi密码输入框
        OutlinedTextField(
            value = wifiPassword,
            onValueChange = { viewModel.setWifiPassword(it) },
            label = { Text("WiFi密码") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true,
            keyboardOptions = KeyboardOptions(
                keyboardType = KeyboardType.Text,
                imeAction = ImeAction.Done
            )
        )

        Spacer(modifier = Modifier.height(16.dp))

        // 扫描按钮
        Button(
            onClick = {
                val missingPermissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    listOf(
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                    ).any { permission ->
                        ActivityCompat.checkSelfPermission(context, permission) != PackageManager.PERMISSION_GRANTED
                    }
                } else {
                    ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED
                }
                
                if (missingPermissions) {
                    (context as? MainActivity)?.getPermissionManager()?.checkAndRequestPermissions()
                    return@Button
                }
                
                if (isScanning) viewModel.stopScan()
                else viewModel.startScan()
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (isScanning) "停止扫描" else "开始扫描")
        }

        Spacer(modifier = Modifier.height(16.dp))

        // 消息提示
        if (message.isNotEmpty()) {
            Text(
                text = message, modifier = Modifier.padding(vertical = 8.dp)
            )
        }

        // 设备列表
        LazyColumn(
            modifier = Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            items(devices.toList()) { device ->
                DeviceCard(
                    device = device,
                    onConnect = { viewModel.connect(device) },
                    onDisconnect = { viewModel.disconnect() },
                    ssid = wifiInfo,
                    password = wifiPassword,
                    onActivate = { config ->
                        // 这里可以添加配网逻辑
                        viewModel.configureDevice(
                            device,
                            config.wifiConfig.ssid,
                            config.wifiConfig.pwd ?: ""
                        )
                    }
                )
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DeviceCard(
    device: BleDevice,
    onConnect: (() -> Unit)? = null,
    onDisconnect: (() -> Unit)? = null,
    ssid: String? = null,
    password: String? = null,
    onActivate: ((DeviceConnectConfig) -> Unit)? = null
) {
    Card(modifier = Modifier.fillMaxWidth(), onClick = { }) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
        ) {
            // 设备基本信息
            Text(
                text = device.name, style = MaterialTheme.typography.titleMedium
            )
            Text(
                text = "MAC: ${device.address}", style = MaterialTheme.typography.bodyMedium
            )
            Text(
                text = "RSSI: ${device.rssi}", style = MaterialTheme.typography.bodyMedium
            )

            Spacer(modifier = Modifier.height(8.dp))

            // 建立连接按钮
            Button(
                onClick = {
                    onConnect?.invoke()
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("建立连接")
            }

            Spacer(modifier = Modifier.height(8.dp))

            // 配置网络按钮
            Button(
                onClick = {
                    val config = DeviceConnectConfig(
                        wifiConfig = WifiConfig(ssid = ssid ?: "", pwd = password ?: "")
                    )
                    onActivate?.invoke(config)
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("配置网络")
            }

            // 断开连接按钮
            Button(
                onClick = {
                    onDisconnect?.invoke()
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("断开连接")
            }
        }
    }
}