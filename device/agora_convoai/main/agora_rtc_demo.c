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
#include "agora_config.h"
#include "agora_rtc.h"
#include "audio_transfer.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#include <driver/media_types.h>
#include <driver/lcd.h>
#include <modules/wifi.h>
#include "modules/wifi_types.h"
#include "media_app.h"
#include "lcd_act.h"
#include "components/bk_uid.h"
#if CONFIG_NETWORK_AUTO_RECONNECT
#include "bk_genie_smart_config.h"
#endif
#include "app_event.h"
#include "agora_convoai_iot.h"

#define TAG "agora_main"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#define AGORA_RX_SPK_DATA_DUMP

#ifdef AGORA_RX_SPK_DATA_DUMP
#include "uart_util.h"
static uart_util_t g_agora_spk_uart_util = {0};
#define AGORA_RX_SPK_DATA_DUMP_UART_ID            (1)
#define AGORA_RX_SPK_DATA_DUMP_UART_BAUD_RATE     (2000000)

#define AGORA_RX_SPK_DATA_DUMP_OPEN()                        uart_util_create(&g_agora_spk_uart_util, AGORA_RX_SPK_DATA_DUMP_UART_ID, AGORA_RX_SPK_DATA_DUMP_UART_BAUD_RATE)
#define AGORA_RX_SPK_DATA_DUMP_CLOSE()                       uart_util_destroy(&g_agora_spk_uart_util)
#define AGORA_RX_SPK_DATA_DUMP_DATA(data_buf, len)           uart_util_tx_data(&g_agora_spk_uart_util, data_buf, len)
#else
#define AGORA_RX_SPK_DATA_DUMP_OPEN()
#define AGORA_RX_SPK_DATA_DUMP_CLOSE()
#define AGORA_RX_SPK_DATA_DUMP_DATA(data_buf, len)
#endif  //AGORA_RX_SPK_DATA_DUMP

#ifdef CONFIG_USE_G722_CODEC
#define AUDIO_SAMP_RATE         (16000)
#else
#define AUDIO_SAMP_RATE         (8000)
#endif
#define AEC_ENABLE              (1)


bool g_connected_flag = false;
static bool audio_en = false;
static bool video_en = false;
static media_camera_device_t camera_device =
{

#if defined(CONFIG_UVC_CAMERA)
    .type = UVC_CAMERA,
    .mode = JPEG_MODE,
    .fmt  = PIXEL_FMT_JPEG,
    /* expect the width and length */
    .info.resolution.width  = 640,//640,//864,
    .info.resolution.height = 480,
    .info.fps = FPS25,
#elif defined(CONFIG_DVP_CAMERA)
    /* DVP Camera */
    .type = DVP_CAMERA,
    .mode = H264_MODE,//JPEG_MODE
    .fmt  = PIXEL_FMT_H264,//PIXEL_FMT_JPEG
    /* expect the width and length */
    .info.resolution.width  = 640,//1280,//,
    .info.resolution.height = 480,//720,//,
    .info.fps = FPS20,
#endif
};


static beken_thread_t  agora_thread_hdl = NULL;
static beken_semaphore_t agora_sem = NULL;
bool agora_runing = false;
static agora_rtc_config_t agora_rtc_config = DEFAULT_AGORA_RTC_CONFIG();
static agora_rtc_option_t agora_rtc_option = DEFAULT_AGORA_RTC_OPTION();
static agora_convoai_configs_resp_t *convoai_configs = NULL;
static agora_convoai_start_resp_t *convoai_start_resp = NULL;

static uint32_t g_target_bps = BANDWIDTH_ESTIMATE_MIN_BITRATE;
extern bool smart_config_running;
extern uint32_t volume;
extern uint32_t g_volume_gain[SPK_VOLUME_LEVEL];

#if CONFIG_WIFI_ENABLE
extern void rwnxl_set_video_transfer_flag(uint32_t video_transfer_flag);
#else
#define rwnxl_set_video_transfer_flag(...)
#endif

