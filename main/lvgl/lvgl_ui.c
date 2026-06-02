#include "lvgl_ui.h"

lv_subject_t temp_subj;
lv_subject_t relh_subj;

esp_err_t lvgl_display_init(lv_display_t *disp_handle) {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,         /* LVGL task priority */
        .task_stack = 6144,         /* LVGL task stack size */
        .task_affinity = -1,        /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5        /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = GPIO_NUM_39,
        .cs_gpio_num = GPIO_NUM_7,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .pclk_hz = (80 * 1000 * 1000)
    };
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    // Panel Setup
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_40,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    assert(panel_handle);

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_set_gap(panel_handle, LVGL_HGAP, LVGL_VGAP);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LVGL_HRES * LVGL_VRES,
        .double_buffer = false,
        .hres = LVGL_VRES,
        .vres = LVGL_HRES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = true,
            .mirror_x = false,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = false,
            .swap_bytes = false,
        }
    };

    disp_handle = lvgl_port_add_disp(&disp_cfg);
    assert(disp_handle);

    return ESP_OK;
}

void lvgl_main(void) {
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    // Temperature Label
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Temperature Text
    lv_subject_init_float(&temp_subj, 0.0);
    lv_label_bind_text(label, &temp_subj, "Temp: %02.1f°C");

    // Relative Humidity Label
    lv_obj_t *relh_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(relh_label, &lv_font_montserrat_24, LV_PART_MAIN);    
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(relh_label, LV_ALIGN_LEFT_MID, 0, 0);

    // Relative Humidity Text
    lv_subject_init_int(&relh_subj, 0);
    lv_label_bind_text(relh_label, &relh_subj, "Humidity: %d%%");

}