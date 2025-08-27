idf_component_register(SRCS 
                            "com_uart.c"
                            "com_spi.c"
                            "com_i2c.c"
                            "com_i2s.c"
                            "com_can.c"
                            "com_rmt.c"
                    INCLUDE_DIRS "." "include"
                    REQUIRES global lua driver)
