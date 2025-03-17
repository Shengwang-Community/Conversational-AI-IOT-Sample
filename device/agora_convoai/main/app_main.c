#include <common/sys_config.h>
#include <components/log.h>
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <string.h>

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "media_service.h"
#include <driver/pwr_clk.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#if CONFIG_BUTTON
#include <key_main.h>
#include <key_adapter.h>
#endif
#include "sys_hal.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "bk_genie_comm.h"
#include "wifi_boarding_utils.h"
#if (CONFIG_SYS_CPU0)
#include "agora_config.h"
#include "aud_intf.h"
#include "bk_factory_config.h"
#if CONFIG_NETWORK_AUTO_RECONNECT
#include "bk_genie_smart_config.h"
#endif
#include "motor.h"
#endif

#include "app_event.h"
#include "countdown.h"
#include <led_blink.h>
#include <common/bk_include.h>
#include "components/bluetooth/bk_dm_bluetooth.h"
extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);

#define TAG "AGORA"

//#define AUTOCONNECT_WIFI

#define CONFIG_WIFI_SSID            "BEKEN-CES"//"test123"//"biubiu"//"MEGSCREEN_TEST"//"cs-ruowang-2.4G"//"Carl"//"NXIOT"
#define CONFIG_WIFI_PASSWORD        "1233211234567"//"1234567890"//"87654321"//"987654321"//"wohenruo"//"12345678"//"88888888"

#ifdef CONFIG_LDO3V3_ENABLE
#ifndef LDO3V3_CTRL_GPIO
#ifdef CONFIG_LDO3V3_CTRL_GPIO
#define LDO3V3_CTRL_GPIO    CONFIG_LDO3V3_CTRL_GPIO
#else
#define LDO3V3_CTRL_GPIO    GPIO_52
#endif
#endif
#endif

#if (CONFIG_SYS_CPU0)
uint32_t volume = 7;   // volume level, not gain.
uint32_t g_volume_gain[SPK_VOLUME_LEVEL] = {0};
#endif

#if (CONFIG_SYS_CPU0)
extern void lvgl_app_init(void);

static const uint32_t s_user_value2 = 10;
const struct factory_config_t s_user_config[] = {
    {"user_key1", (void *)"user_value1", 11, BK_FALSE, 0},
    {"user_key2", (void *)&s_user_value2, 4, BK_TRUE, 4},
};
#endif

#if (CONFIG_SYS_CPU0)
// 按键 1 的回调函数
void volume_init(void)
{
	int volume_size = bk_config_read("volume", (void *)&volume, 4);
	if (volume_size != 4)
	{
		BK_LOGE(TAG, "read volume config fail, use default config volume_size:%d\n", volume_size);
	}
	if (volume > (SPK_VOLUME_LEVEL-1)) {
		volume = SPK_VOLUME_LEVEL-1;
		if (0 != bk_config_write("volume", (void *)&volume, 4))
		{
			BK_LOGE(TAG, "storage volume: %d fail\n", volume);
		}
	}

	/* SPK_GAIN_MAX * [(exp(i/(SPK_VOLUME_LEVEL-1)-1)/(exp(1)-1)] */
	uint32_t step[SPK_VOLUME_LEVEL] = {0,6,12,20,28,37,47,58,71,84,100};
	for (uint32_t i = 0; i < SPK_VOLUME_LEVEL; i++) {
		g_volume_gain[i] = SPK_GAIN_MAX * step[i]/100;
	}
}

void volume_increase()
{
    BK_LOGI(TAG, " volume up\r\n");
    if (volume == (SPK_VOLUME_LEVEL-1))
    {
        BK_LOGI(TAG, "volume have reached maximum volume: %d\n", SPK_GAIN_MAX);
        return;
    }
    if (BK_OK == bk_aud_intf_set_spk_gain(g_volume_gain[volume+1]))
    {
        volume += 1;
        if (0 != bk_config_write("volume", (void *)&volume, 4))
        {
            BK_LOGE(TAG, "storage volume: %d fail\n", volume);
        }
        BK_LOGI(TAG, "current volume: %d\n", volume);
    }
    else
    {
        BK_LOGI(TAG, "set volume fail\n");
    }
}

