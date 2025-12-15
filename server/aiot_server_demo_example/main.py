"""
This service project is only for developer quick experience and demonstration purposes. 
Do not use in production environment. Production environment services need to be developed by developers.
"""
import uuid
import requests
import json
import logging
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

def load_tenai_config():
    """Load tenai_config.json file"""
    config_path = os.path.join(os.path.dirname(__file__), 'tenai_config.json')
    try:
        with open(config_path) as f:
            config = json.load(f)
            logger.info('Successfully loaded TenAI config file')
            return config
    except Exception as e:
        logger.error(f'Failed to load TenAI config file: {str(e)}')
        raise

# Global variables for ping timer
ping_timer = None
ping_channel_name = None
ping_timer_running = False

# Common constants
JSON_HEADERS = {'Content-Type': 'application/json'}

def _build_tenai_url(path):
    """Build TenAI agent URL from config and path"""
    config = load_tenai_config()
    tenai_agent_url = config.get('tenai_agent_url', 'https://agent.theten.ai')
    return f"{tenai_agent_url}/{path}"

def _send_tenai_request(url, payload, request_type="request"):
    """Send HTTP POST request to TenAI agent"""
    try:
        response = requests.post(url, headers=JSON_HEADERS, json=payload)
        request_id = payload.get('request_id', 'unknown')
        channel_name = payload.get('channel_name', 'unknown')
        logger.info(f'{request_type.capitalize()} sent - request_id: {request_id}, channel_name: {channel_name}, status: {response.status_code}')
        return response
    except Exception as e:
        logger.error(f'Failed to send {request_type}: {str(e)}')
        raise

def _build_channel_request_payload(channel_name):
    """Build common request payload with request_id and channel_name"""
    return {
        "request_id": str(uuid.uuid4()),
        "channel_name": channel_name
    }

def send_ping_request(channel_name):
    """Send ping request to TenAI agent"""
    ping_url = _build_tenai_url("/ping")
    payload = _build_channel_request_payload(channel_name)
    _send_tenai_request(ping_url, payload, "ping request")

def send_stop_request(channel_name):
    """Send stop request to TenAI agent"""
    stop_url = _build_tenai_url("/stop")
    payload = _build_channel_request_payload(channel_name)
    return _send_tenai_request(stop_url, payload, "stop request")

def ping_timer_worker(channel_name):
    """Worker function for ping timer"""
    global ping_timer_running
    ping_timer_running = True
    
    while ping_timer_running:
        send_ping_request(channel_name)
        # Sleep in small intervals to allow quick stopping
        for _ in range(100):  # 10 seconds = 100 * 0.1 seconds
            if not ping_timer_running:
                break
            time.sleep(0.1)

def start_ping_timer(channel_name):
    """Start ping timer for the given channel_name"""
    global ping_timer, ping_channel_name
    
    # Stop existing timer if any
    stop_ping_timer()
    
    # Set new channel name and start timer
    ping_channel_name = channel_name
    ping_timer = threading.Thread(target=ping_timer_worker, args=(channel_name,), daemon=True)
    ping_timer.start()
    logger.info(f'Started ping timer for channel_name: {channel_name}')

def stop_ping_timer():
    """Stop ping timer"""
    global ping_timer, ping_channel_name, ping_timer_running
    if ping_timer_running:
        ping_timer_running = False
        ping_channel_name = None
        logger.info('Stopped ping timer')

