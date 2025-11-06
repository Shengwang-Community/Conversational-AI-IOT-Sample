#pragma once

typedef enum
{
    EMOTION_HAPPY = 0,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_SURPRISED,
    EMOTION_NEUTRAL,
    EMOTION_THINKING,
    EMOTION_SLEEPY,
    EMOTION_LOVING,
    EMOTION_CURIOUS,
} app_emotion_t;

typedef enum
{
    APP_EVT_ASR_WAKEUP = 0,
    APP_EVT_ASR_STANDBY,
    APP_EVT_IR_MODE_SWITCH,
    APP_EVT_NETWORK_PROVISIONING,
    APP_EVT_NETWORK_PROVISIONING_SUCCESS,
    APP_EVT_NETWORK_PROVISIONING_FAIL,
    APP_EVT_RECONNECT_NETWORK,
    APP_EVT_RECONNECT_NETWORK_SUCCESS,
    APP_EVT_RECONNECT_NETWORK_FAIL,
    APP_EVT_RTC_REJOIN_SUCCESS,
    APP_EVT_RTC_CONNECTION_LOST,
    APP_EVT_AGENT_JOINED,
    APP_EVT_AGENT_OFFLINE,
    APP_EVT_AGENT_START_FAIL,
    APP_EVT_AGENT_DEVICE_REMOVE,
    APP_EVT_CLOSE_BLUETOOTH,
    APP_EVT_LOW_VOLTAGE,
    APP_EVT_CHARGING,
    APP_EVT_SHUTDOWN_LOW_BATTERY,
    APP_EVT_OTA_START,
    APP_EVT_OTA_SUCCESS,
    APP_EVT_OTA_FAIL,
    APP_EVT_SYNC_FLASH,

    APP_EVT_POWER_ON,

    APP_EVT_CONVOAI_OTA_CHECK,
    APP_EVT_CONVOAI_CONFIG_LOADING,
    APP_EVT_CONVOAI_START_TIMER_EXPIRE,
    APP_EVT_CONVOAI_EXIT,
    APP_EVT_CONVOAI_CHANGE_LVGL_RESOURCE,
    APP_EVT_SMART_CONFIG_START,
} app_evt_type_t;

static inline const char *app_emotion_2_avi_file(app_emotion_t emotion)
{
   switch (emotion) {
        case EMOTION_HAPPY:
            return "/happy.avi";
        case EMOTION_SAD:
            return "/sad.avi";
        case EMOTION_ANGRY:
            return "/angry.avi";
        case EMOTION_SURPRISED:
            return "/surprise.avi";
        case EMOTION_NEUTRAL:
            return "/neutral.avi";
        case EMOTION_THINKING:
            return "/thinking.avi";
        case EMOTION_SLEEPY:
            return "/sleepy.avi";
        case EMOTION_LOVING:
            return "/love.avi";
        case EMOTION_CURIOUS:
            return "/curious.avi";
        default:
            break;
    }

    return "/neutral.avi";
}

void app_event_init(void);
bk_err_t app_event_send_msg(uint32_t event, uint32_t param);

typedef struct
{
    beken_thread_t thread;
    beken_queue_t queue;
} app_evt_info_t;

typedef struct
{
    app_evt_type_t event;
    uint32_t param;
} app_evt_msg_t;

typedef void (*app_event_callback_t)(app_evt_msg_t *msg, void *user_data);
typedef struct app_event_handler {
    app_evt_type_t        event_type;
    app_event_callback_t    callback;
    void                    *user_data;
    struct app_event_handler *next;
} app_event_handler_t;


int app_event_register_handler(app_evt_type_t event_type, 
                              app_event_callback_t callback,
                              void *user_data);
