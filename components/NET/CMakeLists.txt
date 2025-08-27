idf_component_register(SRCS 

                            "net_wifi.c"
                            "net_net.c"
                            "net_http.c"
                            "net_httpd.c"
                            "net_websocket.c"
                            "net_mdns.c"
                            "net_tls.c"
                            "net_socket.c"

                            
                    INCLUDE_DIRS "." "include"
                    REQUIRES global lua driver nvs_flash mdns 
                    esp_event esp_netif esp_wifi esp_timer esp_http_client esp_http_server esp_websocket_client)


