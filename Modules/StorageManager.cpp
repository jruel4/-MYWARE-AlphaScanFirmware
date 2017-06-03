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

        /*
        WiFi Caching API
        */
        bool init_cache_conditional(void){
            /*
            Calls init cache only if cache files have not already been created.
            */

            // Check for a* cache file. Only checking for last expected file, we should do something more thorough than this.
            char* fname = "f_1874";

            spiffs_file fd = SPIFFS_open(&fs, fname, SPIFFS_RDONLY, 0);
            if (fd < 0) {
                // Assuming this means cache files have not been created
                printf("Cache files not found. Proceeding to initialization.\n");
                // NOTE: should be checcking errno here for specific value
                int xerrno = SPIFFS_errno(&fs);
                return init_cache();
            }
            else{
                // Assuming this means cache files already initialized
                printf("Cache files found, skipping initialization.\n");
                int read_bytes = SPIFFS_read(&fs, fd, buf, mPktSize);
                SPIFFS_close(&fs, fd);
            }
        }

        bool init_cache(void){
            /*
               Create 60 seconds worth of sample cache files i.e. 60*250/8 = 1875 files with each file being 240 bytes (8 sample worth)
               This will occupy 450,000 bytes + the space needed to store references.
             */

            char fname[6] = {0};
            char buf[mPktSize] = {0};
            spiffs_file fd;

            for (int i = 0; i < mNumCacheFiles; i++){
                // generate file name according to schema f_n : n = [0,num_files]

                sprintf(fname, "f_%4d", i);
                fd = SPIFFS_open(&fs, fname, SPIFFS_O_WRONLY | SPIFFS_O_CREAT, 0);

                if (fd < 0) {
                    printf("Error opening file: %s\n", fname);
                    int xerrno = SPIFFS_errno(&fs);
                    printf("errno: %d\n",xerrno);
                    return;
                }

                int written = SPIFFS_write(&fs, fd, buf, mPktSize);

                if (writing != 240){
                    printf("Only wrote %d bytes on file %s\n", writing, fname);
                    return;
                }

                SPIFFS_close(&fs, fd);
            }
        }

        bool write_cache(char[mPktSize] pkt){
            /* 
               Cache pkt of 8 samples i.e. 240 byte buffer
             */


            // Perform cache spreading logic
            if (muc < mNumCacheFiles){
                // incrememnt endpointer
                mep = incrementCachePointer(mep);
                // Write to pkt to file
                char fname[6] = {0};
                sprintf(fname, "f_%4d", mep);
                spiffs_file fd = SPIFFS_open(&fs, fname, SPIFFS_O_WRONLY | SPIFFS_O_CREAT, 0);
                if (fd < 0) {
                    printf("Error opening file\n");
                    int xerrno = SPIFFS_errno(&fs);
                    printf("errno: %d\n",xerrno);
                    return false;
                }
                int written = SPIFFS_write(&fs, fd, pkt, mPktSize);
                SPIFFS_close(&fs, fd);
                // increment used count
                muc++;
                return true;
            }
            else{
                // no more space to cache files
                return false;
            }
        }

        bool read_cache(char[mPktSize] buf){
            /*
               Read pkt from cache (if available)
             */

            // Perfrom cache logic
            if (muc > 0){

                // Read from begin pointer
                char fname[6] = {0};
                sprintf(fname, "f_%4d", mbp);
                spiffs_file fd = SPIFFS_open(&fs, fname, SPIFFS_RDONLY, 0);
                if (fd < 0) {
                    printf("Error opening file\n");
                    int xerrno = SPIFFS_errno(&fs);
                    printf("errno: %d\n",xerrno);
                    return false;
                }
                int read_bytes = SPIFFS_read(&fs, fd, buf, mPktSize);
                SPIFFS_close(&fs, fd);

                // Increment begin point
                mbp = incrementCachePointer(mbp);

                // Decrement used count
                muc--;

            }
            else{
                // No cached data
                return false;
            }
        }

    private:

        // Class member variables
        const int mNumCacheFiles = 1875;
        const int mPktSize = 240;
        int mbp = 0; // begin pointer
        int mep = 0; // end pointer
        int muc = 0; // used count

        int incrementCachePointer(int val){
            return (val + 1) % mNumCacheFiles;
        }

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