static void agora_rtc_user_notify_msg_handle(agora_rtc_msg_t *p_msg)
{
    switch (p_msg->code)
    {
        case AGORA_RTC_MSG_JOIN_CHANNEL_SUCCESS:
            g_connected_flag = true;
            LOGI("Join channel success.\n");
            break;
        case AGORA_RTC_MSG_USER_JOINED:
            LOGI("User Joined.\n");
            app_event_send_msg(APP_EVT_AGENT_JOINED, 0);
            g_connected_flag = true;	//for rejoin success
            smart_config_running = false;
            break;
        case AGORA_RTC_MSG_USER_OFFLINE:
            LOGI("User Offline.\n");
            app_event_send_msg(APP_EVT_AGENT_OFFLINE, 0);
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
    LOGI("psram:\t%d\t%d\t%d\t%d\r\n", total_size, free_size, mini_size, total_size - mini_size);
#endif
}

#if defined(CONFIG_UVC_CAMERA)
static void media_checkout_uvc_device_info(bk_uvc_device_brief_info_t *info, uvc_state_t state)
{
    bk_uvc_config_t uvc_config_info_param = {0};
    uint8_t format_index = 0;
    uint8_t frame_num = 0;
    uint8_t index = 0;

    if (state == UVC_CONNECTED)
    {
        uvc_config_info_param.vendor_id  = info->vendor_id;
        uvc_config_info_param.product_id = info->product_id;

        format_index = info->format_index.mjpeg_format_index;
        frame_num    = info->all_frame.mjpeg_frame_num;
        if (format_index > 0)
        {
            LOGI("%s uvc_get_param MJPEG format_index:%d\r\n", __func__, format_index);
            for (index = 0; index < frame_num; index++)
            {
                LOGI("uvc_get_param MJPEG width:%d heigth:%d index:%d\r\n",
                     info->all_frame.mjpeg_frame[index].width,
                     info->all_frame.mjpeg_frame[index].height,
                     info->all_frame.mjpeg_frame[index].index);
                for (int i = 0; i < info->all_frame.mjpeg_frame[index].fps_num; i++)
                {
                    LOGI("uvc_get_param MJPEG fps:%d\r\n", info->all_frame.mjpeg_frame[index].fps[i]);
                }

                if (info->all_frame.mjpeg_frame[index].width == camera_device.info.resolution.width
                    && info->all_frame.mjpeg_frame[index].height == camera_device.info.resolution.height)
                {
                    uvc_config_info_param.frame_index = info->all_frame.mjpeg_frame[index].index;
                    uvc_config_info_param.fps         = info->all_frame.mjpeg_frame[index].fps[0];
                    uvc_config_info_param.width       = camera_device.info.resolution.width;
                    uvc_config_info_param.height      = camera_device.info.resolution.height;
                }
            }
        }

        uvc_config_info_param.format_index = format_index;

        if (media_app_set_uvc_device_param(&uvc_config_info_param) != BK_OK)
        {
            LOGE("%s, failed\r\n, __func__");
        }
    }
    else
    {
        LOGI("%s, %d\r\n", __func__, state);
    }
}
#endif

void app_media_read_frame_callback(frame_buffer_t *frame)
{
    video_frame_info_t info = { 0 };

    if (false == g_connected_flag)
    {
        /* agora rtc is not running, do not send video. */
        return;
    }

    info.stream_type = VIDEO_STREAM_HIGH;
    if (frame->fmt == PIXEL_FMT_JPEG)
    {
        info.data_type = VIDEO_DATA_TYPE_GENERIC_JPEG;
        info.frame_type = VIDEO_FRAME_KEY;
    }
    else if (frame->fmt == PIXEL_FMT_H264)
    {
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

    bk_agora_rtc_video_data_send((uint8_t *)frame->frame, (size_t)frame->length, &info);

    /* send two frame images per second */
    rtos_delay_milliseconds(500);
}

static int agora_rtc_user_audio_rx_data_handle(unsigned char *data, unsigned int size, const audio_frame_info_t *info_ptr)
{
    bk_err_t ret = BK_OK;

    ret = bk_aud_intf_write_spk_data((uint8_t *)data, (uint32_t)size);
    if (ret != BK_OK)
    {
        LOGE("write spk data fail \r\n");
    }

    return ret;
}

bk_err_t video_turn_off(void)
{
    bk_err_t ret =  BK_OK;
    LOGI("%s\n", __func__);

    ret = media_app_unregister_read_frame_callback();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, unregister read_frame_cb failed\n", __func__, __LINE__);
    }

    ret = media_app_h264_pipeline_close();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, h264_pipeline_close failed\n", __func__, __LINE__);
    }

    ret = media_app_camera_close(camera_device.type);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, media_app_camera_close failed\n", __func__, __LINE__);
    }

    rwnxl_set_video_transfer_flag(false);

    bk_wifi_set_wifi_media_mode(false);
    bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_HD);

    return BK_OK;
}


static bk_err_t video_turn_on(void)
{
    bk_err_t ret = BK_OK;
    LOGI("%s\n", __func__);

    bk_wifi_set_wifi_media_mode(true);
    bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_FD);

    rwnxl_set_video_transfer_flag(true);

#if defined(CONFIG_UVC_CAMERA)
    media_app_uvc_register_info_notify_cb(media_checkout_uvc_device_info);
