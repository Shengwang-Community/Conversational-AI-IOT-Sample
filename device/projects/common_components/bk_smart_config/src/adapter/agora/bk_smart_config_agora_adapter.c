// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <common/sys_config.h>
#include <components/log.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include "components/webclient.h"
#include "cJSON.h"
#include "components/bk_uid.h"
#include "bk_genie_comm.h"
#include "bk_smart_config_agora_adapter.h"
#include "bk_smart_config.h"
#include "bk_factory_config.h"
#include "driver/trng.h"
#include "app_event.h"
#include "media_app.h"
#include "agora_config.h"
#include "video_engine.h"
#include "wifi_boarding_utils.h"

#define TAG "bk_sconf_agora"
char *app_id_record = NULL;
char *channel_name_record = NULL;

static void __emotion_ctrl(const char *emotion_type)
{
    int value = -1;

    do {
        if (0 == strcmp(emotion_type, "happy")) {
            value = EMOTION_HAPPY;
            BK_LOGW(TAG, "emotion_type happy.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "sad")) {
            value = EMOTION_SAD;
            BK_LOGW(TAG, "emotion_type sad.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "angry")) {
            value = EMOTION_ANGRY;
            BK_LOGW(TAG, "emotion_type angry.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "surprised")) {
            value = EMOTION_SURPRISED;
            BK_LOGW(TAG, "emotion_type surprised.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "neutral")) {
            value = EMOTION_NEUTRAL;
            BK_LOGW(TAG, "emotion_type neutral.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "thinking")) {
            value = EMOTION_THINKING;
            BK_LOGW(TAG, "emotion_type thinking.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "sleepy")) {
            value = EMOTION_SLEEPY;
            BK_LOGW(TAG, "emotion_type sleepy.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "loving")) {
            value = EMOTION_LOVING;
            BK_LOGW(TAG, "emotion_type loving.\n");
            break;
        }

        if (0 == strcmp(emotion_type, "curious")) {
            value = EMOTION_CURIOUS;
            BK_LOGW(TAG, "emotion_type curious.\n");
            break;
        }

        BK_LOGW(TAG, "unsupport emotion cmd. op=%s\n", emotion_type);
    } while (0);

    if (-1 != value) {
        app_event_send_msg(APP_EVT_CONVOAI_CHANGE_LVGL_RESOURCE, value);
    }
}

static void __device_control(char *msg_json_str)
{
    cJSON *root = NULL, *action_type = NULL, *emotion_type = NULL;
    bool is_device_control = false;
    bool is_display_emotion = false;

    if (NULL == (root = cJSON_Parse(msg_json_str))) {
        BK_LOGW(TAG, "msg not json format.\n");
        goto L_END;
    }

    if (NULL == (action_type = cJSON_GetObjectItem(root, "action_type"))) {
        BK_LOGW(TAG, "msg format invalid. action_type not found\n");
        goto L_END;
    }

    if ((action_type->type & 0xFF) != cJSON_String) {
        BK_LOGW(TAG, "msg format invalid. action_type not string\n");
        goto L_END;
    }

    BK_LOGW(TAG, "action_type=%s\n", action_type->valuestring);
    is_device_control = (0 == strcmp(action_type->valuestring, "control_device"));
    is_display_emotion = (0 == strcmp(action_type->valuestring, "display_emotion"));
    if (!is_device_control && !is_display_emotion) {
        BK_LOGW(TAG, "action type=%s, skip it\n", action_type->valuestring);
        goto L_END;
    }

    if (is_display_emotion) {
        if (NULL == (emotion_type = cJSON_GetObjectItem(root, "emotion_type"))) {
            BK_LOGW(TAG, "msg format invalid. emotion_type not found\n");
            goto L_END;
        }

        if ((emotion_type->type & 0xFF) != cJSON_String) {
            BK_LOGW(TAG, "msg format invalid. emotion_type not string\n");
            goto L_END;
        }

        __emotion_ctrl(emotion_type->valuestring);
    }

L_END:
    if (root) {
        cJSON_Delete(root);
        root = NULL;
    }
}

static void data_stream_msg_dispatch(char *msg_json_str)
{
    cJSON *root = NULL, *object = NULL, *content = NULL;
    if (NULL == (root = cJSON_Parse(msg_json_str))) {
        BK_LOGW(TAG, "msg not json format.\n");
        goto L_END;
    }

    if (NULL == (object = cJSON_GetObjectItem(root, "object"))) {
        BK_LOGW(TAG, "msg format invalid. object not found\n");
        goto L_END;
    }

    if ((object->type & 0xFF) != cJSON_String) {
        BK_LOGW(TAG, "msg format invalid. object not string\n");
        goto L_END;
    }

    if (0 != strcmp(object->valuestring, "message.user")) {
        BK_LOGW(TAG, "msg not user data type=%s. just skip it\n", object->valuestring);
        goto L_END;
    }

    if (NULL == (content = cJSON_GetObjectItem(root, "content"))) {
        BK_LOGW(TAG, "msg format invalid. content not found\n");
        goto L_END;
    }

    if ((content->type & 0xFF) != cJSON_String) {
        BK_LOGW(TAG, "msg format invalid. content not string\n");
        goto L_END;
    }

    BK_LOGW(TAG, "msg content=%s\n", content->valuestring);
    __device_control(content->valuestring);

L_END:
    if (root) {
        cJSON_Delete(root);
        root = NULL;
    }
}

#if CONFIG_ENABLE_AGORA_DATASTREAM
#include "base_64.h"
#define CONFIG_DATASTREAM_TASK_PRIORITY 4
static beken_thread_t datastream_thread_handle = NULL;
beken_queue_t datastream_queue = NULL;
#define MAX_DATASTREAM_SPLIT 4
#define MAX_DATASTREAM_LEN 1024*8
void parse_data_stream_main()
{
    char *save_ptr = NULL, *store_str = NULL, *msg_payload = NULL, *decode_str = NULL;
    const char *delim = "|";
    char *msg_id = NULL, *last_msg_id = NULL, *cur_index_str = NULL, *total_num_str = NULL;
    uint8_t cur_index = 0, total_num = 0, store_cur_index = 0, store_total_num = 0;
    int  remaining_len = 0;
    __maybe_unused int ret = 0, decode_len;
    bk_agora_ai_data_stream_t msg;

    while (1) {
        ret = rtos_pop_from_queue(&datastream_queue, &msg, BEKEN_WAIT_FOREVER);
        //BK_LOGW(TAG, "%s%d: msg=%s\n", __FUNCTION__, __LINE__, msg.data);
        //message id
        msg_id = strtok_r(msg.data, delim, &save_ptr);
        cur_index_str = strtok_r(NULL, delim, &save_ptr);
        total_num_str = strtok_r(NULL, delim, &save_ptr);
        //pkt index
        cur_index = os_strtoul(cur_index_str, NULL, 10);
        //total pkt num
        total_num = os_strtoul(total_num_str, NULL, 10);
        //message content
        msg_payload = strtok_r(NULL, delim, &save_ptr);
        if (!last_msg_id) {
            last_msg_id = os_strdup(msg_id);
        } else {
            if (os_strcmp(msg_id, last_msg_id)) {
                os_free(last_msg_id);
                last_msg_id = os_strdup(msg_id);
            }
        }
        if (!last_msg_id) {
            BK_LOGI(TAG,"OOM!\r\n");
            goto new_msg_loop;
        }
        store_cur_index = cur_index;
        store_total_num = total_num;
        if (store_total_num > MAX_DATASTREAM_SPLIT)
            goto new_msg_loop;
        if (store_cur_index < 1 ||store_cur_index > store_total_num)
            goto new_msg_loop;

        //check decode string
        if (!decode_str)
            decode_str = psram_zalloc(1025*total_num);
        if (store_cur_index == 1 && decode_str) {
            os_free(decode_str);
            decode_str = psram_zalloc(1025*total_num);
        }
        if (!decode_str) {
            BK_LOGI(TAG,"OOM!\r\n");
            goto new_msg_loop;
        }

        //check store string and store
        if (!store_str) {
            store_str = psram_zalloc(MAX_DATASTREAM_LEN+1);
            if (!store_str) {
                BK_LOGI(TAG,"OOM!\r\n");
                goto new_msg_loop;
            }
        }

        remaining_len = MAX_DATASTREAM_LEN - os_strlen(store_str);
        os_snprintf(store_str+os_strlen(store_str), remaining_len, "%s", msg_payload);
        //decode data stream
        if (store_cur_index == store_total_num) {
            BK_LOGI(TAG,"enc_data: %s\r\n", store_str);
            base64_decode((unsigned char *)store_str, os_strlen(store_str), &decode_len, (unsigned char *)decode_str);
            BK_LOGI(TAG,"dec_data: %s\r\n", decode_str);
            data_stream_msg_dispatch(decode_str);
            //for customer further development
            goto new_msg_loop;
        }
        psram_free(msg.data);
        continue;
new_msg_loop:
        psram_free(msg.data);
        if (last_msg_id) {
            os_free(last_msg_id);
            last_msg_id = NULL;
        }
        if (decode_str) {
            os_free(decode_str);
            decode_str = NULL;
        }
        if (store_str) {
            os_free(store_str);
            store_str = NULL;
        }
    }
}

int bk_sconf_init_datastream_resource()
{
    int ret = 0;

    ret = rtos_init_queue(&datastream_queue,
							 "datastream_queue",
							 sizeof(char *),
							 4);

#if CONFIG_PSRAM_AS_SYS_MEMORY
    ret = rtos_create_psram_thread(&datastream_thread_handle,
                                CONFIG_DATASTREAM_TASK_PRIORITY,
                                "parse_data_stream",
                                (beken_thread_function_t)parse_data_stream_main,
                                4096,
                                (beken_thread_arg_t)0);
#else
    ret = rtos_create_thread(&datastream_thread_handle,
                                CONFIG_DATASTREAM_TASK_PRIORITY,
                                "parse_data_stream",
                                (beken_thread_function_t)parse_data_stream_main,
                                4096,
                                (beken_thread_arg_t)0);
#endif

    return ret;
}
#endif

//image recognition mode switch
#define CONFIG_IR_MODE_SWITCH_TASK_PRIORITY 4
extern bk_err_t video_turn_on(void);
extern bk_err_t video_turn_off(void);

extern void lvgl_app_deinit(void);
void agora_ir_mode_config(bool enable)
{
    if (enable) {
        BK_LOGW(TAG, "ir switch on\n");
        video_turn_on();
        media_app_lvgl_switch_ui(LVGL_UI_DISP_IN_TEXT_AND_IMAGE);
    } else {
        BK_LOGW(TAG, "ir switch off\n");
        video_turn_off();
        media_app_lvgl_switch_ui(LVGL_UI_DISP_IN_INIT);
        lvgl_app_deinit();
    }
}

int bk_sconf_post_nfc_id(uint8_t *nfc_id)
{
    return 0;
}

extern void agora_convoai_engine_stop();
extern void lvgl_app_deinit();
void bk_sconf_trans_stop(void)
{
#if (CONFIG_DUAL_SCREEN_AVI_PLAY || CONFIG_SINGLE_SCREEN_AVI_PLAY || CONFIG_SINGLE_SCREEN_FONT_DISPLAY)
    lvgl_app_deinit();
#endif
    agora_convoai_engine_stop();
}