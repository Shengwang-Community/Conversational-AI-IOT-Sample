#pragma once

#include <stddef.h>
#include "agora_config.h"

#define AGORA_CONVOAI_REQUEST_ID_SIZE    32 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_DEVICE_ID_SIZE     64 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_AGENT_ID_SIZE      64 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_CHANNEL_NAME_SIZE  64 + 1  //contain c string tail '\0'
#define AGORA_CONVOAI_AGENT_URL_SIZE     256 + 1 //contain c string tail '\0'
#define AGORA_CONVOAI_REQUEST_TOKEN_SIZE 512
#define AGORA_CONVOAI_SERVER_URL_SIZE    256

typedef struct {
  char channel_name[AGORA_CONVOAI_DEVICE_ID_SIZE];
  int local_uid;
} agora_convoai_configs_param_t;

typedef struct {
  char app_id[AGORA_CONVOAI_AGENT_ID_SIZE];
  char rtc_token[AGORA_CONVOAI_REQUEST_TOKEN_SIZE];
  char channel_name[AGORA_CONVOAI_DEVICE_ID_SIZE];
  int local_uid;
} agora_convoai_configs_resp_t;

typedef struct {
  char channel_name[AGORA_CONVOAI_DEVICE_ID_SIZE];
  int local_uid;
  int agent_uid;
} agora_convoai_start_param_t;

typedef struct {
  char agent_id[AGORA_CONVOAI_AGENT_ID_SIZE];
} agora_convoai_start_resp_t;

typedef struct {
  char agent_id[AGORA_CONVOAI_AGENT_ID_SIZE];
} agora_convoai_stop_param_t;

agora_convoai_configs_resp_t* agora_convoai_configs_get(agora_convoai_configs_param_t *config_param);
agora_convoai_start_resp_t* agora_convoai_start(agora_convoai_start_param_t *start_param);
int agora_convoai_stop(agora_convoai_stop_param_t *stop_param);

void agora_convoai_get_device_id(char device_id[AGORA_CONVOAI_DEVICE_ID_SIZE]);