#endif

    ret = media_app_camera_open(&camera_device);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, media_app_camera_open failed\n", __func__, __LINE__, ret);
        goto fail;
    }

    bool media_mode = false;
    uint8_t quality = 0;
    bk_wifi_get_wifi_media_mode_config(&media_mode);
    bk_wifi_get_video_quality_config(&quality);
    LOGE("~~~~~~~~~~wifi media mode %d, video quality %d~~~~~~\r\n", media_mode, quality);

#if defined(CONFIG_UVC_CAMERA)
    ret = media_app_h264_pipeline_open();
    if (ret != BK_OK)
    {
        LOGE("%s, %d, h264_pipeline_open failed, ret:%d\n", __func__, __LINE__, ret);
        goto fail;
    }

    ret = media_app_register_read_frame_callback(PIXEL_FMT_H264, app_media_read_frame_callback);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, register read_frame_cb failed\n", __func__, __LINE__, ret);
        goto fail;
    }
#elif defined(CONFIG_DVP_CAMERA)
    ret = media_app_register_read_frame_callback(camera_device.fmt, app_media_read_frame_callback);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, register read_frame_cb failed\n", __func__, __LINE__, ret);
        goto fail;
    }
#endif
    memory_free_show();

    return BK_OK;

fail:
    video_turn_off();

    return BK_FAIL;
}

bk_err_t audio_turn_off(void)
{
    bk_err_t ret =  BK_OK;
    LOGI("%s\n", __func__);
#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE
    if (g_connected_flag)
    {
        bk_agora_rtc_register_audio_rx_handle(NULL);
    }
#else
    /* deregister callback to handle audio data received from agora rtc */
    bk_agora_rtc_register_audio_rx_handle(NULL);
#endif

    /* stop voice */
    ret = bk_aud_intf_voc_stop();
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s, %d, voice stop fail, ret:%d\n", __func__, __LINE__, ret);
    }

    /* deinit vioce */
    ret = bk_aud_intf_voc_deinit();
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s, %d, voice deinit fail, ret:%d\n", __func__, __LINE__, ret);
    }

    bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_NULL);

    ret = bk_aud_intf_drv_deinit();
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s, %d, aud_intf driver deinit fail, ret:%d\n", ret);
    }

    audio_tras_deinit();

    AGORA_RX_SPK_DATA_DUMP_CLOSE();

    return BK_OK;
}

bk_err_t audio_turn_on(void)
{
    bk_err_t ret =  BK_OK;
    LOGI("%s\n", __func__);

    AGORA_RX_SPK_DATA_DUMP_OPEN();

    aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
    aud_intf_voc_setup_t aud_intf_voc_setup = DEFAULT_AUD_INTF_VOC_SETUP_CONFIG();

    audio_tras_init();

    aud_intf_drv_setup.aud_intf_tx_mic_data = send_audio_data_to_agora;
    ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s, %d, aud_intf driver init fail, ret:%d\n", __func__, __LINE__, ret);
    }

    ret = bk_aud_intf_set_mode(AUD_INTF_WORK_MODE_VOICE);
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("%s, %d, aud_intf set_mode fail, ret:%d\n", __func__, __LINE__, ret);
    }

#ifdef CONFIG_USE_G722_CODEC
    aud_intf_voc_setup.data_type  = AUD_INTF_VOC_DATA_TYPE_G722;
#else
    aud_intf_voc_setup.data_type  = AUD_INTF_VOC_DATA_TYPE_G711A;
#endif
    aud_intf_voc_setup.spk_mode   = AUD_DAC_WORK_MODE_DIFFEN;
    aud_intf_voc_setup.aec_enable = AEC_ENABLE;
    aud_intf_voc_setup.samp_rate  = AUDIO_SAMP_RATE;
#if CONFIG_AEC_ECHO_COLLECT_MODE_HARDWARE
    aud_intf_voc_setup.mic_gain   = 0x30;
#else
    aud_intf_voc_setup.mic_gain   = 0x3F;
#endif
    aud_intf_voc_setup.spk_gain   = g_volume_gain[volume];
    aud_intf_voc_setup.mic_type = AUD_INTF_MIC_TYPE_BOARD;
    aud_intf_voc_setup.spk_type = AUD_INTF_MIC_TYPE_BOARD;

    ret = bk_aud_intf_voc_init(aud_intf_voc_setup);
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("bk_aud_intf_voc_init fail, ret:%d \r\n", ret);
    }

#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE

#else
    ret = bk_agora_rtc_register_audio_rx_handle((agora_rtc_audio_rx_data_handle)agora_rtc_user_audio_rx_data_handle);
    if (ret != BK_OK)
    {
        LOGE("bk_aggora_rtc_register_audio_rx_handle fail, ret:%d \r\n", ret);
    }
