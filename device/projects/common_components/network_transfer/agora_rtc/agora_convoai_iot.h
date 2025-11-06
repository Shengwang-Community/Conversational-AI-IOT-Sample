#ifndef __AGORA_CONVOAI_IOT_H__
#define __AGORA_CONVOAI_IOT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define AGORA_CONVOAI_REQUEST_ID_SIZE       32 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_DEVICE_ID_SIZE        64 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_CONVERSATION_ID_SIZE  64 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_CHANNEL_NAME_SIZE     64 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_AGENT_URL_SIZE        256 + 1 //contain c string tail '\0'
#define AGORA_CONVOAI_REQUEST_TOKEN_SIZE    512
#define AGORA_CONVOAI_SERVER_URL_SIZE       256
#define AGORA_CONVOAI_OTA_URL_SIZE          256
#define AGORA_CONVOAI_OTA_FIRMWARE_ID_SIZE  256

typedef struct {
  char channel_name[AGORA_CONVOAI_DEVICE_ID_SIZE];
  int local_uid;
} agora_convoai_configs_param_t;

typedef struct {
  char app_id[33];
  char rtc_token[512];
  bool token_enable;
  uint32_t timestamp;
} agora_convoai_configs_resp_t;

typedef struct {
  char channel_name[AGORA_CONVOAI_DEVICE_ID_SIZE];
  int local_uid;
  int agent_uid;
} agora_convoai_start_param_t;

typedef struct {
  char conversation_id[AGORA_CONVOAI_CONVERSATION_ID_SIZE];
} agora_convoai_start_resp_t;

typedef struct {
  char conversation_id[AGORA_CONVOAI_CONVERSATION_ID_SIZE];
} agora_convoai_stop_param_t;

typedef struct {
  char firmware_id[AGORA_CONVOAI_OTA_FIRMWARE_ID_SIZE];
  char url[AGORA_CONVOAI_OTA_URL_SIZE];
} agora_convoai_ota_version_t;

typedef struct {
  char firmware_id[AGORA_CONVOAI_OTA_FIRMWARE_ID_SIZE];
  bool is_install_success;
} agora_convoai_ota_result_report_t;

typedef struct {
  char firmware_id[AGORA_CONVOAI_OTA_FIRMWARE_ID_SIZE];
  int8_t major;
  int8_t minor;
  int8_t patch;
} agora_convoai_ota_info_t;

void agora_convoai_ota_info_persistence_write(agora_convoai_ota_info_t *ota_info);
void agora_convoai_ota_info_persistence_read(agora_convoai_ota_info_t *ota_info);

agora_convoai_ota_version_t* agora_convoai_ota_version_get(void);
int agora_convoai_ota_result_report(agora_convoai_ota_result_report_t *ota_result);

agora_convoai_configs_resp_t* agora_convoai_configs_get(agora_convoai_configs_param_t *config_param);
agora_convoai_start_resp_t* agora_convoai_start(agora_convoai_start_param_t *start_param);
int agora_convoai_stop(agora_convoai_stop_param_t *stop_param);

void agora_convoai_request_token_persistence_write(char token[AGORA_CONVOAI_REQUEST_TOKEN_SIZE]);
void agora_convoai_request_token_persistence_read(char token[AGORA_CONVOAI_REQUEST_TOKEN_SIZE]);
void agora_convoai_server_url_write(char url[AGORA_CONVOAI_SERVER_URL_SIZE]);
void agora_convoai_server_url_read(char url[AGORA_CONVOAI_SERVER_URL_SIZE]);

void agora_convoai_get_uuid(char buf[AGORA_CONVOAI_REQUEST_ID_SIZE]);
void agora_convoai_get_device_id(char device_id[AGORA_CONVOAI_DEVICE_ID_SIZE]);
void agora_convoai_get_channel_name(char channel_name[AGORA_CONVOAI_CHANNEL_NAME_SIZE]);

#ifdef __cplusplus
}
#endif
#endif