#ifndef _STO_MAN_GRD
#define _STO_MAN_GRD

#include <string.h>
#include <espressif/esp_common.h>
#include <FreeRTOS.h>
#include <task.h>
#include <cstring>
#include "ipv4/lwip/ip_addr.h"
#include <algorithm>
#include "fcntl.h"
#include "unistd.h"
#include "spiffs.h"
#include "esp_spiffs.h"
#include "esp8266.h"
#include <stdlib.h>

class StorageManager {

    public:  

        StorageManager(){}

        void test_object(){
            printf("test of StorageManager success\n");
        }

        void initialize(void)
        {
            _inititalize(NULL);
        }

        ///////////////////////////////////////////////////
        // SSID 
        ///////////////////////////////////////////////////
        char* ssid_file = "ssid.txt";
        void store_ssid(char* ssid){
            printf("Storing ssid: %s\n",ssid);
            spiffs_write_file(ssid_file, ssid);
        }
        int retrieve_ssid(char* val){
            return read_file_spiffs(ssid_file,val,30); 
        }       

        ///////////////////////////////////////////////////
        // PASS 
        ///////////////////////////////////////////////////
        char* key_file = "pass.txt";
        void store_pass(char* key){
            printf("Storing pass: %s\n",key);
            spiffs_write_file(key_file, key);
        }
        int retrieve_pass(char* val){
            return read_file_spiffs(key_file,val,30); 
        }

        ///////////////////////////////////////////////////
        // IP 
        ///////////////////////////////////////////////////
        char* ip_file = "ip.txt";
        void store_ip(char* key){
            printf("Storing ip: %s\n",key);
            spiffs_write_file(ip_file, key);
        }
        int retrieve_ip(char* val){
            return read_file_spiffs(ip_file,val,30); 
        }

        ///////////////////////////////////////////////////
        // port 
        ///////////////////////////////////////////////////
        char* port_file = "port.txt";
        void store_port(char* key){
            printf("Storing port: %s\n",key);
            spiffs_write_file(port_file, key);
        }
        int retrieve_port(char* val){
            return read_file_spiffs(port_file,val,30); 
        }









        void example_read_file_spiffs()
        {
            const int buf_size = 0xFF;
            uint8_t bufx[buf_size];

            spiffs_file fd = SPIFFS_open(&fs, "shit.txt", SPIFFS_RDONLY, 0);
            if (fd < 0) {
                printf("Error opening file\n");
                int xerrno = SPIFFS_errno(&fs);
                printf("errno: %d\n",xerrno);
                return;
            }

            int read_bytes = SPIFFS_read(&fs, fd, bufx, buf_size);
            printf("Read %d bytes\n", read_bytes);

            bufx[read_bytes] = '\0';    // zero terminate string
            printf("Data: %s\n", bufx);

            SPIFFS_close(&fs, fd);
        }

        int read_file_spiffs(char* file, char* bufx, int buf_size)
        {
            spiffs_file fd = SPIFFS_open(&fs, file, SPIFFS_RDONLY, 0);
            if (fd < 0) {
                printf("Error opening file\n");
                int xerrno = SPIFFS_errno(&fs);
                printf("errno: %d\n",xerrno);
                return -1;
            }

            int read_bytes = SPIFFS_read(&fs, fd, bufx, buf_size);
            printf("Read %d bytes\n", read_bytes);

            // Replace '*' with null terminator
            for (int i = 0; i < read_bytes; i++){
                if (bufx[i] == '*') bufx[i] = '\0';
            }

            bufx[read_bytes] = '\0';    // zero terminate string
            printf("Data: %s\n", bufx);

            SPIFFS_close(&fs, fd);

            return read_bytes;
        }

        void spiffs_write_file(const char* file, char* buf)
        {

            printf("\nWriting cusom string\n");

            spiffs_file fd = SPIFFS_open(&fs, file, SPIFFS_O_WRONLY | SPIFFS_O_CREAT, 0);

            if (fd < 0) {
                printf("Error opening file\n");
                int xerrno = SPIFFS_errno(&fs);
                printf("errno: %d\n",xerrno);
                return;
            }

            printf("writing size %d string: %s\n",strlen(buf),buf); 

            int written = SPIFFS_write(&fs, fd, buf, strlen(buf));
            printf("Written %d bytes\n", written);

            SPIFFS_close(&fs, fd);
        }

        void write_file(char buf[])
        {

            printf("\nWriting cusom string\n");

            spiffs_file fd = SPIFFS_open(&fs, "shit.txt", SPIFFS_O_WRONLY | SPIFFS_O_CREAT, 0);

            if (fd < 0) {
                printf("Error opening file\n");
                int xerrno = SPIFFS_errno(&fs);
                printf("errno: %d\n",xerrno);
                return;
            }

            printf("writing size %d string: %s\n",strlen(buf),buf); 

            int written = SPIFFS_write(&fs, fd, buf, strlen(buf));
            printf("Written %d bytes\n", written);

            SPIFFS_close(&fs, fd);
        }

        void example_fs_info()
        {
            uint32_t total, used;
            SPIFFS_info(&fs, &total, &used);
            printf("Total: %d bytes, used: %d bytes", total, used);
        }

        void format_fs() {

            if (esp_spiffs_mount() != SPIFFS_OK) {
                printf("Error mount SPIFFS\n");
            }
            else {
                printf("spiffs mount success\n");
            }

            SPIFFS_unmount(&fs);  // FS must be unmounted before formating
            if (SPIFFS_format(&fs) == SPIFFS_OK) {
                printf("Format complete\n");
            } else {
                printf("Format failed\n");
            }
            esp_spiffs_mount();

            if (esp_spiffs_mount() != SPIFFS_OK) {
                printf("Error mount SPIFFS\n");
            }
            else {
                printf("SPIFFS mount success\n");
            }
        }

    private:



        void _inititalize(void *pvParameters)
        {

#if SPIFFS_SINGLETON == 1
            esp_spiffs_init();
            printf("calling spiffs init SINGLETON\n");
#else
            // for run-time configuration when SPIFFS_SINGLETON = 0
            esp_spiffs_init(0xf00000, 0x10000);
            printf("calling spiffs init SPECIAL\n");
#endif

            if (esp_spiffs_mount() != SPIFFS_OK) {
                printf("Error mount SPIFFS\n");
            }
            else {
                printf("SPIFFS mount success\n");
            }

        }
};
#endif
