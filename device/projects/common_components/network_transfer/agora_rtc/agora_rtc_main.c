#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/shell_task.h>
#include <components/event.h>
#include <components/netif_types.h>
#include "bk_rtos_debug.h"
#include "audio_config.h"
#include "agora_rtc.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#include <driver/media_types.h>
#include <driver/lcd.h>
#include <modules/wifi.h>
#include "modules/wifi_types.h"
#include "media_app.h"
#include "lcd_act.h"
#include "components/bk_uid.h"
#if CONFIG_BK_SMART_CONFIG
#include "bk_smart_config.h"
#endif
// #if CONFIG_AGORA_IOT_SDK
// #include "bk_smart_config_agora_adapter.h"
// #endif
#include "app_event.h"
#include "audio_process.h"
#include "cli.h"

#include "audio_engine.h"
#include "video_engine.h"
#include <driver/aon_rtc.h>
#include "agora_convoai_iot.h"

#define TAG "agora_main"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_DEBUG_DUMP
#include "debug_dump.h"
extern bool rx_spk_data_flag;
#endif//CONFIG_DEBUG_DUMP

#define VIDEO_FRAME_INTERVAL_MS            500


bool g_connected_flag = false;
bool g_agent_offline = true;
static bool audio_en = false;
static bool video_en = false;


static beken_thread_t  agora_thread_hdl = NULL;
static beken_semaphore_t agora_sem = NULL;
bool agora_runing = false;
static agora_rtc_config_t agora_rtc_config = DEFAULT_AGORA_RTC_CONFIG();
static agora_rtc_option_t agora_rtc_option = DEFAULT_AGORA_RTC_OPTION();
char agora_channel_name[AGORA_CONVOAI_CHANNEL_NAME_SIZE] = {0};
static agora_convoai_configs_resp_t *convoai_configs = NULL;
static agora_convoai_start_resp_t *convoai_start_resp = NULL;
static beken2_timer_t agora_convoai_start_countdown_ms_timer = { 0 };

static uint32_t g_target_bps = BANDWIDTH_ESTIMATE_MIN_BITRATE;
extern bool smart_config_running;

#if (CONFIG_IMAGE_DEBUG_DUMP)
static bool video_lock = false;
#endif

extern bk_err_t video_turn_off(void);
extern bk_err_t video_turn_on(void);

static void agora_rtc_user_notify_msg_handle(agora_rtc_msg_t *p_msg)
{
    switch (p_msg->code)
    {
        case AGORA_RTC_MSG_JOIN_CHANNEL_SUCCESS:
            g_connected_flag = true;
            LOGI("Join channel success.\n");
            break;
        case AGORA_RTC_MSG_REJOIN_CHANNEL_SUCCESS:
            g_connected_flag = true;
            LOGI("Rejoin channel success.\n");
            if (g_agent_offline == false)
                network_reconnect_stop_timeout_check();
            app_event_send_msg(APP_EVT_RTC_REJOIN_SUCCESS, 0);
            break;
        case AGORA_RTC_MSG_USER_JOINED:
            LOGI("User Joined.\n");
            network_reconnect_stop_timeout_check();
            app_event_send_msg(APP_EVT_AGENT_JOINED, 0);
            g_agent_offline = false;
            smart_config_running = false;
            break;
        case AGORA_RTC_MSG_USER_OFFLINE:
            LOGI("User Offline.\n");
            g_agent_offline = true;
            app_event_send_msg(APP_EVT_AGENT_OFFLINE, 0);
            if (g_connected_flag == true)
               app_event_send_msg(APP_EVT_AGENT_DEVICE_REMOVE, 0);
            break;
        case AGORA_RTC_MSG_CONNECTION_LOST:
            LOGE("Lost connection. Please check wifi status.\n");
            g_connected_flag = false;
            app_event_send_msg(APP_EVT_RTC_CONNECTION_LOST, 0);
            break;
        case AGORA_RTC_MSG_INVALID_APP_ID:
            LOGE("Invalid App ID. Please double check.\n");
            break;
        case AGORA_RTC_MSG_INVALID_CHANNEL_NAME:
            LOGE("Invalid channel name. Please double check.\n");
            break;
        case AGORA_RTC_MSG_INVALID_TOKEN:
        case AGORA_RTC_MSG_TOKEN_EXPIRED:
            LOGE("Invalid token. Please double check.\n");
            break;
        case AGORA_RTC_MSG_BWE_TARGET_BITRATE_UPDATE:
            g_target_bps = p_msg->data.bwe.target_bitrate;
            break;
        case AGORA_RTC_MSG_KEY_FRAME_REQUEST:
#if 0
            media_app_h264_regenerate_idr(camera_device.type);
#endif
            break;
        default:
            break;
    }
}


