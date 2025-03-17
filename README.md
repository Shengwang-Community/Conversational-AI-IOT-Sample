# 使用说明

1. 该示例项目包含 app 端、BK7258 device 端，以及配套 server 端组成，旨在展示如何在BK7258芯片平台上集成RTSA Lite SDK，并与ConversationalAI建立连接实现与AI Agent实时对话。
2. 使用时请分别参考三个端侧对应开源工程，分别部署 server，安装 APK 后，再与 BK7258 device 端进行联调。
3. 各端开源工程集成方法分别见各自目录下 README.md 文件说明：
   - [APP: app/iot_dn_android/README.md](app/iot_dn_android/README.md)
   - [Server: server/aiot_server_demo_example/README.md](server/aiot_server_demo_example/README.md)
   - [BK7258: device/README.md](device/README.md)

## 关于声网

声网媒体流加速（原实时码流加速，Real-Time Streaming Acceleration，RTSA）提供优质的音视频流传输，帮助开发者通过第三方或自研编解码模块为智能硬件实现人与人、人与物、物与物的实时互动连接。依托声网自建的底层实时传输网络 Agora SD-RTN™ (Software Defined Real-time Network)，为所有支持网络功能的 Linux/RTOS 设备提供音视频码流在互联网实时传输的能力。该方案充分利用了声网全球全网节点和智能动态路由算法，与此同时支持了前向纠错、智能重传、带宽预测、码流平滑等多种组合抗弱网的策略，可以在设备所处的各种不确定网络环境下，仍然交付高连通、高实时和高稳定的最佳音视频网络体验。此外，该方案具有极小的包体积和内存占用，适合运行在任何资源受限的 IoT 设备上，包括 ESP32S3、BK7258 等SOC产品。

## 技术支持

请按照下面的链接获取技术支持：

- 如果发现了示例代码的 bug 和有其他疑问，可以直接联系商务负责人

我们会尽快回复。
