idf_component_register(SRCS 
                            "fos_task.c"
                            "fos_queue.c"
                            "fos_mutex.c"
                            "fos_semaphore.c"
                    INCLUDE_DIRS "." "include"
                    REQUIRES global lua driver)
