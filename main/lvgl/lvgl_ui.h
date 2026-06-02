#ifndef LVGL_UI_H
#define LVGL_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_panel_st7789.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"

#define LVGL_HRES 135
#define LVGL_VRES 240
#define LVGL_HGAP 40
#define LVGL_VGAP 52

static const char *TAG = "RFM69HCW";

extern lv_subject_t temp_subj;
extern lv_subject_t relh_subj;

esp_err_t lvgl_display_init(lv_display_t *disp_handle);
void lvgl_main(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LVGL_UI_H