#include "espressif/esp_common.h"
#include "esp/uart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "ipv4/lwip/ip_addr.h"
#include "ssid_config.h"
//#include "debug_dumps.h"

#include "mbedtls/sha256.h"
#include "ota-tftp.h"
#include "rboot-api.h"

#define WEB_SERVER "marzipan-Lenovo-ideapad-Y700-15ISK"
#define WEB_PORT 50007

/* TFTP client will request this image filenames from this server */
#define TFTP_IMAGE_SERVER "192.168.1.23"
#define TFTP_IMAGE_FILENAME1 "firmware1.bin"
static const char *FIRMWARE1_SHA256 = "88199daff8b9e76975f685ec7f95bc1df3c61bd942a33a54a40707d2a41e5488";

class OtaManager{

    public:

        int run(){ 
            _initOTA();
        }

    private:

        /* Output of the command 'sha256sum firmware1.bin' */

        /* Example function to TFTP download a firmware file and verify its SHA256 before
           booting into it.
         */
        static void tftpclient_download_and_verify_file1(int slot, rboot_config *conf)
        {
            printf("Downloading %s to slot %d...\n", TFTP_IMAGE_FILENAME1, slot);
            int res = ota_tftp_download(TFTP_IMAGE_SERVER, TFTP_PORT+1, TFTP_IMAGE_FILENAME1, 1000, slot, NULL);
            printf("ota_tftp_download %s result %d\n", TFTP_IMAGE_FILENAME1, res);

            if (res != 0) {
                return;
            }

            printf("Looks valid, calculating SHA256...\n");
            uint32_t length;
            bool valid = rboot_verify_image(conf->roms[slot], &length, NULL);
            static mbedtls_sha256_context ctx;
            mbedtls_sha256_init(&ctx);
            mbedtls_sha256_starts(&ctx, 0);
            valid = valid && rboot_digest_image(conf->roms[slot], length, (rboot_digest_update_fn)mbedtls_sha256_update, &ctx);
            static uint8_t hash_result[32];
            mbedtls_sha256_finish(&ctx, hash_result);
            mbedtls_sha256_free(&ctx);

            if(!valid)
            {
                printf("Not valid after all :(\n");
                return;
            }

            printf("Image SHA256 = ");
            valid = true;
            for(int i = 0; i < sizeof(hash_result); i++) {
                char hexbuf[3];
                snprintf(hexbuf, 3, "%02x", hash_result[i]);
                printf(hexbuf);
                if(strncmp(hexbuf, FIRMWARE1_SHA256+i*2, 2))
                    valid = false;
            }
            printf("\n");

            if(!valid) {
                printf("Downloaded image SHA256 didn't match expected '%s'\n", FIRMWARE1_SHA256);
                return;
            }

            printf("SHA256 Matches. Rebooting into slot %d...\n", slot);
            rboot_set_current_rom(slot);
            sdk_system_restart();
        }

        void tftp_client_task(void *pvParameters)
        {
            printf("TFTP client task starting...\n");
            rboot_config conf;
            conf = rboot_get_config();
            int slot = (conf.current_rom + 1) % conf.count;
            printf("Image will be saved in OTA slot %d.\n", slot);
            if(slot == conf.current_rom) {
                printf("FATAL ERROR: Only one OTA slot is configured!\n");
                while(1) {}
            }

            while(1) {
                tftpclient_download_and_verify_file1(slot, &conf);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
        }

        void _initOTA(){
            ota_tftp_init_server(TFTP_PORT);
            //tftp_client_task(NULL);
            xTaskCreate((void(*)(void*))&OtaManager::tftp_client_task, "tftp_client", 2048, NULL, 2, NULL);
        }
};
