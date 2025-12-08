# Warning

This service project is only for developer quick experience and demonstration purposes.
Do not use in production environment. Production environment services need to be developed by developers.

# IoT Conversational AI Server 🚀

[![Python 3.7+](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/downloads/)

This server provides APIs for managing IoT devices and conversational AI agents through TenAI integration, built with Python HTTP server.

## Features

- 🎙️ TenAI agent integration (start, stop, generate token)
- 🔐 Secure token generation via TenAI API
- ⏰ Automatic ping mechanism (every 10 seconds)
- 📦 Easy deployment options
- 📊 Comprehensive logging

## Requirements

- Python 3.7+
- Required packages:
  ```bash
  requests
  ```

## Installation

1. Clone this repository
2. Install dependencies:

```bash
pip install requests
```

## Configuration

### TenAI Configuration File

Create a `tenai_config.json` file with the following structure:

```json
{
  "tenai_agent_url": "https://agent.theten.ai",
  "ai_agent_channel_name": "default_channel",
  "ai_agent_user_id": 1,
  "graph_name": "va_openai_azure",
  "tenai_audio_codec": "{\"che.audio.custom_payload_type\":9}",
  "greeting": "Hello",
  "prompt": "You are a helpful assistant.",
  "language": "en-US",
  "voice_type": "male"
}
```

**Configuration Parameters:**

- `tenai_agent_url`: TenAI agent service URL
- `ai_agent_channel_name`: Default channel name (fallback if not provided in request)
- `ai_agent_user_id`: Default user ID (fallback if not provided in request)
- `graph_name`: Graph name for agent (default: "va_openai_azure")
- `tenai_audio_codec`: Audio codec parameters (JSON string)
- `greeting`: Greeting message
- `prompt`: System prompt for the agent
- `language`: Language setting (default: "en-US")
- `voice_type`: Voice type (default: "male")

## Running the Server

Start the server with:

```bash
python3 main.py
```

The server will run on port 5001 by default.

## API Documentation

### Base URL

`http://localhost:5001`

### API Endpoints

#### POST /device

Generate RTC token using TenAI agent.

**Request body:**

```json
{
  "channel_name": "your_channel_name",
  "uid": 123
}
```

**Response:**

```json
{
  "app_id": "app id",
  "token": "access token"
}
```

**Example:**

```bash
curl -X POST http://localhost:5001/device \
  -H "Content-Type: application/json" \
  -d '{"channel_name": "test_channel", "uid": 123}'
```

#### POST /agent/start

Start a TenAI conversational agent. This will automatically start a ping timer that sends ping requests every 10 seconds.

**Request body:**

```json
{
  "channel_name": "convoai-datastream",
  "uid": "1"
}
```

**Response:**
Returns the response from TenAI agent start API.

**Example:**

```bash
curl -X POST http://localhost:5001/agent/start \
  -H "Content-Type: application/json" \
  -d '{"channel_name": "convoai-datastream", "uid": "1"}'
```

**Note:** After a successful start request, the server automatically starts a ping timer that sends ping requests to `https://agent.theten.ai/api/agents/ping` every 10 seconds.

#### POST /agent/stop

Stop a TenAI conversational agent. This will also stop the ping timer.

**Request body:**

```json
{
  "channel_name": "convoai-datastream"
}
```

**Response:**
Returns the response from TenAI agent stop API.

**Example:**

```bash
curl -X POST http://localhost:5001/agent/stop \
  -H "Content-Type: application/json" \
  -d '{"channel_name": "convoai-datastream"}'
```

**Note:** After a successful stop request, the ping timer is automatically stopped.

## Automatic Ping Mechanism

When an agent is started via `/agent/start`, the server automatically starts a background timer that:

- Sends ping requests to `https://agent.theten.ai/api/agents/ping` every 10 seconds
- Uses the `channel_name` from the start request
- Generates a new UUID for each ping request
- Stops automatically when `/agent/stop` is called

## Logging

Logs are written to stdout with the following format:

```
[timestamp] [level] - [message]
```

Log levels:

- INFO: General operational messages
- WARNING: Indicates potential issues
- ERROR: Errors that need attention

## Error Handling

All endpoints return appropriate HTTP status codes:

- `200`: Success
- `400`: Bad Request (missing or invalid parameters)
- `404`: Not Found (invalid endpoint)
- `500`: Internal Server Error

Error responses follow this format:

```json
{
  "error": "Error message description"
}
```

## Security Considerations

- Always keep your TenAI agent URL and configuration secure
- Use HTTPS in production environments
- Validate all input parameters
- Implement rate limiting for API endpoints

## Code Structure

The server is built using Python's `http.server` module with the following main components:

- **RequestHandler**: Handles HTTP requests and routes them to appropriate handlers
- **TenAI Integration**: Functions for communicating with TenAI agent API
- **Ping Timer**: Background thread for automatic ping requests
- **Configuration Management**: Loads settings from `tenai_config.json`

## License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.