void volume_decrease()
{
    BK_LOGI(TAG, " volume down\r\n");
    if (volume == 0)
    {
        BK_LOGI(TAG, "volume have reached minimum volume: 0\n");
        return;
    }
    if (BK_OK == bk_aud_intf_set_spk_gain(g_volume_gain[volume-1]))
    {
        volume -= 1;
        if (0 != bk_config_write("volume", (void *)&volume, 4))
        {
            BK_LOGE(TAG, "storage volume: %d fail\n", volume);
        }
        BK_LOGI(TAG, "current volume: %d\n", volume);
    }
    else
    {
        BK_LOGI(TAG, "set volume fail\n");
    }
}

void power_off()
{
    BK_LOGI(TAG, " power_off\r\n");
    BK_LOGW(TAG, " ************TODO:Just force deep sleep for Demo!\r\n");
	extern bk_err_t audio_turn_off(void);
	extern bk_err_t video_turn_off(void);
	audio_turn_off();
	video_turn_off();
    bk_reboot_ex(RESET_SOURCE_FORCE_DEEPSLEEP);
}

void power_on()
{
    BK_LOGI(TAG, "power_on\r\n");
}

void ai_agent_config()
{
    //BK_LOGW(TAG, " ************TODO:AI Agent doesn't complete!\r\n");
}

/*Do not execute blocking or time-consuming long code in event handler
 functions. The reason is that key_thread processes messages in a
 single task in sequence. If a handler function blocks or takes too
 long to execute, it will cause subsequent key events to be responded to untimely.*/
static void handle_system_event(key_event_t event)
{
    uint32_t time;
    switch (event)
    {
        case VOLUME_UP:
            volume_increase();
            break;
        case VOLUME_DOWN:
            volume_decrease();
            break;
        case SHUT_DOWN:
            time = rtos_get_time(); //long press more than 6s
            if (time < 9000)
            {
                break;
            }
            power_off();
            break;
        case POWER_ON:
            power_on();
            break;
        case AI_AGENT_CONFIG:
            ai_agent_config();
            break;
        case CONFIG_NETWORK:
            BK_LOGW(TAG, "Start to config network!\n");
            if(bk_bluetooth_init())
            {
                BK_LOGE(TAG, "bluetooth init err\n");
            }

            bk_genie_prepare_for_smart_config();
            break;
        case FACTORY_RESET:
            BK_LOGW(TAG, "trigger factory config reset\r\n");
            bk_factory_reset();
            bk_reboot();
            break;
        // 其他事件处理...
        default:
            break;
    }
}

KeyConfig_t key_config[] = {
        {
            .gpio_id = KEY_GPIO_13,   //corresponding to the actual key
            .active_level = LOW_LEVEL_TRIGGER,
            .short_event = VOLUME_UP,
            .double_event = VOLUME_UP,	//TRICK: at shutdown mode, it can't recognize double press,short_event is really power on.(but short event is used by VOLUME UP when system is active).
            .long_event = CONFIG_NETWORK
        },
        {
            .gpio_id = KEY_GPIO_12,
            .active_level = LOW_LEVEL_TRIGGER,
            .short_event = POWER_ON,
            .double_event = POWER_ON,
            .long_event = SHUT_DOWN
        },
        {
            .gpio_id = KEY_GPIO_8,
            .active_level = LOW_LEVEL_TRIGGER,
            .short_event = VOLUME_DOWN,
            .double_event = VOLUME_DOWN,
            .long_event = FACTORY_RESET
        }
};

static void bk_key_register_wakeup_source()
{
    for (uint8_t i = 0; i < sizeof(key_config) / sizeof(KeyConfig_t); i++)
    {
        if ((key_config[i].short_event == POWER_ON) || (key_config[i].double_event == POWER_ON) || (key_config[i].long_event == POWER_ON))
        {
            if (key_config[i].active_level == LOW_LEVEL_TRIGGER)
            {
                bk_gpio_register_wakeup_source(key_config[i].gpio_id, GPIO_INT_TYPE_FALLING_EDGE);
            }
            else
            {
                bk_gpio_register_wakeup_source(key_config[i].gpio_id, GPIO_INT_TYPE_RISING_EDGE);
            }
        }
    }

}
static bk_err_t app_force_analog_audio_close()
{
	sys_hal_set_ana_reg18_value(0);
	sys_hal_set_ana_reg19_value(0);
	sys_hal_set_ana_reg20_value(0);
	sys_hal_set_ana_reg21_value(0);
	return 0;
}
static void bk_enter_deepsleep()
{
    #if CONFIG_GSENSOR_ENABLE
		extern bk_err_t gsensor_demo_lowpower_wakeup();
		gsensor_demo_lowpower_wakeup();
		rtos_delay_milliseconds(50);
    #endif

	BK_LOGI(TAG,"RESET_SOURCE_FORCE_DEEPSLEEP\r\n");
    bk_key_register_wakeup_source();
    bk_pm_clear_deep_sleep_modules_config(PM_POWER_MODULE_NAME_AUDP);
	bk_pm_clear_deep_sleep_modules_config(PM_POWER_MODULE_NAME_VIDP);
	app_force_analog_audio_close();
	bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);
    rtos_delay_milliseconds(10);
}