static void memory_free_show(void)
{
    uint32_t total_size, free_size, mini_size;

    LOGW("%-5s   %-5s   %-5s   %-5s   %-5s\r\n", "name", "total", "free", "minimum", "peak");

    total_size = rtos_get_total_heap_size();
    free_size  = rtos_get_free_heap_size();
    mini_size  = rtos_get_minimum_free_heap_size();
    LOGW("heap:\t%d\t%d\t%d\t%d\r\n",  total_size, free_size, mini_size, total_size - mini_size);

#if CONFIG_PSRAM_AS_SYS_MEMORY
    total_size = rtos_get_psram_total_heap_size();
    free_size  = rtos_get_psram_free_heap_size();
    mini_size  = rtos_get_psram_minimum_free_heap_size();
    LOGW("psram:\t%d\t%d\t%d\t%d\r\n", total_size, free_size, mini_size, total_size - mini_size);
#endif
}

static void app_media_read_frame_callback(frame_buffer_t *frame)
{
    video_frame_info_t info = { 0 };
    static uint64_t before = 0, curr = 0;
    curr = bk_aon_rtc_get_ms();

    if (false == g_connected_flag)
    {
        /* agora rtc is not running, do not send video. */
        return;
    }

    if (before == 0)
        before = curr;

    info.stream_type = VIDEO_STREAM_HIGH;
    if (frame->fmt == PIXEL_FMT_JPEG)
    {
        info.data_type = VIDEO_DATA_TYPE_GENERIC_JPEG;
        info.frame_type = VIDEO_FRAME_KEY;
    }
    else if (frame->fmt == PIXEL_FMT_H264)
    {
        if ((frame->h264_type & (1 << H264_NAL_I_FRAME)) == 0)
        {
            LOGW("%s, ####not i frame, 0x%8x%08x - 0x%8x%08x : 0x%8%08x###\n", __func__, (uint32_t)(curr>>32), (uint32_t)curr, (uint32_t)(before>>32), (uint32_t)before, (uint32_t)((curr - before)>>32), (uint32_t)(curr - before));
            return;
        }
        info.data_type = VIDEO_DATA_TYPE_H264;
        info.frame_type = VIDEO_FRAME_AUTO_DETECT;
    }
    else if (frame->fmt == PIXEL_FMT_H265)
    {
        info.data_type = VIDEO_DATA_TYPE_H265;
        info.frame_type = VIDEO_FRAME_AUTO_DETECT;
    }
    else
    {
        LOGE("not support format: %d \r\n", frame->fmt);
    }

    if (curr > before && curr - before >= VIDEO_FRAME_INTERVAL_MS)
    {
        LOGI("##########send frame: 0x%x%08x - 0x%x%08x : 0x%x%08x######################\n", (uint32_t)(curr>>32), (uint32_t)curr, (uint32_t)(before>>32), (uint32_t)before, (uint32_t)((curr - before)>>32), (uint32_t)(curr - before));
#if (CONFIG_IMAGE_DEBUG_DUMP)
        do {
#endif
            bk_agora_rtc_video_data_send((uint8_t *)frame->frame, (size_t)frame->length, &info);

            /* send two frame images per second */
#if (CONFIG_IMAGE_DEBUG_DUMP)
        } while (video_lock);
#endif
        before = curr;
    }

}

