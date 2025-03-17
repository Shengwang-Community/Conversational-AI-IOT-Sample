# 蓝牙低功耗（BLE）设备配网库

## 项目概述

这是一个用于Android设备的蓝牙低功耗（BLE）设备配网库，主要用于通过BLE连接智能设备并配置其WiFi网络连接。该项目包含两个主要模块：

1. **lib模块**：核心BLE功能库，提供BLE设备扫描、连接、Wifi管理和数据交互的功能
2. **app模块**：示例应用，展示如何使用lib库进行BLE设备配网

## 功能特点

### 核心库功能

- BLE设备扫描与发现
- BLE设备连接管理
- BLE数据传输
- 当前连接WiFi信息

### 示例应用功能

- 权限请求界面
- WiFi信息获取
- BLE设备扫描
- 设备列表显示
- 设备连接与配网

## 技术架构

### 核心库架构

- **回调接口**：`BleConnectionCallback`, `BleScanCallback`, `BleListener`
- **连接器**：`BleConnector`, `IBleConnector`
- **扫描器**：`BleScanner`, `IBleScanner`
- **管理器**：`BleManager`, `IBleManager`, `WifiManager`, `IWifiManager`
- **数据模型**：`BleDevice`, `WifiInfo`
- **状态管理**：`BleConnectionState`, `BleScanState`
- **工具类**：`BleUtils`, `BleLogger`

### 示例应用架构

- **MVVM架构**：使用ViewModel和StateFlow
- **Jetpack Compose UI**：现代化的声明式UI
- **权限管理**：`BlePermissionManager`
- **ANR监控**：`AnrMonitor`

## 使用方法

### 添加依赖

```gradle
implementation project(":lib")
```

### 初始化BLE日志

```kotlin
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
```

### 创建BLE管理器

```kotlin
val bleManager = BleManager(context)
```

### 添加监听器

```kotlin
bleManager.addListener(object : BleListener {
    override fun onScanStateChanged(state: BleScanState) {
        // 处理扫描状态变化
    }
    
    override fun onDeviceFound(device: BleDevice) {
        // 处理发现的设备
    }
    
    override fun onConnectionStateChanged(state: BleConnectionState) {
        // 处理连接状态变化
    }
    
    override fun onDataReceived(uuid: String, data: ByteArray) {
        // 处理接收到的数据
    }
})
```

### 扫描设备

```kotlin
bleManager.startScan(null)  // 无过滤器扫描所有设备
```

### 蓝牙设备连接与配网流程

1. **连接蓝牙设备**
```kotlin
bleManager.connect(device.device)
```

2. **获取设备ID**
```kotlin
val deviceId = bleManager.getDeviceId()
```

3. **获取Token**
由业务层完成

4. **配置WiFi网络**
```kotlin
bleManager.distributionNetwork(device.device, ssid, password, token, url)
```

5. **断开连接**
```kotlin
bleManager.disconnect()
```

请确保按照上述在子线程中顺序执行操作，先连接设备，然后获取设备ID，最后进行WiFi配网和断开连接。

## 权限要求

应用需要以下权限：

```xml
<!-- WiFi权限 -->
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />

<!-- 蓝牙权限 -->
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
```


## 注意事项

1. 需要在Android 6.0及以上设备上动态请求权限
2. 对于Android 12及以上设备，需要特别注意`BLUETOOTH_SCAN`和`BLUETOOTH_CONNECT`权限
3. 位置权限对于BLE扫描是必需的
4. 确保目标设备支持BLE功能