class RequestHandler(BaseHTTPRequestHandler):
    def _parse_request_body(self):
        """Parse JSON request body"""
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)
        return json.loads(post_data)
    
    def _validate_channel_name(self, data):
        """Validate channel_name parameter"""
        channel_name = data.get('channel_name')
        if not channel_name:
            logger.warning('Missing channel_name parameter')
            self.send_error_response(400, {'error': 'Missing channel_name parameter'})
            return None
        return channel_name
    
    def _validate_uid(self, data):
        """Validate uid parameter"""
        uid = data.get('uid')
        if not uid:
            logger.warning('Missing uid parameter')
            self.send_error_response(400, {'error': 'Missing uid parameter'})
            return None
        return uid
    
    def do_POST(self):
        """Handle POST requests"""
        if self.path == '/device':
            self._handle_generate_token_request()
        elif self.path == '/agent/start':
            self.handle_ten_agent_start_request()
        elif self.path == '/agent/stop':
            self.handle_agent_stop_request()
        else:
            self.send_response(404)
            self.end_headers()

    def _handle_generate_token_request(self):
        """Handle requests to /device endpoint - Generate token using TenAI agent"""
        try:
            data = self._parse_request_body()
            channel_name = self._validate_channel_name(data)
            if channel_name is None:
                return
            
            uid = self._validate_uid(data)
            if uid is None:
                return
            
            # Call handle_ten_agent_generate_request with uid and channel_name from request
            tenai_response = self.handle_ten_agent_generate_request(uid=uid, channel_name=channel_name)
            
            # Check if TenAI API call was successful
            if not isinstance(tenai_response, dict):
                raise Exception('Invalid response format from TenAI API')
            
            code = tenai_response.get('code', '')
            if code != '0':
                msg = tenai_response.get('msg', 'Unknown error')
                raise Exception(f'TenAI API returned error: {msg}')
            
            # Extract data from TenAI response
            data = tenai_response.get('data', {})
            if not data:
                raise Exception('No data in TenAI API response')
            
            # Extract token and appId from data
            token = data.get('token', '')
            app_id = data.get('appId', '')
            
            if not token:
                raise Exception('Token not found in TenAI API response')
            
            # Return same data structure as handle_device_request
            response = {
                'app_id': app_id,
                'token': token
            }
            
            logger.info(f'Successfully generated token via TenAI: {token[:10] if token else "empty"}...')
            self.send_success_response(response)
            
        except Exception as e:
            logger.error(f'Error occurred while handling generate token request: {str(e)}')
            self.send_error_response(500, {'error': f'Server error: {str(e)}'})

    def handle_agent_stop_request(self):
        """Handle requests to /agent/stop endpoint"""
        try:
            data = self._parse_request_body()
            channel_name = self._validate_channel_name(data)
            if channel_name is None:
                return
            
            # Call send_stop_request to TenAI agent
            response = send_stop_request(channel_name)
            
            if response.status_code == 200:
                # Stop ping timer after successful stop request
                stop_ping_timer()
                logger.info(f'Successfully stopped agent and ping timer for channel_name: {channel_name}')
                self.send_success_response(response.json())
            else:
                logger.warning(f'Stop request failed with status {response.status_code}')
                self.send_error_response(response.status_code, {'error': 'Stop request failed'})
                
        except json.JSONDecodeError as e:
            logger.error(f'Invalid JSON in request body: {str(e)}')
            self.send_error_response(400, {'error': 'Invalid JSON format'})
        except Exception as e:
            logger.error(f'Error occurred while handling agent/stop request: {str(e)}')
            self.send_error_response(500, {'error': f'Server error: {str(e)}'})    

    def handle_ten_agent_start_request(self):
        """Handle requests to /agent/start endpoint (TenAI agent start) - Client function"""
        try:
            data = self._parse_request_body()
            channel_name = self._validate_channel_name(data)
            if channel_name is None:
                return
            
            uid = self._validate_uid(data)
            if uid is None:
                return
            
            # Convert uid to int if it's a string
            try:
                user_uid = int(uid)
            except (ValueError, TypeError):
                logger.warning(f'Invalid uid parameter: {uid}')
                self.send_error_response(400, {'error': 'Invalid uid parameter, must be a number'})
                return
            
            # Build URL and request body
            start_url = _build_tenai_url("/start")
            request = self._build_start_json(channel_name=channel_name, user_uid=user_uid)
            
            # Print request (equivalent to printf in C code)
            print(f"http_with_url request ={request}")
            
            # Send HTTP POST request
            response = requests.post(start_url, headers=JSON_HEADERS, data=request)
            
            if response.status_code == 200:
                print(f"HTTPS Status = {response.status_code}, content_length = {len(response.content)}")
                # Start ping timer after successful start request
                start_ping_timer(channel_name)
                self.send_success_response(response.json())
            else:
                print(f"HTTPS Status = {response.status_code}, content_length = {len(response.content)}")
                self.send_error_response(response.status_code, {'error': 'Request failed'})
                
        except json.JSONDecodeError as e:
            logger.error(f'Invalid JSON in request body: {str(e)}')
            self.send_error_response(400, {'error': 'Invalid JSON format'})
        except requests.exceptions.RequestException as e:
            print(f"Failed to open HTTP connection: {str(e)}")
            self.send_error_response(500, {'error': f'HTTP connection failed: {str(e)}'})
        except Exception as e:
            print(f"Error perform http request {str(e)}")
            logger.error(f'Error occurred while handling agent/start request: {str(e)}')
            self.send_error_response(500, {'error': f'Server error: {str(e)}'})
    
    def _build_start_json(self, channel_name=None, user_uid=None):
        """Build start JSON request body (equivalent to _build_start_json() in C code)"""
        config = load_tenai_config()
        
        request_id = str(uuid.uuid4())
        # Use channel_name from parameter, fallback to config if not provided
        if channel_name is None:
            channel_name = config.get('ai_agent_channel_name', 'default_channel')
        # Use user_uid from parameter, fallback to config if not provided
        if user_uid is None:
            user_uid = config.get('ai_agent_user_id', 1)
        graph_name = config.get('graph_name', 'va_openai_azure')
        tenai_audio_codec = config.get('tenai_audio_codec', '{"che.audio.custom_payload_type":9}')
        greeting = config.get('greeting', 'Hello')
        prompt = config.get('prompt', 'You are a helpful assistant.')
        language = config.get('language', 'en-US')
        voice_type = config.get('voice_type', 'male')
        
        # Build JSON structure
        root = {
            'request_id': request_id,
            'channel_name': channel_name,
            'user_uid': user_uid,
            'graph_name': graph_name,
            'greeting': greeting,
            'prompt': prompt,
            'language': language,
            'voice_type': voice_type,
            'properties': {
                'agora_rtc': {
                    'sdk_params': tenai_audio_codec
                }
            }
        }
        
        return json.dumps(root)

    def handle_ten_agent_generate_request(self, uid=None, channel_name=None):
        """Generate AI agent information - Client function (equivalent to ai_agent_generate() in C code)"""
        # Build URL and request body
        generate_url = _build_tenai_url("token/generate")
        request = self._build_generate_json(uid, channel_name)
        
        # Print request (equivalent to printf in C code)
        print(f"http_with_url={generate_url} request ={request}")
        
        try:
            # Send HTTP POST request
            response = requests.post(generate_url, headers=JSON_HEADERS, data=request)
            
            if response.status_code == 200:
                print(f"HTTPS Status = {response.status_code}, content_length = {len(response.content)}")
                # Return response data instead of sending directly
                return response.json()
            else:
                print(f"HTTPS Status = {response.status_code}, content_length = {len(response.content)}")
                # Raise exception to be handled by caller
                raise Exception(f'TenAI API request failed with status {response.status_code}')
                    
        except requests.exceptions.RequestException as e:
            print(f"Failed to open HTTP connection: {str(e)}")
            raise
        except Exception as e:
            print(f"Error perform http request {str(e)}")
            raise
    
    def _build_generate_json(self, uid, channel_name=None):
        """Build generate JSON request body (equivalent to _build_generate_json() in C code)"""
        config = load_tenai_config()
        
        # Get values from config or use defaults
        request_id = str(uuid.uuid4())
        # Use uid from parameter, fallback to config if not provided
        if uid is None:
            uid = config.get('ai_agent_user_id', 1)
        # Use channel_name from parameter, fallback to config if not provided
        if channel_name is None:
            channel_name = config.get('ai_agent_channel_name', 'default_channel')
        
        # Build JSON structure matching C code: request_id, uid, channel_name
        root = {
            'request_id': request_id,
            'uid': uid,
            'channel_name': channel_name
        }
        
        return json.dumps(root)

    def send_success_response(self, data):
        """Send success response"""
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def send_error_response(self, status_code, error_data):
        """Send error response"""
        self.send_response(status_code)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(error_data).encode())

def run(server_class=HTTPServer, handler_class=RequestHandler, port=5001):
    """Start HTTP server"""
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    logger.info(f'Starting server on port {port}...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()