static int agora_rtc_user_audio_rx_data_handle(unsigned char *data, unsigned int size, const audio_frame_info_t *info_ptr)
{
    bk_err_t ret = BK_OK;

    #if CONFIG_DEBUG_DUMP
    if(rx_spk_data_flag)
    {
        uint32_t decoder_type = bk_aud_get_decoder_type();
        uint8_t dump_file_type = dbg_dump_get_dump_file_type((uint8_t)decoder_type);
        DEBUG_DATA_DUMP_UPDATE_HEADER_DUMP_FILE_TYPE(DUMP_TYPE_RX_SPK,0,dump_file_type);
        DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_LEN(DUMP_TYPE_RX_SPK,0,size);
        DEBUG_DATA_DUMP_UPDATE_HEADER_TIMESTAMP(DUMP_TYPE_RX_SPK);
        #if CONFIG_DEBUG_DUMP_DATA_TYPE_EXTENSION
        DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_SAMP_RATE(DUMP_TYPE_RX_SPK,0,bk_aud_get_dac_sample_rate());
        DEBUG_DATA_DUMP_UPDATE_HEADER_DATA_FLOW_FRAME_IN_MS(DUMP_TYPE_RX_SPK,0,bk_aud_get_dec_frame_len_in_ms());
        #endif
        DEBUG_DATA_DUMP_BY_UART_HEADER(DUMP_TYPE_RX_SPK);
        DEBUG_DATA_DUMP_UPDATE_HEADER_SEQ_NUM(DUMP_TYPE_RX_SPK);
        DEBUG_DATA_DUMP_BY_UART_DATA(data, size);
    }
    #endif//CONFIG_DEBUG_DUMP

    ret = bk_aud_intf_write_spk_data((uint8_t *)data, (uint32_t)size);
    if (ret != BK_OK)
    {
        LOGE("write spk data fail \r\n");
    }

    return ret;
}

