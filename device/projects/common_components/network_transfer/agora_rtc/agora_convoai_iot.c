#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include "components/webclient.h"
#include "components/bk_uid.h"
#include "bk_genie_comm.h"
#include "cJSON.h"
#include "bk_ef.h"
#include "agora_config.h"

#include "agora_convoai_iot.h"

#define TAG "convoai"

#define LOGI(format, ...) BK_LOGW(TAG, format "\n", ##__VA_ARGS__)
#define LOGE(format, ...) BK_LOGE(TAG, format "\n", ##__VA_ARGS__)
#define LOGW(format, ...) BK_LOGW(TAG, format "\n", ##__VA_ARGS__)
#define LOGD(format, ...) BK_LOGD(TAG, format "\n", ##__VA_ARGS__)

#define AGORA_CONVOAI_AI_TOKEN_ID   "d_ai_token_id"
#define AGORA_CONVOAI_AI_SERVER_URL "d_ai_server_url"
#define AGORA_CONVOAI_AI_OTA_INF    "d_ai_ota_info"

#define HTTP_REQ_BODY_SIZE  1024
#define HTTP_RSP_BODY_SIZE  4096

#define AGORA_CODE_OK "200"

static void __get_convoai_config_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  agora_convoai_server_url_read(url);
  int len = strlen(url);
  snprintf(url + len, AGORA_CONVOAI_SERVER_URL_SIZE - len, "%s", "/device");
}

static void __get_convoai_start_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  agora_convoai_server_url_read(url);
  int len = strlen(url);
  snprintf(url + len, AGORA_CONVOAI_SERVER_URL_SIZE - len, "%s", "/agent/start");
}

static void __get_convoai_stop_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  agora_convoai_server_url_read(url);
  int len = strlen(url);
  snprintf(url + len, AGORA_CONVOAI_SERVER_URL_SIZE - len, "%s", "/agent/stop");
}

static void __get_convoai_ota_get_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  agora_convoai_server_url_read(url);
  int len = strlen(url);
  snprintf(url + len, AGORA_CONVOAI_SERVER_URL_SIZE - len, "%s", "/firmware/update");
}

static void __get_convoai_ota_result_report_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  agora_convoai_server_url_read(url);
  int len = strlen(url);
  snprintf(url + len, AGORA_CONVOAI_SERVER_URL_SIZE - len, "%s", "/firmware/install-result");
}

