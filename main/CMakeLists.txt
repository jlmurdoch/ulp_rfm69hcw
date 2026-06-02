if(WITH_LVGL_DISPLAY)
    set (lvgl_display_components "esp_lcd")
endif()
idf_component_register(SRCS "main.c" "lvgl/lvgl_ui.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_spi esp_driver_gpio esp_timer rfm69hcw ulp ${lvgl_display_components})
set(ulp_app_name ulp_rfm_dio2_data)
set(ulp_s_sources "ulp/rfm_dio2_data.S")
set(ulp_exp_dep_srcs "main.c")
ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")