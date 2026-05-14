idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_spi esp_driver_gpio esp_timer rfm69hcw ulp)

set(ulp_app_name ulp_rfm_dio2_data)
set(ulp_s_sources "ulp/rfm_dio2_data.S")
set(ulp_exp_dep_srcs "main.c")
ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")