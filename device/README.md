# 声网 BK7258 Demo工程

*简体中文| [English](README.en.md)*

## 例程简介

本例程演示了如何通过 BK7358 AI Robotic Kid 开发板，集成声网RTSA Lite SDK，实现音视频通话功能演示。

### 文件结构
```
├── agora_convoai
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   └── config
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── agora_config.h
│   │   ├── agora_convoai_iot.c
│   │   ├── agora_convoai_iot.h
│   │   ├── agora_rtc_demo.c
│   │   ├── agora_sdk
│   │   │   └── agora_rtc.c
│   │   ├── app_event.c
│   │   ├── app_event.h
│   │   ├── app_main.c
│   │   ├── asr
│   │   │   ├── armino_asr.c
│   │   │   └── armino_asr.h
│   │   ├── audio
│   │   │   └── audio_transfer.c
│   │   ├── avi_play
│   │   │   └── avi_play.c
│   │   ├── boarding_service
│   │   │   ├── boarding_core.c
│   │   │   ├── boarding_service.c
│   │   │   ├── wifi_boarding_internal.h
│   │   │   └── wifi_boarding_utils.c
│   │   ├── bt_pan
│   │   │   ├── bt_comm_list.c
│   │   │   ├── bt_comm_list.h
│   │   │   ├── bt_manager.c
│   │   │   ├── bt_manager.h
│   │   │   ├── pan_demo_cli.c
│   │   │   ├── pan_service.c
│   │   │   ├── pan_service.h
│   │   │   ├── pan_user_config.h
│   │   │   └── storage
│   │   │       ├── bluetooth_storage.c
│   │   │       └── bluetooth_storage.h
│   │   ├── CMakeLists.txt
│   │   ├── countdown.c
│   │   ├── flow_control
│   │   │   └── fpscc.c
│   │   ├── include
│   │   │   ├── agora_rtc.h
│   │   │   ├── audio_transfer.h
│   │   │   ├── bk_genie_comm.h
│   │   │   ├── bk_genie_smart_config.h
│   │   │   ├── boarding_service.h
│   │   │   ├── countdown.h
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   ├── led_blink.h
│   │   │   ├── motor.h
│   │   │   └── wifi_boarding_utils.h
│   │   ├── Kconfig.projbuild
│   │   ├── led_blink.c
│   │   ├── motor.c
│   │   ├── resource
│   │   │   ├── agent_joined_16k_mono_16bit_en.wav
│   │   │   ├── agent_offline_16k_mono_16bit_en.wav
│   │   │   ├── asr_standby_16k_mono_16bit_en.wav
│   │   │   ├── asr_wakeup_16k_mono_16bit_en.wav
│   │   │   ├── genie_eye.avi
│   │   │   ├── low_voltage_16k_mono_16bit_en.wav
│   │   │   ├── network_provision_16k_mono_16bit_en.wav
│   │   │   ├── network_provision_fail_16k_mono_16bit_en.wav
│   │   │   ├── network_provision_success_16k_mono_16bit_en.wav
│   │   │   ├── reconnect_network_16k_mono_16bit_en.wav
│   │   │   ├── reconnect_network_fail_16k_mono_16bit_en.wav
│   │   │   ├── reconnect_network_success_16k_mono_16bit_en.wav
│   │   │   └── rtc_connection_lost_16k_mono_16bit_en.wav
│   │   ├── smart_config
│   │   │   └── bk_genie_smart_config.c
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
└── README.md
```

## 环境配置

### 硬件要求

本例程目前仅支持`BK7258 AI Robotic Kid`开发板。

## 编译和下载

### Linux 操作系统

#### 获取 bk_aidk 框架工程

本例程支持 bk_aidk branch ai_release/v[2.0.1] 及以后的，例程默认使用 tag ai_release/v[2.0.1.2] (commit id: 3cb4544aa5cdc66ad0b4ccf3175beae152dee6c8)。

开发环境搭建请参考BK官方说明：https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/zh_CN/v2.0.1/get-started/index.html

请在确认你已经从BK官方获得相关工程下载权限后，从`gitlab`获取`bk_aidk`工程，如下所示：

```bash
$ git clone --recurse-submodules git@gitlab.bekencorp.com:armino/bk_ai/bk_aidk.git -b ai_release/v2.0.1
```

#### 修改 bk_aidk 工程

1. 需要将`agora_convoai`目录，复制到`bk_aidk`工程`projects`目录下：
```bash
$ cp -rf ./agora_convoai ${bk_aidk路径}/projects/
```
2. 修改`agora_convoai/main/agora_config.h`配置文件，设置server URL为你自己部署的服务器，比如：
```
#define CONFIG_AGENT_SERVER_URL         "http://192.168.1.100:5001"
```

#### 编译固件

在bk_aidk工程目录下，构建demo固件：
```bash
$ cd ${bk_aidk路径}
$ make bk7258 PROJECT=agora_convoai
```

#### 烧录固件
固件构建完毕后，参考BK官方说明，使用烧录程序下载固件：https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/zh_CN/v2.0.1/developer-guide/config_tools/bk_tool_bkfil.html

## 如何使用例程

### 五分钟快速体验

注意：

1. 请使用Type-C数据线接入开发板`USB TO UART`接口，并与电脑连接。
2. 该接口同时用于电池充电。
3. 请注意`RST`按键位置，当烧录工具无法自动重启开发板时，可以手动重启恢复烧录能力。

### 开始注册自己的声网账号

接下来我们带您创建属于您自己的声网账号。
参考文档：https://doc.shengwang.cn/doc/convoai/restful/get-started/enable-service

#### Demo运行

1. 开发板插入电池或数据线后会自动启动。
2. 如果还未给开发板设置wifi账号及密码，请先长按`S1`5秒键进入配网模式。
3. 使用配套的APP（本示例只提供Android工程，请自行构建并安装APK）
4. 配网成功后，你可以说`hi,阿米诺`语音唤醒设备，启动和AI Agent的通话。
5. 对话完毕后，你可以说`bye bye,阿米诺`退出和AI Agent的通话。
6. 开发板在静默状态3分钟后自动进入深度休眠状态，你可以通过`RST`键重启运行。