void agora_main(void *args)
{
    agora_convoai_configs_resp_t *configs = (agora_convoai_configs_resp_t *)args;
    bk_err_t ret = BK_OK;
    memory_free_show();

    LOGI("version: v%s built at %s %s\n", AGORA_CONVOAI_APP_VERSION, __DATE__, __TIME__);

    //service_opt.license_value[0] = '\0';
    agora_rtc_config.p_appid = configs->app_id;
    agora_rtc_config.log_disable = true;
    agora_rtc_config.bwe_param_max_bps = BANDWIDTH_ESTIMATE_MAX_BITRATE;

    audio_tras_register_tx_data_func(bk_agora_rtc_audio_data_send);
    video_register_tx_data_func(app_media_read_frame_callback);

    ret = bk_agora_rtc_create(&agora_rtc_config, (agora_rtc_msg_notify_cb)agora_rtc_user_notify_msg_handle);
    if (ret != BK_OK)
    {
        LOGI("bk_agora_rtc_create fail \r\n");
    }
    // LOGI("-----start agora rtc process-----\r\n");

    agora_rtc_option.p_channel_name = (char *)psram_malloc(os_strlen(agora_channel_name) + 1);
    os_strcpy((char *)agora_rtc_option.p_channel_name, agora_channel_name);
    agora_rtc_option.audio_config.audio_data_type = CONFIG_AUDIO_CODEC_TYPE;
#if defined(CONFIG_SEND_PCM_DATA)
    agora_rtc_option.audio_config.pcm_sample_rate = CONFIG_PCM_SAMPLE_RATE;
    agora_rtc_option.audio_config.pcm_channel_num = CONFIG_PCM_CHANNEL_NUM;
#endif
    agora_rtc_option.p_token = ((configs->rtc_token[0] == '\0' || (0 == strcmp(configs->app_id, configs->rtc_token))) ? NULL : configs->rtc_token);
    agora_rtc_option.uid = AGORA_CONVOAI_LOCAL_UID;
    LOGI("appid=%s, token=%s\n", agora_rtc_config.p_appid, NULL == agora_rtc_option.p_token ? "NULL" : agora_rtc_option.p_token);

    ret = bk_agora_rtc_start(&agora_rtc_option);
    if (ret != BK_OK)
    {
        LOGE("bk_agora_rtc_start fail, ret:%d \r\n", ret);
        return;
    }

    agora_runing = true;

    rtos_set_semaphore(&agora_sem);

    /* wait until we join channel successfully */
    while (!g_connected_flag)
    {
        // memory_free_show();
        //        rtos_dump_task_runtime_stats();
        if (!agora_runing)
        {
            goto exit;
        }
        rtos_delay_milliseconds(100);
    }

    LOGI("-----agora_rtc_join_channel success-----\r\n");

#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE
    ret = bk_agora_rtc_register_audio_rx_handle((agora_rtc_audio_rx_data_handle)agora_rtc_user_audio_rx_data_handle);
    if (ret != BK_OK)
    {
        LOGE("bk_aggora_rtc_register_audio_rx_handle fail, ret:%d \r\n", ret);
    }
#else
    /* turn on audio */
    if (audio_en)
    {
        ret = audio_turn_on();
        if (ret != BK_OK)
        {
            LOGE("%s, %d, audio turn on fail, ret:%d\n", __func__, __LINE__, ret);
            goto exit;
        }
        memory_free_show();
    }
#endif

    /* turn on video */
    if (video_en)
    {
        ret = video_turn_on();
        if (ret != BK_OK)
        {
            LOGE("%s, %d, video turn on fail, ret:%d\n", __func__, __LINE__, ret);
            goto exit;
        }
        memory_free_show();
    }

    while (agora_runing)
    {
        rtos_delay_milliseconds(5000);
        //memory_free_show();
        //rtos_dump_task_runtime_stats();
    }

exit:
#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE
    /* deregister callback to handle audio data received from agora rtc */
    bk_agora_rtc_register_audio_rx_handle(NULL);
#else
    /* free audio  */
    if (audio_en)
    {
        audio_turn_off();
    }
#endif

    /* free video sources */
    if (video_en)
    {
        video_turn_off();
    }

    /* free agora */
    /* stop agora rtc */
    bk_agora_rtc_stop();

    /* destory agora rtc */
    bk_agora_rtc_destroy();

    if (agora_rtc_option.p_channel_name)
    {
        psram_free((char *)agora_rtc_option.p_channel_name);
        agora_rtc_option.p_channel_name = NULL;
    }

    audio_en = false;
    video_en = false;

    g_connected_flag = false;

    /* delete task */
    agora_thread_hdl = NULL;

    agora_runing = false;

    rtos_set_semaphore(&agora_sem);

    rtos_delete_thread(NULL);
}

bk_err_t agora_stop(void)
{
    if (!agora_runing)
    {
        LOGI("agora not start\n");
        return BK_OK;
    }

    agora_runing = false;

    rtos_get_semaphore(&agora_sem, BEKEN_NEVER_TIMEOUT);

    rtos_deinit_semaphore(&agora_sem);
    agora_sem = NULL;

    return BK_OK;
}

