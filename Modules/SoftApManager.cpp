#include <string.h>
#include <espressif/esp_common.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <dhcpserver.h>
#include <lwip/api.h>
#include <cstring>
#include "lwip/err.h"
#include "ipv4/lwip/ip_addr.h"
#include "lwip/api.h"
#include <algorithm>
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "StorageManager.cpp"

#define TELNET_PORT 23
#define WEB_PORT 50007

//#define pkt_size 256

const char* AP_SSID = "esp-open-rtos AP 666";
char AP_PSK[11] = "hello-moto";

class SoftAP {

    public:  

    StorageManager* storageManager = NULL;

        void initialize(StorageManager* sm)
        {
            
            storageManager = sm;

            printf("testing sm object\n");
            sm->test_object();

            printf("initializing SoftAP");

            sdk_wifi_set_opmode(SOFTAP_MODE);

            printf("Set opmode success");

            struct ip_info ap_ip;
            IP4_ADDR(&ap_ip.ip, 192, 16, 0, 1);
            IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
            IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
            sdk_wifi_set_ip_info(1, &ap_ip);

            printf("Set ip info success");

            struct sdk_softap_config ap_config;

            printf("declared ap_config");

            memcpy(ap_config.ssid, AP_SSID, strlen((const char*) AP_SSID));
            memcpy(ap_config.password, AP_PSK, 11);

            printf("Completed reinterpret cast");

            ap_config.ssid_len = strlen(AP_SSID);
            ap_config.channel = 3;
            ap_config.authmode = AUTH_WPA_WPA2_PSK;
            ap_config.ssid_hidden = 0;
            ap_config.max_connection = 4;
            ap_config.beacon_interval = 100;

            printf("Created config struct");

            sdk_wifi_softap_set_config(&ap_config);

            printf("Set soft ap config success");

            ip_addr_t first_client_ip;
            IP4_ADDR(&first_client_ip, 192, 16, 0, 2);
            dhcpserver_start(&first_client_ip, 4);

            printf("Init complete... initializing telnet task");

            //xTaskCreate((void(*)(void*))&SoftAP::http_get_task, "http_get_task", 4096, NULL, 2, NULL);
            http_get_task(NULL);
        }

        bool extract_value(const char* token, char* buf, char* val){
            int b,j;
            for (int i = 0; i < strlen(buf); i++){
                b = 0;
                j = i;
                while (buf[j] == token[b]){
                    if (b == (strlen(token)-1)){
                        goto EXTRACT_VAL;
                    }
                    b++;
                    j++;
                }

            }

            printf("token not found in recieved data\n");
            return false;

EXTRACT_VAL:
            b = 0;
            for (int i = j+1; (i < strlen(buf)) && (buf[i] != ','); i++){
                val[b++] = buf[i]; 
                j = i-1;
            }
            val[b] = '\0';
            printf("Extracted %s: %s\n",token, val);
            return true;
        }

        bool parse_config(char* outbuf){
            // extract and save vals
            storageManager->format_fs(); // switch to a 2 file system in which previous set is formatted only after new set is confirmed
            char buf[30];
            if (!extract_value("ssid_key::",outbuf,buf)){
                return false;
            }
            else{
               storageManager->store_ssid(buf);                 
               storageManager->retrieve_ssid(buf);
            }
            if (!extract_value("pass_key::",outbuf,buf)){
                return false;
            }
            else{
               storageManager->store_pass(buf);                 
               storageManager->retrieve_pass(buf);
            }
            if (!extract_value("ip_key::",outbuf,buf)){
                return false;
            }
            else{
               storageManager->store_ip(buf);                 
               storageManager->retrieve_ip(buf);
            }
            if (!extract_value("port_key::",outbuf,buf)){
                return false;
            }
            else{
               storageManager->store_port(buf);                 
               storageManager->retrieve_pass(buf);
            }
            return true;
        }

        void http_get_task(void *pvParameters)
        {

            const int pkt_size = 256;

            // Setup socket resources
            struct ip_addr my_host_ip;
            IP4_ADDR(&my_host_ip, 192, 16, 0, 2);


            struct sockaddr_in my_sockaddr_in;
            my_sockaddr_in.sin_addr.s_addr = my_host_ip.addr;
            my_sockaddr_in.sin_len = sizeof(struct sockaddr_in);
            my_sockaddr_in.sin_family = AF_INET;
            my_sockaddr_in.sin_port = htons(WEB_PORT);
            std::fill_n(my_sockaddr_in.sin_zero, 8, (char)0x0);


            struct addrinfo res;
            res.ai_addr = (struct sockaddr*) (void*)(&my_sockaddr_in);
            res.ai_addrlen = sizeof(struct sockaddr_in);
            res.ai_family = AF_INET;
            res.ai_socktype = SOCK_STREAM;

            struct in_addr *addr = &((struct sockaddr_in *)res.ai_addr)->sin_addr;
            printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

            while(1) {
                //dump_heapinfo();
                int s = socket(res.ai_family, res.ai_socktype, 0);
                if(s < 0) {
                    printf("... Failed to allocate socket.\r\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    continue;
                }
                printf("... allocated socket\r\n");

                // Connect to host
                if(connect(s, res.ai_addr, res.ai_addrlen) != 0) {
                    close(s);
                    printf("... socket connect failed.\r\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    continue;
                }
                printf("... connected\r\n");

                char outbuf[pkt_size] = {0};

                uint32_t pkt_cnt = 0;

                //Send over WiFi
                while(1) {

                    // Read for Wifi network data
                    int r = read(s, outbuf, pkt_size);
                    if (r > 10){

                        if(parse_config(outbuf)){
                            return;
                        }

                    }
                    else if (r < 0){
                        printf("restarting loop\n");
                        close(s);
                        break;
                    }

                    else {
                        printf("Received only %d bytes\n",r);
                    }



                    vTaskDelay((int)(1000/(float)portTICK_PERIOD_MS));
                    pkt_cnt++;
                }
            }
        }
};
