/**
 * @file lvgl_ui.h
 *
 */

#ifndef LVGL_UI_H
#define LVGL_UI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LV_CMD_PLAY = 0,
} lv_avi_cmd_type_e;

typedef struct {
    lv_avi_cmd_type_e cmd_type;
    void *cmd_data;
} lv_avi_command_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LVGL_UI_H*/