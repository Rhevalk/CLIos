idf_component_register(SRCS 

                            "hdw_gpio.c"
                            "hdw_pwm.c"
                            "hdw_adc.c"
                            "hdw_dac.c"

                    INCLUDE_DIRS "." "include"
                    REQUIRES global lua driver esp_adc)