static int get_ota_version(const char *url, int8_t *major_ota, int8_t *minor_ota, int8_t *patch_ota)
{
  char *version = strstr(url, "app_pack_");
  if (NULL == version) {
    LOGE("ota url invalid. app_pack_ string not find\n");
    return -1;
  }

  char *tmp = version;
  while (tmp) {
    LOGI("%s%d: tmp=%s\n", __FUNCTION__, __LINE__, tmp);
    if (NULL != (tmp = strstr(version + strlen("app_pack_"), "app_pack_"))) {
      version = tmp;
      LOGI("%s%d: version=%s\n", __FUNCTION__, __LINE__, version);
    }
  }

  if (3 != sscanf(version, "app_pack_%d.%d.%d.rbl", major_ota, minor_ota, patch_ota)) {
    LOGE("ota version invalid.\n");
    return -1;
  }

  return 0;
}

static void replace_https_to_http(char *url)
{
    const char* pos = strstr(url, "https://");
    if (pos != url) {
        return;
    }

    strcpy(url, "http://");
    char *dst = url + strlen("http://");
    char *src = url + strlen("https://");
    int len = strlen(src) + 1;
    memmove(dst, src, len);
    LOGI("%s%d: url=%s\n", __FUNCTION__, __LINE__, url);
}

static bool g_ota_inited = false;
extern int bk_http_ota_download(const char *uri);
static void ota_task(void *args)
{
    LOGI("curr version: v%s built at %s %s\n", AGORA_CONVOAI_APP_VERSION, __DATE__, __TIME__);

    /* step1. 从服务器拉取OTA信息 */
    agora_convoai_ota_version_t *ota_version = agora_convoai_ota_version_get();
        if (NULL == ota_version) {
        LOGI("donot need ota update. no version find\n");
            goto L_END;
        }

    /* step2. 从OTA升级固件下载URL中的固件名，解析出固件对应的版本号，固件的命名规则app_pack_x.x.x.rbl x.x.x即为版本号 */
    int8_t major_ota = -1, minor_ota = -1, patch_ota = -1;
    if (0 != get_ota_version(ota_version->url, &major_ota, &minor_ota, &patch_ota)) {
        LOGE("ota version parse failed.\n");
        goto L_END;
    }
    LOGI("OTA version: firmware_id=%s, ver=%d.%d.%d\n", ota_version->firmware_id, major_ota, minor_ota, patch_ota);

    /* step3. 从flash中读取OTA升级信息 */
    agora_convoai_ota_info_t ota_info = {0};
            agora_convoai_ota_info_persistence_read(&ota_info);
    LOGI("OTA flash inf: firmware_id=%s, version=%d.%d.%d\n", ota_info.firmware_id, ota_info.major, ota_info.minor, ota_info.patch);

    /* 如果flash中的firmware_id与服务器给的一致，说明设备已经OTA升级完成，本次为升级完成后设备重启 */
    if (0 == strcmp(ota_version->firmware_id, ota_info.firmware_id)) {
        int8_t major_cur = 0, minor_cur = 0, patch_cur = 0;
        agora_convoai_ota_result_report_t ota_report;
        sscanf(AGORA_CONVOAI_APP_VERSION, "%d.%d.%d", &major_cur, &minor_cur, &patch_cur);
        /* 如果falsh中记录的OTA升级版本号与当前运行的软件版本一样就表情OTA升级成功，反之则代表OTA升级失败 */
        ota_report.is_install_success = (major_cur == ota_info.major && minor_cur == ota_info.minor && patch_cur == ota_info.patch);
        LOGI("OTA update success=%d\n", ota_report.is_install_success);

        /* 上报firmware_id对应的OTA升级结果 */
        snprintf(ota_report.firmware_id, sizeof(ota_report.firmware_id), "%s", ota_version->firmware_id);
        if (0 == agora_convoai_ota_result_report(&ota_report)) {
            /* 上报成功，则清空flash中记录的OTA升级信息 */
            memset(&ota_info, 0, sizeof(ota_info));
                    agora_convoai_ota_info_persistence_write(&ota_info);
                }
            goto L_END;
        }

    /* step4. OTA升级 */
    /* step4.1 先尝试关闭convoai，再进行OTA升级 */
        app_event_send_msg(APP_EVT_CONVOAI_EXIT, 0);
        rtos_delay_milliseconds(1000);

    /* step4.2 flash中记录本次OTA升级信息 */
    snprintf(ota_info.firmware_id, sizeof(ota_info.firmware_id), "%s", ota_version->firmware_id);
    ota_info.major = major_ota;
    ota_info.minor = minor_ota;
    ota_info.patch = patch_ota;
    agora_convoai_ota_info_persistence_write(&ota_info);

        LOGI("%s%d: agora ota process start.\n", __FUNCTION__, __LINE__);

    /* step4.3 如果给的URL是https则替换为http */
    replace_https_to_http(ota_version->url);
    /* step4.4 OTA升级 */
    int err = bk_http_ota_download(ota_version->url);
        if (0 != err) {
            LOGE("%s%d: agora ota process failed.\n", __FUNCTION__, __LINE__);
            goto L_END;
        }
        LOGI("%s%d: agora ota process success.\n", __FUNCTION__, __LINE__);

L_END:
        if (ota_version) {
            psram_free(ota_version);
            ota_version = NULL;
        }

    LOGI("%s%d: agora ota task exit.\n", __FUNCTION__, __LINE__);

    rtos_delete_thread(NULL);
}