#endif

    ret = bk_aud_intf_voc_start();
    if (ret != BK_ERR_AUD_INTF_OK)
    {
        LOGE("bk_aud_intf_voc_start fail, ret:%d \r\n", ret);
    }

    return BK_OK;
}


static bk_err_t agora_renew_token(const char *token)
{
    return bk_agora_rtc_renew_token(token);
}

void agora_convoai_engine_load_config()
{
    if (convoai_configs) {
        LOGW("convoai config already loaded, refresh it to get new token.\n");
        psram_free(convoai_configs);
        convoai_configs = NULL;
    }

    agora_convoai_configs_param_t convoai_config_param;
    agora_convoai_get_device_id(convoai_config_param.channel_name);
    convoai_config_param.local_uid = AGORA_CONVOAI_LOCAL_UID;
    convoai_configs = agora_convoai_configs_get(&convoai_config_param);
    if (NULL == convoai_configs) {
        LOGE("convoai get configs failed.\n");
        return;
    }
}

void agora_convoai_engine_renew_token()
{
    // get new token
    agora_convoai_engine_load_config();
    if (convoai_configs) {
        agora_renew_token(convoai_configs->rtc_token);
        LOGW("renew agora rtc token.\n");
    }
}

void agora_main(void *args)
{
    agora_convoai_configs_resp_t *config = (agora_convoai_configs_resp_t *)args;
    bk_err_t ret = BK_OK;

    if (!config) {
        LOGE("convoai configs cannot be NULL!\n");
        return;
    }

    memory_free_show();

    // 2. API: init agora rtc sdk

    //service_opt.license_value[0] = '\0';
    agora_rtc_config.p_appid = config->app_id;
    agora_rtc_config.log_disable = true;
    agora_rtc_config.bwe_param_max_bps = BANDWIDTH_ESTIMATE_MAX_BITRATE;
    ret = bk_agora_rtc_create(&agora_rtc_config, (agora_rtc_msg_notify_cb)agora_rtc_user_notify_msg_handle);
    if (ret != BK_OK)
    {
        LOGI("bk_agora_rtc_create fail \r\n");
    }
    // LOGI("-----start agora rtc process-----\r\n");

    agora_rtc_option.p_channel_name = config->channel_name;
    agora_rtc_option.audio_config.audio_data_type = CONFIG_AUDIO_CODEC_TYPE;
#if defined(CONFIG_SEND_PCM_DATA)
    agora_rtc_option.audio_config.pcm_sample_rate = CONFIG_PCM_SAMPLE_RATE;
    agora_rtc_option.audio_config.pcm_channel_num = CONFIG_PCM_CHANNEL_NUM;
#endif
    agora_rtc_option.p_token = ((0 == strcmp(config->app_id, config->rtc_token)) ? NULL : config->rtc_token);
    agora_rtc_option.uid = config->local_uid;

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

    beken_time_t last_renew_time = rtos_get_time();
    beken_time_t now_time = rtos_get_time();
    int mem_free_print_count = 0;
    while (agora_runing)
    {
        rtos_delay_milliseconds(100);
        if (mem_free_print_count++ > 20) {
            memory_free_show();
            mem_free_print_count = 0;
        }
        now_time = rtos_get_time();
        if ((now_time - last_renew_time) > (12 * 60 * 60 * 1000)) {       // renew token per 12h
            agora_convoai_engine_renew_token();
            last_renew_time = now_time;
        }
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

static bk_err_t agora_start(void)
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
                             convoai_configs);
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

void agora_convoai_engine_start()
{
    if (NULL == convoai_configs) {
        LOGW("convoai config not load, load configs at first.\n");
        return;
    }

    if (convoai_start_resp) {
        LOGW("convoai has already started. just return.\n");
        return;
    }

    agora_convoai_start_param_t convoai_start_param;
    agora_convoai_get_device_id(convoai_start_param.channel_name);
    convoai_start_param.local_uid = AGORA_CONVOAI_LOCAL_UID;
    convoai_start_param.agent_uid = AGORA_CONVOAI_AGENT_UID;
    convoai_start_resp = agora_convoai_start(&convoai_start_param);
    if (NULL == convoai_start_resp) {
        LOGE("convoai start failed.\n");
        return;
    }

    // start agora rtc engine
    agora_start();
}

void agora_convoai_engine_stop()
{
    // stop agora rtc engine
    agora_stop();

    if (NULL == convoai_start_resp) {
        LOGW("convoai has not started. start it at first\n");
        return;
    }

    agora_convoai_stop_param_t convoai_stop_param;
    os_memcpy(convoai_stop_param.agent_id, convoai_start_resp->agent_id, sizeof(convoai_stop_param.agent_id));
    agora_convoai_stop(&convoai_stop_param);

    psram_free(convoai_start_resp);
    convoai_start_resp = NULL;
}
