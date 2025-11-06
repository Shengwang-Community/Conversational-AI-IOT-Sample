# Warning
This service project is only for developer quick experience and demonstration purposes. 
Do not use in production environment. Production environment services need to be developed by developers.

# IoT Conversational AI Server 🚀

[![Python 3.7+](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/downloads/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

This server provides APIs for managing IoT devices and conversational AI agents, built with Python and supporting real-time communication via Agora RTC.

## Features
- 🎙️ Real-time voice communication
- 🤖 Conversational AI integration
- 🔐 Secure token generation
- 📦 Easy deployment options
- 📊 Comprehensive logging

## Requirements

- Python 3.7+ 
- Required packages:
  ```bash
  requests
  flask
  pyjwt
  ```

## Installation
1. Clone this repository
2. Install dependencies:
```bash
pip install -r requirements.txt
```

## Configuration

### Configuration File
Create a `config.json` file with the following structure and explanations:

```json
{
  // Agora相关配置
  "app_id": "YOUR_AGORA_APP_ID",  // Agora应用ID
  "app_certificate": "YOUR_AGORA_APP_CERTIFICATE",  // Agora应用证书
  
  // 客户认证信息
  "customer_key": "YOUR_CUSTOMER_KEY",  // 客户密钥
  "customer_secret": "YOUR_CUSTOMER_SECRET",  // 客户密钥
  
  // 语音识别(ASR)配置
  "asr": {
    "language": "zh-CN"  // 识别语言，默认中文,（支持中英文混合）
  },
  
  // 系统参数配置
  "parameters": {
    "output_audio_codec": "PCMA"  // 	rtc发流音频编码格式，支持格式："PCMU" "PCMA" "G722" "OPUS" "OPUSFB"
    "transcript": {               //  字幕功能参数配置
      "enable": false             //  禁用字幕功能
    }
  },
  
  // 语音合成(TTS)配置
  "tts": {
    "vendor": "YOUR_TTS_VENDOR",  // TTS服务提供商
    "params": {
      
    }
  },
  
  // 会话超时配置
  "idle_timeout": 30,  // 会话超时时间（秒）
  
  // 大语言模型(LLM)配置
  "llm": {
    "url": "YOUR_LLM_API_URL",  // LLM服务地址
    "params": {
      "model": "YOUR_LLM_MODEL"  // 使用的模型，
    },
    "api_key": "YOUR_LLM_API_KEY",  // LLM服务API密钥
    "system_messages": [  // 系统预设消息
      {
        "role": "system",
        "content": "You are a helpful chatbot."
      }
    ],
    "max_history": 10,  // 最大历史记录数
    "greeting_message": "你好，我是小爱，有什么可以帮助你的吗？",  // 欢迎语
    "failure_message": "抱歉，我暂时无法回答您的问题..."  // 失败提示
  }
}
```
更多详细参数配置见：```https://doc.shengwang.cn/doc/convoai/restful/convoai/operations/start-agent```

## Running the Server
Start the server with:
```bash
python3 main.py
```

The server will run on port 5001 by default.

## API Documentation

### Base URL
`https://your-domain.com/api/v1`

### Authentication
All requests require an Authorization header:
```http
Authorization: Bearer <access_token>
```

### API Endpoints

### POST /device
Register a new device and generate RTC token

Request body:
```json
{
  "channel_name": "DEVICE_ID",
  "uid": DEVICE_USER_ID
}
```

### POST /agent/start 
Start a conversational AI agent

Request body:
```json
{
  "channel_name": "DEVICE_ID",
  "uid": DEVICE_USER_ID,
  "agent_uid": AGENT_USER_ID
}
```

### POST /agent/stop
Stop a conversational AI agent

Request body:
```json
{
  "agent_id": "AGENT_ID"
}
```

## Logging

Logs are written to stdout with the following format:
```
[timestamp] [level] - [message]
```

Log levels:
- DEBUG: Detailed debug information
- INFO: General operational messages
- WARNING: Indicates potential issues
- ERROR: Errors that need attention
- CRITICAL: Critical system failures

## Security Considerations

- Always keep your Agora credentials secure
- Use HTTPS in production environments
- Regularly rotate your access tokens
- Implement rate limiting for API endpoints

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Example Usage

```python
import requests

# Generate RTC token
response = requests.post(
    "https://your-domain.com/device",
    json={"channel_name": "12345", "uid": 1}
)
print(response.json())
```
