#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>
#include "cJSON.h"
#include "components/bk_uid.h"

#include "wifi_boarding_utils.h"
#include "bk_genie_comm.h"
#include "boarding_service.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "cli.h"
#include "bk_genie_smart_config.h"
#include "led_blink.h"
#include "pan_service.h"
#include "app_event.h"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TAG "db-core"

typedef struct
{
    uint32_t enabled : 1;
    uint32_t service : 6;

    char *id;
    beken_thread_t thd;
    beken_queue_t queue;
} bk_genie_info_t;

bk_genie_info_t *db_info = NULL;

const bk_genie_service_interface_t *bk_genie_current_service = NULL;


bk_err_t bk_genie_send_msg(bk_genie_msg_t *msg)
{
    bk_err_t ret = BK_OK;

    if (db_info->queue)
    {
        ret = rtos_push_to_queue(&db_info->queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed\n", __func__);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}

extern uint8_t network_disc_evt_posted;
static int bk_genie_wifi_sta_connect(char *ssid, char *key)
{
    int len;

    wifi_sta_config_t sta_config = {0};

    len = os_strlen(key);

    if (32 < len)
    {
        LOGE("ssid name more than 32 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(sta_config.ssid, ssid);

    len = os_strlen(key);

    if (64 < len)
    {
        LOGE("key more than 64 Bytes\r\n");
        return BK_FAIL;
    }

    os_strcpy(sta_config.password, key);
    network_disc_evt_posted = 0;
    LOGE("ssid:%s key:%s\r\n", sta_config.ssid, sta_config.password);
    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    BK_LOG_ON_ERR(bk_wifi_sta_start());

    return BK_OK;
}

static void bk_genie_message_handle(void)
{
    bk_err_t ret = BK_OK;
    bk_genie_msg_t msg;

    while (1)
    {

        ret = rtos_pop_from_queue(&db_info->queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case DBEVT_WIFI_STATION_CONNECT:
                {
                    LOGI("DBEVT_WIFI_STATION_CONNECT\n");
                    bk_genie_boarding_info_t *bk_genie_boarding_info = (bk_genie_boarding_info_t *) msg.param;
                    bk_genie_wifi_sta_connect(bk_genie_boarding_info->boarding_info.ssid_value,
                                              bk_genie_boarding_info->boarding_info.password_value);
                }
                break;

                case DBEVT_WIFI_STATION_CONNECTED:
                {
                    LOGI("DBEVT_WIFI_STATION_CONNECTED\n");
                    netif_ip4_config_t ip4_config;
                    extern uint32_t uap_ip_is_start(void);

                    os_memset(&ip4_config, 0x0, sizeof(netif_ip4_config_t));
                    bk_netif_get_ip4_config(NETIF_IF_AP, &ip4_config);

                    if (uap_ip_is_start())
                    {
                        bk_netif_get_ip4_config(NETIF_IF_AP, &ip4_config);
                    }
                    else
                    {
                        bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_config);
                    }

                    LOGI("ip: %s\n", ip4_config.ip);

                    bk_genie_boarding_event_notify_with_data(BOARDING_OP_STATION_START, BK_OK, ip4_config.ip, strlen(ip4_config.ip));
                }
                break;

                case DBEVT_START_AGORA_AGENT_START:
                {
                    LOGI("DBEVT_START_AGORA_AGENT_START\n");
                    if (!bk_genie_is_net_pan_mode())
                    {
                        app_event_send_msg(APP_EVT_CLOSE_BLUETOOTH, 0);
                    }
                }

                case DBEVT_WIFI_STATION_DISCONNECTED:
                {
                    LOGI("DBEVT_WIFI_STATION_DISCONNECTED\n");
                }
                break;

                case DBEVT_WIFI_SOFT_AP_TURNING_ON:
                {
                    LOGI("DBEVT_WIFI_SOFT_AP_TURNING_ON\n");
                }
                break;

                case DBEVT_BLE_DISABLE:
                {
                    LOGI("close bluetooth ing\n");
#if CONFIG_BLUETOOTH
                    bk_genie_boarding_deinit();
                    bk_bluetooth_deinit();
                    LOGI("close bluetooth finish!\r\n");
#endif
                }
                break;

                case DBEVT_EXIT:
                    goto exit;
                    break;

                case DBEVT_NET_PAN_REQUEST:
                {
                    LOGI("DBEVT_NET_PAN_REQUEST\n");
                    int status = 1;
#if CONFIG_NET_PAN
                    bk_bt_enter_pairing_mode();
                    status = 0;
#endif
                    uint8_t bt_mac[6];
                    bk_get_mac(bt_mac, MAC_TYPE_BLUETOOTH);
                    bk_genie_boarding_event_notify_with_data(BOARDING_OP_NET_PAN_START, status, (char *)bt_mac, 6);
                }
                break;

                default:
                    break;
            }
        }
    }

exit:
    /* delate msg queue */
    ret = rtos_deinit_queue(&db_info->queue);

    if (ret != kNoErr)
    {
        LOGE("delate message queue fail\n");
    }

    db_info->queue = NULL;

    LOGE("delate message queue complete\n");

    /* delate task */
    rtos_delete_thread(NULL);

    db_info->thd = NULL;

    LOGE("delate task complete\n");
}


void bk_genie_core_init(void)
{
    bk_err_t ret = BK_OK;

    if (db_info == NULL)
    {
        db_info = os_malloc(sizeof(bk_genie_info_t));

        if (db_info == NULL)
        {
            LOGE("%s, malloc db_info failed\n", __func__);
            goto error;
        }

        os_memset(db_info, 0, sizeof(bk_genie_info_t));
    }


    if (db_info->queue != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, db_info->queue allready init, exit!\n", __func__);
        goto error;
    }

    if (db_info->thd != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, db_info->thd allready init, exit!\n", __func__);
        goto error;
    }

    ret = rtos_init_queue(&db_info->queue,
                          "db_info->queue",
                          sizeof(bk_genie_msg_t),
                          10);

    if (ret != BK_OK)
    {
        LOGE("%s, ceate doorbell message queue failed\n");
        goto error;
    }

    ret = rtos_create_thread(&db_info->thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "db_info->thd",
                             (beken_thread_function_t)bk_genie_message_handle,
                             2560,
                             NULL);

    if (ret != BK_OK)
    {
        LOGE("create media major thread fail\n");
        goto error;
    }

    db_info->enabled = BK_TRUE;

    LOGE("%s success\n", __func__);

    return;

error:

    LOGE("%s fail\n", __func__);
}