/* 当前有且仅在设备上电且设备联网成功后尝试一次OTA升级 */
void agora_convoai_ota_check()
{
    bk_err_t ret = BK_OK;
    beken_thread_t ota_pid = NULL;

    if (g_ota_inited) {
        LOGI("ota daemon already inited.\n");
        return;
    }

    ret = rtos_create_thread(&ota_pid, 4, "agora_ota_task", ota_task, 4 * 1024, NULL);
    if (ret != kNoErr)
    {
        LOGE("%s, %d, create ota_daemon task fail, ret:%d\n", __func__, __LINE__, ret);
        return;
    }

    g_ota_inited = true;
    LOGI("create ota_daemon task complete\n");
}

static bk_err_t agora_start(agora_convoai_configs_resp_t *configs)
{
    bk_err_t ret = BK_OK;

    if (agora_runing)
    {
        LOGI("agora already start, Please close and then reopens\n");
        return BK_FAIL;
    }

    ret = rtos_init_semaphore(&agora_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, create semaphore fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = rtos_create_thread(&agora_thread_hdl,
                             4,
                             "agora",
                             (beken_thread_function_t)agora_main,
                             6 * 1024,
                             configs);
    if (ret != kNoErr)
    {
        LOGE("%s, %d, create agora app task fail, ret:%d\n", __func__, __LINE__, ret);
        agora_thread_hdl = NULL;
        goto fail;
    }

    rtos_get_semaphore(&agora_sem, BEKEN_NEVER_TIMEOUT);

    LOGI("create agora app task complete\n");

    return BK_OK;

fail:

    if (agora_sem)
    {
        rtos_deinit_semaphore(&agora_sem);
        agora_sem = NULL;
    }

    return BK_FAIL;
}
/* call this api when wifi autoconnect */

static void __timer_expire_handler()
{
    LOGI("convoai timer exipre. donot stop agent now\n");
    //app_event_send_msg(APP_EVT_CONVOAI_START_TIMER_EXPIRE, 0);
}

static void __convoai_timer_start_or_relaunch()
{
    bk_err_t result;

    if (agora_convoai_start_countdown_ms_timer.handle) {
        LOGI("convoai timer exist, relaunch timer.\n");
        rtos_oneshot_reload_timer(&agora_convoai_start_countdown_ms_timer);
        return;
    }

    result = rtos_init_oneshot_timer(&agora_convoai_start_countdown_ms_timer, 180 * 1000, __timer_expire_handler, NULL, NULL);
    if (kNoErr != result) {
        LOGE("convoai timer create failed.\n");
        return;
        }

    result = rtos_start_oneshot_timer(&agora_convoai_start_countdown_ms_timer);
    if (kNoErr != result) {
        LOGE("convoai timer start failed.\n");
        return;
        }

    LOGI("convoai timer start running.\n");
}

int agora_convoai_engine_load_config()
{
    if (convoai_configs) {
        LOGI("convoai config already loaded. refresh config\n");
        psram_free(convoai_configs);
        convoai_configs = NULL;
    }

    agora_convoai_configs_param_t convoai_config_param;
    snprintf(convoai_config_param.channel_name, sizeof(convoai_config_param.channel_name), "%s", "*");
    convoai_config_param.local_uid = AGORA_CONVOAI_LOCAL_UID;
    convoai_config_param.agent_uid = AGORA_CONVOAI_AGENT_UID;
    LOGI("channel_name=%s, uid=%d\n", convoai_config_param.channel_name, convoai_config_param.local_uid);
    if (NULL == (convoai_configs = agora_convoai_configs_get(&convoai_config_param))) {
        LOGE("convoai get configs failed.\n");
        return -1;
    }

    LOGI("convoai get config success. appid=%s, rtc_token=%s\n", convoai_configs->app_id, convoai_configs->rtc_token);
    return 0;
}

void agora_convoai_engine_start()
{
    /* 判断是否重复启动，重复启动直接退出 */
    if (convoai_start_resp) {
        LOGI("convoai has already started. refresh timer then return.\n");
        __convoai_timer_start_or_relaunch();
        return;
        }

    /* 如果设备联网后http获取设备配置失败，需先拉取配置，如果拉取配置仍失败，则退出 */
    if (NULL == convoai_configs) {
        LOGI("convoai config not load, load configs first.\n");
        if (0 != agora_convoai_engine_load_config()) {
            LOGI("convoai config load failed.\n");
            return;
    }
    }

    /**
     * 如果APPID启用了安全校验token模式，需检查token是否已过期，假定token有效期为12小时
     * 此处trick，获取token时的频道号设置为通配符*，以此来解决channel name全局一致性带来的每次token都需要更新的问题
     */
    if (convoai_configs->token_enable) {
        uint32_t now = rtos_get_time();
        #define TOKEN_TIMEOUS_MSEC (12UL * 60 * 60 * 1000)
        uint32_t elapsed = (now >= convoai_configs->timestamp) ? (now - convoai_configs->timestamp) : (UINT32_MAX - convoai_configs->timestamp + now);
        if (elapsed >= TOKEN_TIMEOUS_MSEC) {
            LOGI("convoai token expired. reload configs\n");
            if (0 != (agora_convoai_engine_load_config())) {
                LOGI("convoai config reload failed.\n");
                return;
        }
        }
    }

    /* 拉起本地rtsa */
    agora_convoai_get_channel_name(agora_channel_name);
    agora_start(convoai_configs);

    /* 拉起convoai服务端 */
    agora_convoai_start_param_t convoai_start_param;
    os_memcpy(convoai_start_param.channel_name, agora_channel_name, sizeof(convoai_start_param.channel_name));
    convoai_start_param.local_uid = AGORA_CONVOAI_LOCAL_UID;
    if (NULL == (convoai_start_resp = agora_convoai_start(&convoai_start_param))) {
        LOGE("convoai start failed.\n");
        return;
    }
    LOGI("convoai start succcess. conversation_id=%s\n", convoai_start_resp->conversation_id);

    /* 拉取定时器 */
    __convoai_timer_start_or_relaunch();
}

void agora_convoai_engine_stop()
{
    /* 退出rtsa */
    agora_stop();

    /* 判断是否已启动，未启动直接退出 */
    if (NULL == convoai_start_resp) {
        LOGI("convoai has not started. just return\n");
        return;
    }

    /* 退出convoai服务端 */
    agora_convoai_stop_param_t convoai_stop_param;
    os_memcpy(convoai_stop_param.conversation_id, convoai_start_resp->conversation_id, sizeof(convoai_stop_param.conversation_id));
    agora_convoai_stop(&convoai_stop_param);
    psram_free(convoai_start_resp);
    convoai_start_resp = NULL;
    LOGI("convoai stop success.\n");

    /* 取消定时器 */
    if (rtos_is_oneshot_timer_running(&agora_convoai_start_countdown_ms_timer)) {
        bk_err_t ret = rtos_stop_oneshot_timer(&agora_convoai_start_countdown_ms_timer);
        if(kNoErr != ret) {
            LOGE("stop convoai timer failed.\n");
        } else {
            LOGI("stop convoai timer success.\n");
        }
        }
}

#if 0
static void agora_test_start()
{
    agora_convoai_start_resp_t *resp;
    agora_convoai_start_param_t param;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    snprintf(param.channel_name, sizeof(param.channel_name), "%s", "benchmark");
    param.local_uid = 1;
    param.agent_uid = 11;
    resp = agora_convoai_start(&param);
    if (resp) {
        psram_free(resp);
    }
}

static void agora_test_stop()
{
    agora_convoai_stop_param_t param;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    snprintf(param.conversation_id, sizeof(param.conversation_id), "%s", "01234567890");
    agora_convoai_stop(&param);
}

static void agora_test_ota()
{
    agora_convoai_ota_version_t *version;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    version = agora_convoai_ota_version_get();
    if (version) {
        psram_free(version);
        }
}

static void agora_test_ota_report()
{
    agora_convoai_ota_result_report_t report;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    snprintf(report.firmware_id, sizeof(report.firmware_id), "%s", "123");
    report.is_install_success = true;
    agora_convoai_ota_result_report(&report);
}

static void agora_test_token()
{
    agora_convoai_configs_param_t param;
    agora_convoai_configs_resp_t *resp;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    snprintf(param.channel_name, sizeof(param.channel_name), "%s", "benchmark");
    param.local_uid = AGORA_CONVOAI_LOCAL_UID;
    resp = agora_convoai_configs_get(&param);
    if (resp) {
        psram_free(resp);
    }
}

static void agora_test_clear()
{
    agora_convoai_ota_info_t ota_info;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    memset(&ota_info, 0, sizeof(ota_info));
    agora_convoai_ota_info_persistence_write(&ota_info);
}

static void agora_test_inf()
{
    agora_convoai_ota_info_t ota_info;

    LOGI("%s%d\n", __FUNCTION__, __LINE__);

    memset(&ota_info, 0, sizeof(ota_info));
    agora_convoai_ota_info_persistence_read(&ota_info);

    LOGI("ota firmware_id=%s, version=%d.%d.%d\n", ota_info.firmware_id, ota_info.major, ota_info.minor, ota_info.patch);
}

static void agora_test_volume_increase()
{
    LOGI("%s%d\n", __FUNCTION__, __LINE__);
    //bk_key_soft_volume_increase();
}

static void agora_test_volume_decrease()
{
    LOGI("%s%d\n", __FUNCTION__, __LINE__);
    //bk_key_soft_volume_decrease();
}

static const struct cli_command agora_commands[] = {
    {"agstart",     NULL, agora_test_start},
    {"agstop",      NULL, agora_test_stop},
    {"agota",       NULL, agora_test_ota},
    {"agotareport", NULL, agora_test_ota_report},
    {"agtoken",     NULL, agora_test_token},
    {"agclear",     NULL, agora_test_clear},
    {"aginfo",      NULL, agora_test_inf},
    {"aginc",       NULL, agora_test_volume_increase},
    {"agdec",       NULL, agora_test_volume_decrease},
};
#define AGORA_CMD_CNT (sizeof(agora_commands) / sizeof(struct cli_command))
#endif

int agora_rtc_cli_init(void)
{
    //cli_register_commands(agora_commands, AGORA_CMD_CNT);
    return 0;
}