static void bk_wait_power_on()
{
    uint32_t press_time = 0;
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    do {
        if (bk_gpio_get_input(KEY_GPIO_12) == 0) {
            extern void delay_ms(uint32 num);
            delay_ms(500);
            press_time += 500;

            if (bk_gpio_get_input(KEY_GPIO_12) != 0) {
                break;
            }
        } else {

            break;
        }
    } while (press_time < LONG_RRESS_TIMR);
    GLOBAL_INT_RESTORE();

    if (press_time < LONG_RRESS_TIMR)
    {
        bk_key_register_wakeup_source();
        bk_enter_deepsleep();
    }

}
#endif

void user_app_main(void)
{
#if (CONFIG_SYS_CPU0)
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_480M);
#endif

}

int main(void)
{
    if (bk_misc_get_reset_reason() != RESET_SOURCE_FORCE_DEEPSLEEP)
    {
#if (CONFIG_SYS_CPU0)
        rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#endif
        bk_init();

#if (CONFIG_SYS_CPU0)
#ifdef CONFIG_LDO3V3_ENABLE
        BK_LOG_ON_ERR(gpio_dev_unmap(LDO3V3_CTRL_GPIO));
        bk_gpio_disable_pull(LDO3V3_CTRL_GPIO);
        bk_gpio_enable_output(LDO3V3_CTRL_GPIO);
        bk_gpio_set_output_high(LDO3V3_CTRL_GPIO);
#endif
#endif

    #if (CONFIG_SYS_CPU0)
    /*to judgement key is long press or short press; long press exit deepsleep*/

        if(bk_misc_get_reset_reason() == RESET_SOURCE_DEEPPS_GPIO && (bk_gpio_get_wakeup_gpio_id() == KEY_GPIO_12))
        {
            //motor vibration
            motor_open(PWM_MOTOR_CH_3);
            bk_wait_power_on();
            motor_close(PWM_MOTOR_CH_3);
        }

        bk_regist_factory_user_config((const struct factory_config_t *)&s_user_config,
                                       sizeof(s_user_config)/sizeof(s_user_config[0]));
        bk_factory_init();
    #endif


    //led init move before
    #if (CONFIG_SYS_CPU0)

        
        //No operation countdown 3 minutes to shut down
        start_countdown(countdown_ms);
        //led init move before
        led_driver_init();
        led_app_set(LED_ON_GREEN,LED_LAST_FOREVER);
        
    #endif
        media_service_init();

#if (CONFIG_SYS_CPU0)
        app_event_init();

        volume_init();

#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE
        extern bk_err_t audio_turn_on(void);
        int ret = audio_turn_on();
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "%s, %d, audio turn on fail, ret:%d\n", __func__, __LINE__, ret);
        }
#endif
#endif

#if (CONFIG_SYS_CPU0)
        bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);

        bk_genie_core_init();

#if CONFIG_NETWORK_AUTO_RECONNECT
        bk_genie_smart_config_init();
#endif

        register_event_handler(handle_system_event);
        bk_key_driver_init(key_config, sizeof(key_config) / sizeof(KeyConfig_t));

#if CONFIG_BAT_MONITOR
        extern void battery_monitor_init(void);
        battery_monitor_init();
#endif


#endif

#if CONFIG_USBD_MSC
        extern void msc_storage_init(void);
        msc_storage_init();
#endif
    }
    else
    {

    #if (CONFIG_SYS_CPU0)
		bk_init();
        bk_enter_deepsleep();
	#endif
}
    return 0;
}