static int __https_get_request(const char *request_url, char *resp_buffer, int resp_buffer_len)
{
  struct webclient_session* session = NULL;
  char *convoai_request_token = NULL;
  int err = -1, bytes_read;

  if (NULL == (convoai_request_token = psram_malloc(AGORA_CONVOAI_REQUEST_TOKEN_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  os_memset(convoai_request_token, 0, AGORA_CONVOAI_REQUEST_TOKEN_SIZE);
  agora_convoai_request_token_persistence_read(convoai_request_token);
  if (convoai_request_token[0] == '\0') {
    LOGE("convoai_request_token invalid.");
    goto L_EXIT;
  }

  if (NULL == (session = webclient_session_create(HTTP_RSP_BODY_SIZE))) {
    LOGE("webclient session create failed.");
    goto L_EXIT;
  }

  LOGI("web get URL=%s", request_url);
  LOGI("web token=%s", convoai_request_token);
  webclient_header_fields_add(session, "Content-Type: application/json\r\n");
  webclient_header_fields_add(session, "Authorization: Bearer %s\r\n", convoai_request_token);

  err = webclient_get(session, request_url);
  LOGI("webclient get err=%d", err);
  if (err != 200) {
    goto L_EXIT;
  }

  do {
    bytes_read = webclient_read(session, resp_buffer, resp_buffer_len);
    if (bytes_read > 0) {
      resp_buffer[bytes_read] = '\0';
      break;
    }
  } while (1);

L_EXIT:
  if (session) {
    webclient_close(session);
    session = NULL;
  }

  if (convoai_request_token) {
    psram_free(convoai_request_token);
    convoai_request_token = NULL;
  }

  return err == 200 ? 0 : -1;
}

static int __https_post_request(const char *request_url, const char *post_body, int post_body_len, char *resp_buffer, int resp_buffer_len)
{
  struct webclient_session* session = NULL;
  char *convoai_request_token = NULL;
  int err = -1, bytes_read;

  if (NULL == (convoai_request_token = psram_malloc(AGORA_CONVOAI_REQUEST_TOKEN_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  os_memset(convoai_request_token, 0, AGORA_CONVOAI_REQUEST_TOKEN_SIZE);
  agora_convoai_request_token_persistence_read(convoai_request_token);
  if (convoai_request_token[0] == '\0') {
    LOGE("convoai_request_token invalid.");
    goto L_EXIT;
  }

  if (NULL == (session = webclient_session_create(HTTP_RSP_BODY_SIZE))) {
    LOGE("webclient session create failed.");
    goto L_EXIT;
  }

  LOGI("web post URL=%s", request_url);
  LOGI("web post body=%s", post_body);
  LOGI("web token=%s", convoai_request_token);
  webclient_header_fields_add(session, "Content-Length: %d\r\n", post_body_len);
  webclient_header_fields_add(session, "Content-Type: application/json\r\n");
  webclient_header_fields_add(session, "Authorization: Bearer %s\r\n", convoai_request_token);
  err = webclient_post(session, request_url, post_body, post_body_len);
  LOGI("webclient post err=%d", err);
  if (err != 200) {
    goto L_EXIT;
  }

  do {
    bytes_read = webclient_read(session, resp_buffer, resp_buffer_len);
    if (bytes_read > 0) {
      resp_buffer[bytes_read] = '\0';
      break;
    }
  } while (1);

L_EXIT:
  if (session) {
    webclient_close(session);
    session = NULL;
  }

  if (convoai_request_token) {
    psram_free(convoai_request_token);
    convoai_request_token = NULL;
  }

  return err == 200 ? 0 : -1;
}

agora_convoai_configs_resp_t* agora_convoai_configs_get(agora_convoai_configs_param_t *config_param)
{
  int len, err = -1;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  cJSON *root = NULL, *app_id = NULL, *token = NULL;
  agora_convoai_configs_resp_t *config = NULL;
  int try_cnt = 2;

  if (NULL == (request_body = psram_malloc(HTTP_REQ_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(HTTP_RSP_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_config_url(request_url);
  len = os_snprintf(request_body, HTTP_REQ_BODY_SIZE, "{\"channel_name\": \"%s\", \"uid\": %d}",
                    config_param->channel_name,
                    config_param->local_uid);

  while (--try_cnt >= 0) {
    err = __https_post_request(request_url, request_body, len, resp_body, HTTP_RSP_BODY_SIZE);
    if (err >= 0) break;
  }

  if (err < 0) {
    LOGE("convoai get configs failed. err=%d", err);
    goto L_EXIT;
  }

  err = -1;
  LOGI("convoai config resp=%s", resp_body);

  if (NULL == (root = cJSON_Parse(resp_body))) {
    LOGE("convoai start resp format invalid. not json");
    goto L_EXIT;
  }

  if (NULL == (app_id = cJSON_GetObjectItem(root, "app_id"))) {
    LOGE("convoai start resp format invalid. app_id not found");
    goto L_EXIT;
  }

  if ((app_id->type & 0xFF) != cJSON_String) {
    LOGE("convoai start resp format invalid. app_id not string");
    goto L_EXIT;
  }

  if (NULL == (token = cJSON_GetObjectItem(root, "token"))) {
    LOGE("convoai start resp format invalid. token not found");
    goto L_EXIT;
  }

  if ((token->type & 0xFF) != cJSON_String) {
    LOGE("convoai start resp format invalid. token not string");
    goto L_EXIT;
  }

  if (NULL == (config = psram_malloc(sizeof(agora_convoai_configs_resp_t)))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  os_snprintf(config->app_id, sizeof(config->app_id), "%s", app_id->valuestring);
  os_snprintf(config->rtc_token, sizeof(config->rtc_token), "%s", token->valuestring);
  config->token_enable = (config->rtc_token[0] != '\0' && 0 != strcmp(config->app_id, config->rtc_token));
  config->timestamp = rtos_get_time();
  err = 0;

  LOGI("convoai get config success. appid=%s, token=%s, token_enable=%d, ts=%u", config->app_id, config->rtc_token, config->token_enable, config->timestamp);

L_EXIT:
  if (request_body) {
    psram_free(request_body);
    request_body = NULL;
  }

  if (resp_body) {
    psram_free(resp_body);
    resp_body = NULL;
  }

  if (request_url) {
    psram_free(request_url);
    request_url = NULL;
  }

  return 0 == err ? config : NULL;
}

agora_convoai_start_resp_t* agora_convoai_start(agora_convoai_start_param_t *start_param)
{
  int len, err = -1;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  cJSON *root = NULL, *conversation_id = NULL;
  agora_convoai_start_resp_t *start_rsp = NULL;
  int try_cnt = 2;

  if (NULL == (request_body = psram_malloc(HTTP_REQ_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(HTTP_RSP_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_start_url(request_url);
  len = os_snprintf(request_body,
                    HTTP_REQ_BODY_SIZE, "{\"channel_name\": \"%s\", "
                                         "\"agent_uid\": %d, "
                                         "\"uid\": %d "
                                        "}",
                    start_param->channel_name,
                    start_param->agent_uid,
                    start_param->local_uid);

  while (--try_cnt >= 0) {
    err = __https_post_request(request_url, request_body, len, resp_body, HTTP_RSP_BODY_SIZE);
    if (err >= 0) break;
  }

  if (err < 0) {
    LOGE("convoai start failed. err=%d", err);
    goto L_EXIT;
  }

  err = -1;
  LOGI("convoai start resp=%s", resp_body);

  if (NULL == (root = cJSON_Parse(resp_body))) {
    LOGE("convoai start resp format invalid. not json");
    goto L_EXIT;
  }

  if (NULL == (conversation_id = cJSON_GetObjectItem(root, "agent_id"))) {
    LOGE("convoai start resp format invalid. agent_id not found");
    goto L_EXIT;
  }

  if ((conversation_id->type & 0xFF) != cJSON_String) {
    LOGE("convoai start resp format invalid. conversation_id not string");
    goto L_EXIT;
  }

  if (NULL == (start_rsp = psram_malloc(sizeof(agora_convoai_start_resp_t)))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  os_snprintf(start_rsp->conversation_id, sizeof(start_rsp->conversation_id), conversation_id->valuestring);
  err = 0;

  LOGI("convoai start success. conversation_id=%s, channel_name=%s, local_uid=%u, agent_uid=%u", start_rsp->conversation_id,
       start_param->channel_name, start_param->local_uid, start_param->agent_uid);

L_EXIT:
  if (request_body) {
    psram_free(request_body);
    request_body = NULL;
  }

  if (resp_body) {
    psram_free(resp_body);
    resp_body = NULL;
  }

  if (request_url) {
    psram_free(request_url);
    request_url = NULL;
  }

  if (root) {
    cJSON_Delete(root);
    root = NULL;
  }

  return 0 == err ? start_rsp : NULL;
}

int agora_convoai_stop(agora_convoai_stop_param_t *stop_param)
{
  int len, err = -1;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  int try_cnt = 2;

  if (NULL == (request_body = psram_malloc(HTTP_REQ_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(HTTP_RSP_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_stop_url(request_url);
  len = os_snprintf(request_body, HTTP_REQ_BODY_SIZE, "{\"agent_id\": \"%s\"}", stop_param->conversation_id);

  while (--try_cnt >= 0) {
    err = __https_post_request(request_url, request_body, len, resp_body, HTTP_RSP_BODY_SIZE);
    if (err >= 0) break;
  }

  if (err < 0) {
    LOGE("convoai stop failed. err=%d", err);
    goto L_EXIT;
  }

  LOGI("convoai stop success. conversation_id=%s", stop_param->conversation_id);

  err = 0;
L_EXIT:

  if (request_body) {
    psram_free(request_body);
    request_body = NULL;
  }

  if (resp_body) {
    psram_free(resp_body);
    resp_body = NULL;
  }

  if (request_url) {
    psram_free(request_url);
    request_url = NULL;
  }

  return err;
}

void agora_convoai_ota_info_persistence_write(agora_convoai_ota_info_t *ota_info)
{
  bk_set_env_enhance(AGORA_CONVOAI_AI_OTA_INF, ota_info, sizeof(agora_convoai_ota_info_t));
}

void agora_convoai_ota_info_persistence_read(agora_convoai_ota_info_t *ota_info)
{
  bk_get_env_enhance(AGORA_CONVOAI_AI_OTA_INF, ota_info, sizeof(agora_convoai_ota_info_t));
}

void agora_convoai_request_token_persistence_write(char token[AGORA_CONVOAI_REQUEST_TOKEN_SIZE])
{
  bk_set_env_enhance(AGORA_CONVOAI_AI_TOKEN_ID, token, AGORA_CONVOAI_REQUEST_TOKEN_SIZE);
}

void agora_convoai_request_token_persistence_read(char token[AGORA_CONVOAI_REQUEST_TOKEN_SIZE])
{
  bk_get_env_enhance(AGORA_CONVOAI_AI_TOKEN_ID, token, AGORA_CONVOAI_REQUEST_TOKEN_SIZE);
}

void agora_convoai_server_url_write(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  // TODO: enable it if you have send URL frome APP
  //bk_set_env_enhance(AGORA_CONVOAI_AI_SERVER_URL, url, AGORA_CONVOAI_SERVER_URL_SIZE);
}

void agora_convoai_server_url_read(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  //bk_get_env_enhance(AGORA_CONVOAI_AI_SERVER_URL, url, AGORA_CONVOAI_SERVER_URL_SIZE);
  strncpy(url, CONFIG_AGENT_SERVER_URL, AGORA_CONVOAI_SERVER_URL_SIZE);
}

static int32_t __device_id_encode(const char *src, int32_t src_len, char *encoded)
{
  const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789&#";
  int i;
  char *p = encoded;
  for (i = 0; i < src_len - 2; i += 3) {
    *p++ = basis_64[(src[i] >> 2) & 0x3F];
    *p++ = basis_64[((src[i] & 0x3) << 4) |
                    ((src[i + 1] & 0xF0) >> 4)];
    *p++ = basis_64[((src[i + 1] & 0xF) << 2) |
                    ((src[i + 2] & 0xC0) >> 6)];
    *p++ = basis_64[src[i + 2] & 0x3F];
  }
  if (i < src_len) {
    *p++ = basis_64[(src[i] >> 2) & 0x3F];
    if (i == (src_len - 1)) {
        *p++ = basis_64[((src[i] & 0x3) << 4)];
        *p++ = '@';
    } else {
        *p++ = basis_64[((src[i] & 0x3) << 4) |
                        ((src[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((src[i + 1] & 0xF) << 2)];
    }
    *p++ = '@';
  }

  *p++ = '\0';
  return (p - encoded);
}

void agora_convoai_get_device_id(char device_id[AGORA_CONVOAI_DEVICE_ID_SIZE])
{
  unsigned char uuid[32] = {0};
  bk_uid_get_data(uuid);
  __device_id_encode((char *)uuid, sizeof(uuid), device_id);
}

void agora_convoai_get_channel_name(char channel_name[AGORA_CONVOAI_CHANNEL_NAME_SIZE])
{
  char device_id[AGORA_CONVOAI_DEVICE_ID_SIZE] = {0};
  agora_convoai_get_device_id(device_id);
  snprintf(channel_name, AGORA_CONVOAI_CHANNEL_NAME_SIZE, "%sTS%llu", device_id, rtos_get_time());
}

agora_convoai_ota_version_t* agora_convoai_ota_version_get(void)
{
  int err = -1;
  int try_cnt = 2;
  char *resp_body = NULL;
  char *request_url = NULL;
  cJSON *root = NULL, *code = NULL, *msg = NULL, *data = NULL, *firmware_id = NULL, *url = NULL;
  agora_convoai_ota_version_t *ota_version = NULL;

  if (NULL == (resp_body = psram_malloc(HTTP_RSP_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_ota_get_url(request_url);

  while (--try_cnt >= 0) {
    err = __https_get_request(request_url, resp_body, HTTP_RSP_BODY_SIZE);
    if (err >= 0) break;
  }

  if (err < 0) {
    LOGE("convoai ota get failed. err=%d", err);
    goto L_EXIT;
  }

  LOGI("convoai ota resp=%s", resp_body);
  err = -1;

  if (NULL == (root = cJSON_Parse(resp_body))) {
    LOGE("convoai ota resp format invalid. not json");
    goto L_EXIT;
  }

  if (NULL == (code = cJSON_GetObjectItem(root, "code"))) {
    LOGE("convoai ota resp format invalid. code not found");
    goto L_EXIT;
  }

  if ((code->type & 0xFF) != cJSON_String) {
    LOGE("convoai ota resp format invalid. code not string");
    goto L_EXIT;
  }

  if (0 != strcmp(code->valuestring, AGORA_CODE_OK)) {
    LOGE("convoai ota resp failed. code=%s", code->valuestring);
    goto L_EXIT;
  }

  if (NULL == (msg = cJSON_GetObjectItem(root, "msg"))) {
    LOGE("convoai ota resp format invalid. msg not found");
  } else {
    if ((msg->type & 0xFF) != cJSON_String) {
      LOGE("convoai ota resp format invalid. msg not string");
    } else {
      LOGI("msg=%s", msg->valuestring);
    }
  }

  if (NULL == (data = cJSON_GetObjectItem(root, "data"))) {
    LOGE("convoai ota format invalid. data not found");
    goto L_EXIT;
  }

  if (NULL == (firmware_id = cJSON_GetObjectItem(data, "firmwareId"))) {
    LOGE("convoai ota format invalid. firmware_id not found");
    goto L_EXIT;
  }

  if ((firmware_id->type & 0xFF) != cJSON_String) {
    LOGE("convoai ota format invalid. firmware_id not cJSON_String");
    goto L_EXIT;
  }

  if (NULL == (url = cJSON_GetObjectItem(data, "url"))) {
    LOGE("convoai ota format invalid. url not found");
    goto L_EXIT;
  }

  if ((url->type & 0xFF) != cJSON_String) {
    LOGE("convoai ota format invalid. url not string");
    goto L_EXIT;
  }

  if (NULL == (ota_version = psram_malloc(sizeof(agora_convoai_ota_version_t)))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  snprintf(ota_version->url, sizeof(ota_version->url), "%s", url->valuestring);
  snprintf(ota_version->firmware_id, sizeof(ota_version->firmware_id), "%s", firmware_id->valuestring);
  LOGI("convoai ota version get success. firmware_id=%s, url=%s", ota_version->firmware_id, ota_version->url);

  err = 0;
L_EXIT:

  if (resp_body) {
    psram_free(resp_body);
    resp_body = NULL;
  }

  if (request_url) {
    psram_free(request_url);
    request_url = NULL;
  }

  if (root) {
    cJSON_Delete(root);
    root = NULL;
  }

  return 0 == err ? ota_version : NULL;
}

int agora_convoai_ota_result_report(agora_convoai_ota_result_report_t *ota_result)
{
  int len, err = -1;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  cJSON *root = NULL, *code = NULL, *msg = NULL;
  int try_cnt = 2;

  if (NULL == (request_body = psram_malloc(HTTP_REQ_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(HTTP_RSP_BODY_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_ota_result_report_url(request_url);
  len = os_snprintf(request_body, HTTP_REQ_BODY_SIZE, "{\"firmware_id\": \"%s\", "
                                                       "\"is_install_success\": %d "
                                                      "}",
                    ota_result->firmware_id,
                    ota_result->is_install_success);

  while (--try_cnt >= 0) {
    err = __https_post_request(request_url, request_body, len, resp_body, HTTP_RSP_BODY_SIZE);
    if (err >= 0) break;
  }

  if (err < 0) {
    LOGE("convoai ota result report failed. err=%d", err);
    goto L_EXIT;
  }

  LOGI("convoai ota result report resp=%s", resp_body);
  err = -1;

  if (NULL == (root = cJSON_Parse(resp_body))) {
    LOGE("convoai ota result report resp format invalid. not json");
    goto L_EXIT;
  }

  if (NULL == (code = cJSON_GetObjectItem(root, "code"))) {
    LOGE("convoai ota result report resp format invalid. code not found");
    goto L_EXIT;
  }

  if ((code->type & 0xFF) != cJSON_String) {
    LOGE("convoai ota result resp format invalid. code not string");
    goto L_EXIT;
  }

  if (0 != strcmp(code->valuestring, AGORA_CODE_OK)) {
    LOGE("convoai ota result resp failed. code=%s", code->valuestring);
    goto L_EXIT;
  }

  if (NULL == (msg = cJSON_GetObjectItem(root, "msg"))) {
    LOGE("convoai ota result report resp format invalid. msg not found");
  } else {
    if ((msg->type & 0xFF) != cJSON_String) {
      LOGE("convoai ota result report resp format invalid. msg not string");
    } else {
      LOGI("msg=%s", msg->valuestring);
    }
  }

  LOGI("convoai ota report success. firmware_id=%s, is_install_success=%d", ota_result->firmware_id, ota_result->is_install_success);

  err = 0;
L_EXIT:

  if (request_body) {
    psram_free(request_body);
    request_body = NULL;
  }

  if (resp_body) {
    psram_free(resp_body);
    resp_body = NULL;
  }

  if (request_url) {
    psram_free(request_url);
    request_url = NULL;
  }

  if (root) {
    cJSON_Delete(root);
    root = NULL;
  }

  return err;
}