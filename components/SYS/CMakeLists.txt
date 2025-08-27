idf_component_register(SRCS 
                            "sys_os.c"
                            "sys_littlefs.c"
                            
                    INCLUDE_DIRS "." "include"
                    REQUIRES global lua driver littlefs)
