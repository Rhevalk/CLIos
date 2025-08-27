idf_component_register(SRCS 

                        "ble_ble.c"
      
                    INCLUDE_DIRS "." "include"
                    REQUIRES global lua driver bt nvs_flash)
