#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include "components/webclient.h"
#include "components/bk_uid.h"
#include "bk_genie_comm.h"
#include "cJSON.h"
#include "bk_ef.h"

#include "agora_convoai_iot.h"
#include "agora_config.h"

#define TAG "convoai"

#define LOGI(format, ...) BK_LOGW(TAG, format "\n", ##__VA_ARGS__)
#define LOGE(format, ...) BK_LOGE(TAG, format "\n", ##__VA_ARGS__)
#define LOGW(format, ...) BK_LOGW(TAG, format "\n", ##__VA_ARGS__)
#define LOGD(format, ...) BK_LOGD(TAG, format "\n", ##__VA_ARGS__)

#define AGORA_CONVOAI_AI_TOKEN_ID   "d_ai_token_id"
#define AGORA_CONVOAI_AI_SERVER_URL "d_ai_server_url"

static void __get_convoai_config_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  snprintf(url, AGORA_CONVOAI_SERVER_URL_SIZE, "%s%s", CONFIG_AGENT_SERVER_URL, "/device");
}

static void __get_convoai_start_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  snprintf(url, AGORA_CONVOAI_SERVER_URL_SIZE, "%s%s", CONFIG_AGENT_SERVER_URL, "/agent/start");
}

static void __get_convoai_stop_url(char url[AGORA_CONVOAI_SERVER_URL_SIZE])
{
  snprintf(url, AGORA_CONVOAI_SERVER_URL_SIZE, "%s%s", CONFIG_AGENT_SERVER_URL, "/agent/stop");
}

static int __https_post_request(const char *request_url, const char *post_body, int post_body_len, char *resp_buffer, int resp_buffer_len)
{
  struct webclient_session* session;
  int err = -1, bytes_read;

  if (NULL == (session = webclient_session_create(4096))) {
    LOGE("webclient session create failed.");
    return -1;
  }

  LOGI("web post URL=%s", request_url);
  LOGI("web post body=%s", post_body);
  webclient_header_fields_add(session, "Content-Length: %d\r\n", post_body_len);
  webclient_header_fields_add(session, "Content-Type: application/json\r\n");
  err =  webclient_post(session, request_url, post_body, post_body_len);
  LOGI("webclient post err=%d", err);

  do {
		bytes_read = webclient_read(session, resp_buffer, resp_buffer_len);
		if (bytes_read > 0) {
			break;
		}
	} while (1);

  webclient_close(session);
  return err == 200 ? 0 : err;
}

