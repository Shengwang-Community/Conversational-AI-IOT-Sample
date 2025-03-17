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
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <string.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "components/webclient.h"
#include "cJSON.h"
#include "components/bk_uid.h"
#include "bk_genie_comm.h"
#include "wifi_boarding_utils.h"
#include "bk_genie_smart_config.h"
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
#include "bk_ef.h"
#endif
#include "pan_service.h"
#include "led_blink.h"
#include "app_event.h"
#include "boarding_service.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define TAG "bk_sconf"
#define RCV_BUF_SIZE            256
#define SEND_HEADER_SIZE           1024
#define POST_DATA_MAX_SIZE  1024
#define MAX_URL_LEN         256

bool smart_config_running = false;
char *channel_name_record = NULL;
static beken2_timer_t network_reconnect_tmr = {0};
extern bool agora_runing;
uint8_t network_disc_evt_posted = 0;

void network_reconnect_check_status(void)
{
  if (smart_config_running == false)
  {
    BK_LOGW(TAG,"reconnect timeout!\n");
    if (network_disc_evt_posted == 0) {
        app_event_send_msg(APP_EVT_RECONNECT_NETWORK_FAIL, 0);
        network_disc_evt_posted = 1;
    }
  }
}

void network_reconnect_start_timeout_check(uint32_t timeout)
{
  bk_err_t err = kNoErr;
  uint32_t clk_time;

  clk_time = timeout*1000;		//timeout unit: seconds

  if (rtos_is_oneshot_timer_init(&network_reconnect_tmr)) {
     BK_LOGI(TAG,"network provisioning status timer reload\n");
    rtos_oneshot_reload_timer(&network_reconnect_tmr);
  } else {
    err = rtos_init_oneshot_timer(&network_reconnect_tmr, clk_time, (timer_2handler_t)network_reconnect_check_status, NULL, NULL);
    BK_ASSERT(kNoErr == err);

    err = rtos_start_oneshot_timer(&network_reconnect_tmr);
    BK_ASSERT(kNoErr == err);
     BK_LOGI(TAG,"network provisioning status timer:%d\n", clk_time);
  }

  return;
}


void network_reconnect_stop_timeout_check(void)
{
  bk_err_t ret = kNoErr;

  if (rtos_is_oneshot_timer_init(&network_reconnect_tmr)) {
    if (rtos_is_oneshot_timer_running(&network_reconnect_tmr)) {
      ret = rtos_stop_oneshot_timer(&network_reconnect_tmr);
      BK_ASSERT(kNoErr == ret);
    }

    ret = rtos_deinit_oneshot_timer(&network_reconnect_tmr);
    BK_ASSERT(kNoErr == ret);
  }
}

int is_wifi_sta_auto_restart_info_saved(void)
{
	BK_FAST_CONNECT_D info = {0};
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
	bk_get_env_enhance("d_network_id", (void *)&info, sizeof(BK_FAST_CONNECT_D));
#endif
	if (info.flag == 0x71l)
		return 0;
	else
		return 1;
}

int bk_genie_is_net_pan_mode(void)
{
	BK_FAST_CONNECT_D info = {0};
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
	bk_get_env_enhance("d_network_id", (void *)&info, sizeof(BK_FAST_CONNECT_D));
#endif

#if CONFIG_NET_PAN
	if (info.flag == 0x74l)
	{
		return 1;
	}
	else
#endif
	{
		return 0;
	}
}

