#include <os/os.h>
#include "lcd_act.h"
#include "media_app.h"
#include "media_evt.h"
#include "lvgl_ui.h"

uint8_t lvgl_app_init_flag = 0;

static lcd_open_t lcd_open =
{
    .device_ppi = PPI_160X160,
    .device_name = "gc9d01"
};

static lv_avi_command_t lv_comand = 
{
    .cmd_type = LV_CMD_PLAY,
    .cmd_data = "/genie_eye.avi"
};

void lvgl_app_init(void)
{
    bk_err_t ret;

    if (lvgl_app_init_flag == 1)
    {
        os_printf("lvgl_app_init has inited\r\n");
        return;
    }

    ret = media_app_lvgl_open((lcd_open_t *)&lcd_open);
    if (ret != BK_OK)
    {
        os_printf("media_app_lvgl_open failed\r\n");
        return;
    }

    lvgl_app_init_flag = 1;
}

void lvgl_app_play(char *avi_name)
{
    bk_err_t ret;

    if (lvgl_app_init_flag == 0)
    {
        lvgl_app_init();

        if (lvgl_app_init_flag == 0) {
        os_printf("lvgl_app_deinit has deinited or init failed\r\n");
        return;
        }
    }

    lv_comand.cmd_data = (void *)avi_name;
    ret = media_app_lvgl_send_data((void *)&lv_comand);
    if (ret != BK_OK)
    {
        os_printf("media_app_lvgl_close failed\r\n");
        return;
    }
}

void lvgl_app_deinit(void)
{
    bk_err_t ret;

    if (lvgl_app_init_flag == 0)
    {
        os_printf("lvgl_app_deinit has deinited or init failed\r\n");
        return;
    }

    ret = media_app_lvgl_close();
    if (ret != BK_OK)
    {
        os_printf("media_app_lvgl_close failed\r\n");
        return;
    }

    lvgl_app_init_flag = 0;
}