agora_convoai_configs_resp_t* agora_convoai_configs_get(agora_convoai_configs_param_t *config_param)
{
  int len, err = -1;
  cJSON *root = NULL, *app_id = NULL, *rtc_token = NULL;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  agora_convoai_configs_resp_t *config = NULL;

  if (NULL == (request_body = psram_malloc(1024))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(4096))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_config_url(request_url);
  len = os_snprintf(request_body, 1024, "{\"channel_name\": \"%s\", \"uid\": %d}",
                    config_param->channel_name,
                    config_param->local_uid);
  err = __https_post_request(request_url, request_body, len, resp_body, 4096);
  if (err < 0) {
    LOGE("convoai get configs failed. err=%d", err);
    goto L_EXIT;
  }

  err = -1;
  LOGI("convoai get configs resp=%s", resp_body);

  if (NULL == (root = cJSON_Parse(resp_body))) {
    LOGE("convoai get configs resp format invalid. not json");
    goto L_EXIT;
  }

  if (NULL == (app_id = cJSON_GetObjectItem(root, "app_id"))) {
    LOGE("convoai get configs resp format invalid. app_id not found");
    goto L_EXIT;
  }

  if ((app_id->type & 0xFF) != cJSON_String) {
    LOGE("convoai get configs resp format invalid. app_id not string");
    goto L_EXIT;
  }

  if (NULL == (rtc_token = cJSON_GetObjectItem(root, "token"))) {
    LOGE("convoai get configs resp format invalid. rtc_token not found");
    goto L_EXIT;
  }

  if ((rtc_token->type & 0xFF) != cJSON_String) {
    LOGE("convoai get configs resp format invalid. rtc_token not string");
    goto L_EXIT;
  }

  if (NULL == (config = psram_malloc(sizeof(agora_convoai_configs_resp_t)))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  os_snprintf(config->app_id, sizeof(config->app_id), "%s", app_id->valuestring);
  os_snprintf(config->rtc_token, sizeof(config->rtc_token), "%s", rtc_token->valuestring);
  os_snprintf(config->channel_name, sizeof(config->channel_name), "%s", config_param->channel_name);
  config->local_uid = config_param->local_uid;
  err = 0;

L_EXIT:
  if (request_body) psram_free(request_body);
  if (resp_body) psram_free(resp_body);
  if (request_url) psram_free(request_url);
  if (root) cJSON_Delete(root);

  return 0 == err ? config : NULL;
}

agora_convoai_start_resp_t* agora_convoai_start(agora_convoai_start_param_t *start_param)
{
  int len, err = -1;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  cJSON *root = NULL, *agent_id = NULL;
  agora_convoai_start_resp_t *start_rsp = NULL;

  if (NULL == (request_body = psram_malloc(1024))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(4096))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_start_url(request_url);
  len = os_snprintf(request_body, 1024, "{\"channel_name\": \"%s\", \"agent_uid\": \"%d\", \"uid\": \"%d\"}",
                    start_param->channel_name,
                    start_param->agent_uid,
                    start_param->local_uid);
  err = __https_post_request(request_url, request_body, len, resp_body, 4096);
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

  if (NULL == (agent_id = cJSON_GetObjectItem(root, "agent_id"))) {
    LOGE("convoai start resp format invalid. app_id not found");
    goto L_EXIT;
  }

  if ((agent_id->type & 0xFF) != cJSON_String) {
    LOGE("convoai start resp format invalid. app_id not string");
    goto L_EXIT;
  }

  if (NULL == (start_rsp = psram_malloc(sizeof(agora_convoai_start_resp_t)))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  os_snprintf(start_rsp->agent_id, sizeof(start_rsp->agent_id), agent_id->valuestring);
  err = 0;

L_EXIT:
  if (request_body) psram_free(request_body);
  if (resp_body) psram_free(resp_body);
  if (request_url) psram_free(request_url);
  if (root) cJSON_Delete(root);
  return 0 == err ? start_rsp : NULL;
}

int agora_convoai_stop(agora_convoai_stop_param_t *stop_param)
{
  int len, err = -1;
  char *request_body = NULL;
  char *resp_body = NULL;
  char *request_url = NULL;
  cJSON *root = NULL;

  if (NULL == (request_body = psram_malloc(1024))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (resp_body = psram_malloc(4096))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  if (NULL == (request_url = psram_malloc(AGORA_CONVOAI_SERVER_URL_SIZE))) {
    LOGE("alloc memory failed.");
    goto L_EXIT;
  }

  __get_convoai_stop_url(request_url);
  len = os_snprintf(request_body, 1024, "{\"agent_id\": \"%s\"}",
                    stop_param->agent_id);
  err = __https_post_request(request_url, request_body, len, resp_body, 4096);
  if (err < 0) {
    LOGE("convoai stop failed. err=%d", err);
    goto L_EXIT;
  }

  err = -1;
  LOGI("convoai stop resp=%s", resp_body);

  if (NULL == (root = cJSON_Parse(resp_body))) {
    LOGE("convoai stop resp format invalid. not json");
    goto L_EXIT;
  }

  err = 0;
L_EXIT:

  if (request_body) psram_free(request_body);
  if (resp_body) psram_free(resp_body);
  if (request_url) psram_free(request_url);
  if (root) cJSON_Delete(root);
  return err;
}

void agora_convoai_get_device_id(char device_id[AGORA_CONVOAI_DEVICE_ID_SIZE])
{
  unsigned char uuid[32] = {0};
  bk_uid_get_data(uuid);
  for (int i = 0; i < 24; i++) {
      sprintf(device_id + i * 2, "%02x", uuid[i]);
  }
}