void demo_erase_network_auto_reconnect_info(void)
{
	BK_FAST_CONNECT_D info_tmp = {0};
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
	bk_set_env_enhance("d_network_id", (const void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#endif
}

extern int demo_sta_app_init(char *oob_ssid, char *connect_key);
extern int demo_softap_app_init(char *ap_ssid, char *ap_key, char *ap_channel);
int demo_network_auto_reconnect(void)
{
	BK_FAST_CONNECT_D info = {0};
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
	bk_get_env_enhance("d_network_id", (void *)&info, sizeof(BK_FAST_CONNECT_D));
#endif
	/*0x01110001:sta, 0x01110010:softap, 0x01110100:pan*/
	if (info.flag == 0x71l) {
		network_reconnect_stop_timeout_check();
		network_reconnect_start_timeout_check(30);    //30s
		app_event_send_msg(APP_EVT_RECONNECT_NETWORK, 0);
		network_disc_evt_posted = 0;
		demo_sta_app_init((char *)info.sta_ssid, (char *)info.sta_pwd);
	}
	if (info.flag == 0x72l)
		demo_softap_app_init((char *)info.ap_ssid, (char *)info.ap_pwd, NULL);
#if CONFIG_NET_PAN
	if (info.flag == 0x74l) {
		network_reconnect_stop_timeout_check();
		network_reconnect_start_timeout_check(30);    //30s
		app_event_send_msg(APP_EVT_RECONNECT_NETWORK, 0);
		pan_service_init();
		bt_start_pan_reconnect();
	}
#endif
	return info.flag;
}

int demo_save_network_auto_restart_info(netif_if_t type, void *val)
{
	BK_FAST_CONNECT_D info_tmp = {0};
	__maybe_unused wifi_ap_config_t *ap_config = NULL;
	__maybe_unused wifi_sta_config_t *sta_config = NULL;
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
	bk_get_env_enhance("d_network_id", (void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#endif
	if (type == NETIF_IF_STA) {
		info_tmp.flag |= 0x71l;
		sta_config = (wifi_sta_config_t *)val;
		os_memset((char *)info_tmp.sta_ssid, 0x0, 33);
		os_memset((char *)info_tmp.sta_pwd, 0x0, 65);
		os_strcpy((char *)info_tmp.sta_ssid, (char *)sta_config->ssid);
		os_strcpy((char *)info_tmp.sta_pwd, (char *)sta_config->password);
	} else if (type == NETIF_IF_AP) {
		info_tmp.flag |= 0x72l;
		ap_config = (wifi_ap_config_t *)val;
		os_memset((char *)info_tmp.ap_ssid, 0x0, 33);
		os_memset((char *)info_tmp.ap_pwd, 0x0, 65);
		os_strcpy((char *)info_tmp.ap_ssid, (char *)ap_config->ssid);
		os_strcpy((char *)info_tmp.ap_pwd, (char *)ap_config->password);
#if CONFIG_NET_PAN
	} else if (type == NETIF_IF_PAN) {
		info_tmp.flag |= 0x74l;
#endif
	} else
		return -1;
#if (CONFIG_EASY_FLASH && CONFIG_EASY_FLASH_V4)
	bk_set_env_enhance("d_network_id", (const void *)&info_tmp, sizeof(BK_FAST_CONNECT_D));
#endif
	return 0;
}

static int bk_genie_sconf_netif_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    netif_event_got_ip4_t *got_ip;
    bk_genie_msg_t msg;
    __maybe_unused wifi_sta_config_t sta_config = {0};

    switch (event_id)
    {
        case EVENT_NETIF_GOT_IP4:
            network_disc_evt_posted = 0;
            got_ip = (netif_event_got_ip4_t *)event_data;
            BK_LOGI(TAG, "%s got ip %s.\n", got_ip->netif_if == NETIF_IF_STA ? "STA" : "BK PAN", got_ip->ip);
            if (smart_config_running)
            {
                app_event_send_msg(APP_EVT_NETWORK_PROVISIONING_SUCCESS, 0);
                bk_wifi_sta_get_config(&sta_config);
                demo_save_network_auto_restart_info(got_ip->netif_if, &sta_config);
                msg.event = DBEVT_WIFI_STATION_CONNECTED;
                bk_genie_send_msg(&msg);

                msg.event = DBEVT_START_AGORA_AGENT_START;
                bk_genie_send_msg(&msg);
            }
            else
            {
                network_reconnect_stop_timeout_check();
                app_event_send_msg(APP_EVT_RECONNECT_NETWORK_SUCCESS, 0);
            }

            break;
        default:
            BK_LOGI(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}

static int bk_genie_sconf_wifi_event_cb(void *arg, event_module_t event_module, int event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *sta_disconnected;
    wifi_event_sta_connected_t *sta_connected;
    bk_genie_msg_t msg;

    switch (event_id)
    {
        case EVENT_WIFI_STA_CONNECTED:
            sta_connected = (wifi_event_sta_connected_t *)event_data;
            BK_LOGW(TAG, "STA connected to %s\n", sta_connected->ssid);
            break;

        case EVENT_WIFI_STA_DISCONNECTED:
            sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
            BK_LOGW(TAG, "STA disconnected, reason(%d)\n", sta_disconnected->disconnect_reason);
            /*drop local generated disconnec event by user*/
            if (sta_disconnected->disconnect_reason == WIFI_REASON_DEAUTH_LEAVING &&
				sta_disconnected->local_generated == 1)
			break;
            msg.event = DBEVT_WIFI_STATION_DISCONNECTED;
            bk_genie_send_msg(&msg);
            if (network_disc_evt_posted == 0) {
                if (smart_config_running == false)
                    app_event_send_msg(APP_EVT_RECONNECT_NETWORK_FAIL, 0);
                else
                    app_event_send_msg(APP_EVT_NETWORK_PROVISIONING_FAIL, 0);
                network_disc_evt_posted = 1;
            }
            break;

        default:
            BK_LOGI(TAG, "rx event <%d %d>\n", event_module, event_id);
            break;
    }

    return BK_OK;
}


void event_handler_init(void)
{
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, bk_genie_sconf_wifi_event_cb, NULL));
    BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, bk_genie_sconf_netif_event_cb, NULL));
}

extern void agora_convoai_engine_stop();
void bk_genie_prepare_for_smart_config(void)
{
    smart_config_running = true;
    app_event_send_msg(APP_EVT_NETWORK_PROVISIONING, 0);
    network_reconnect_stop_timeout_check();
    agora_convoai_engine_stop();
    bk_wifi_sta_stop();
    demo_erase_network_auto_reconnect_info();

    extern bool ate_is_enabled(void);

    if (!ate_is_enabled())
    {
        bk_genie_boarding_init();
        wifi_boarding_adv_start();
    }
    else
    {
        BK_LOGW(TAG, "%s ATE is enable, ble will not enable!!!!!!\n", __func__);
    }

    //network_reconnect_start_timeout_check(300);	//5min
}

int bk_genie_smart_config_init(void)
{
    int flag;

    event_handler_init();
    flag = demo_network_auto_reconnect();

    if (flag != 0x71l && flag != 0x73l
#if CONFIG_NET_PAN
        && flag != 0x74l
#endif
    ) {
        bk_genie_prepare_for_smart_config();
    }
    else
    {
#if CONFIG_NET_PAN
        if (flag != 0x74l)
#endif
        {
            bk_bluetooth_deinit();
        }
    }

    return 0;
}
