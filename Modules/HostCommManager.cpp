#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "esp/timer.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
//#include "../../../../AlphaScanRTOS/lwip/lwip/src/api/sockets.c"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "ssid_config.h"
#include "ipv4/lwip/ip_addr.h"
#include <algorithm>
#include "ads_ctrl.cpp"
#include <cstdlib>
#include "lwip/api.h"
#include "StorageManager.cpp"


#define WEB_SERVER "marzipan-Lenovo-ideapad-Y700-15ISK"
//#define WEB_PORT 50007
#define SAMPLE_SIZE (29)
#define SAMPLES_PER_PACKET (4*7)
#define PACKET_SIZE (SAMPLE_SIZE * SAMPLES_PER_PACKET)

//JCR LAZINESS
unsigned int old_nbset = 0, nbset = 0;
#define NB_ON 1
#define NB_OFF 0
#define TCP_NB_SET(x) {old_nbset = nbset; nbset = x; lwip_ioctl(mSocket, FIONBIO, &nbset); }
#define TCP_NB_UNSET() {nbset = old_nbset; lwip_ioctl(mSocket, FIONBIO, &nbset); }

#define WIFI_SSID "MartianWearablesLLC"
#define WIFI_PASS "phobicbird712"

class HostCommManager {


	public:

		int initialize(StorageManager* sm){
			return _initialize(sm);
		}

		int update(){
			int rcode = _process_tcp_command();
			if (rcode < 0){
				 return _establish_host_connection();
			}
			return rcode;
		}

		void stream_ads(ADS* ads){
			//_stream_ads(ads);
            _stream_task(ads);
		}


        void send_ads_registers(ADS* ads) {
            int nbset = 0;
            vTaskDelay((TickType_t)8); //Delay so that we send to correct function
            memcpy(mOutbuf, "bbb", 3);
            memcpy((mOutbuf + 27), "eee", 3);       
            ads->receiveRegisterMapFromADS();
            ads->copyRegisterMapToArray(mOutbuf+3);
            TCP_NB_SET(NB_OFF);
            write(mSocket, mOutbuf, 30);
            TCP_NB_UNSET();
            ads->printSerialRegistersFromADS();
        }

        void receive_ads_registers(ADS* ads) {
            int nbset = 0;
            printf("In E\n");
            TCP_NB_SET(NB_OFF);
            int r = read(mSocket, mInbuf, 30); //TODO - blocking or non-blocking?
            TCP_NB_UNSET();
            printf("r=%d in E\n", r);
            for(int jj = 0; jj < r; ++jj) printf("%c ", mInbuf[jj]);
            //TODO More robust checking here
            if (r >= 30)
            {
                int a = 0;
                for(int i = 0; i < 3; ++i)
                {                                                       
                    while(a != 1 && i+30 < r) { if(mInbuf[i] == 'e') {a=1; break;} else ++i;}
                    if(mInbuf[i] != 'b' || mInbuf[i+27] != 'e')
                    {
                        printf("Error, invalid received data.\n");
                        for(int j = 0; j < r; ++j) printf("%c ", mInbuf[j]);
                        return;
                    }
                }
                ads->printSerialRegistersFromADS();
                ads->receiveRegisterMapFromArray(mInbuf+3);
                ads->flushRegisterMapToADS();                                           
                ads->printSerialRegistersFromADS();
            }
        }

	private:

		// Variables
		int mSocket = -1;
		unsigned char mOutbuf[PACKET_SIZE] = {0};
		unsigned char mInbuf[PACKET_SIZE] = {0};
		int mKeepAliveCounter = 0;
		const static int TEST_BUF_LEN = 256;
		char test_outbuf[TEST_BUF_LEN] = {0};
        static const int pkt_size = 1400;
        static const int inbuf_size = 3;
        struct ip_addr mHostIp;
        int mHostPort;
        StorageManager* storageManager = NULL;
        bool mFirstConnectAttempt = true;

        bool parse_quartet(char* ipstring, int* ipq){
            int j = 0;
            for (int i = 0; i < 4; i++){
                char tmp[6];
                int n = 0;
                while((ipstring[j] != '.') && (j < strlen(ipstring)) ){
                    tmp[n++] = ipstring[j++];
                }
                j++;
                tmp[n] = '\0';
                ipq[i] = atoi(tmp);
                printf("Quartet idx: %d, val: %d\n",i,atoi(tmp));
            }
            return true;
        }

        int retrieveHostParams(){
            char host_ip[30];
            int ip_len = storageManager->retrieve_ip(host_ip);
            if (ip_len > 3){
                // part ip quartet out of string
                int ipq[4];
                if (!parse_quartet(host_ip, ipq)){
                    printf("Failed to extract valid ip quartet\n");
                }
                IP4_ADDR(&mHostIp, ipq[0], ipq[1], ipq[2], ipq[3]);
            }
            else {
                printf("No valid ip key found in memory\n");
                return -1;
            }

            char host_port[30];
            int port_len = storageManager->retrieve_port(host_port);
            if (port_len > 3){
                printf("Converted port string to %d\n", atoi(host_port));
                mHostPort = atoi(host_port);
            }
            else {
                printf("No valid ip key found in memory\n");
                return -1;
            }
            return 0;
        }



        // Connect to host
        int _establish_host_connection(){

            // run check on output
            if (retrieveHostParams() < 0){
                printf("Failed to retrieve valid host params\n");
                return 0;
            }

            struct addrinfo res;
            struct sockaddr_in my_sockaddr_in;
            my_sockaddr_in.sin_addr.s_addr = mHostIp.addr;
            my_sockaddr_in.sin_len = sizeof(struct sockaddr_in);
            my_sockaddr_in.sin_family = AF_INET;
            my_sockaddr_in.sin_port = htons(mHostPort);
            std::fill_n(my_sockaddr_in.sin_zero, 8, (char)0x0);

            res.ai_addr = (struct sockaddr*) (void*)(&my_sockaddr_in);
            res.ai_addrlen = sizeof(struct sockaddr_in);
            res.ai_family = AF_INET;
            res.ai_socktype = SOCK_STREAM;

            struct in_addr *addr = &((struct sockaddr_in *)res.ai_addr)->sin_addr;
            printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

            int nbset = 1;
            int ctlr = -2;
            int optval = 1;
            // Only attempt for 10 seconds, otherwise restart...
            int retry_max = (int)(30.0/0.300);
            int retry_ctr = 0;

            printf("Attempting to establish host connection\n");
            while(1) {

                if (retry_ctr++ > retry_max && mFirstConnectAttempt){
                    retry_ctr = 0;
                    printf("Switching to AP Mode (host connect timed out\n");
                    printf("heap size: %d\n", xPortGetFreeHeapSize());
                    return -2; // Tell main controller class to enter AP mode
                }

                mSocket = socket(res.ai_family, res.ai_socktype, 0);
                //TODO maybe put this back in: lwip_setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (void*)&optval, sizeof(optval));

                if(mSocket < 0) {
                    //printf("... Failed to allocate socket.\r\n");
                    //freeaddrinfo(res);
                    vTaskDelay(400 / portTICK_PERIOD_MS);
                    continue;
                }

                nbset = 0;
                ctlr = lwip_ioctl(mSocket, FIONBIO, &nbset);

                if(connect(mSocket, res.ai_addr, res.ai_addrlen) != 0) {
                    //printf("Closing socket: %d\n",mSocket);
                    close(mSocket);
                    //freeaddrinfo(res);
                    //printf("... socket connect failed.\r\n");
                    vTaskDelay(400 / portTICK_PERIOD_MS);
                    continue;
                }

                nbset = 1;
                ctlr = lwip_ioctl(mSocket, FIONBIO, &nbset);

                printf("... connected\r\n");
                mFirstConnectAttempt = false;
                break;
            }
            return 0;
        }

        // Process tcp command
        int _process_tcp_command(){

            int rready = _read_ready(true);
            if (rready < 0){
                printf("closing socket: %d",mSocket);
                close(mSocket);
                return rready;
            }
            else if ( rready == 0){
                return rready;
            }
            else {
                //proceed with read
            }

            int r = read(mSocket, mInbuf, PACKET_SIZE);
            if (r > 0){
                printf("received: %s", mInbuf);

                //////////////////////////////////////////////////////////
                // OTA
                //////////////////////////////////////////////////////////
                if (mInbuf[0] == 0x1){
                    printf("Received OTA Command");
                    if (write(mSocket, "OTA", 3) < 0){
                        printf("closing socket: %d\n",mSocket);
                        close(mSocket);
                        printf("failed to send ACK");
                    }
                    else {
                        printf("ACK sent");
                        return mInbuf[0];
                    }
                }
                //////////////////////////////////////////////////////////
                // Dump ADS Register to Serial
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0x2){
                    // print reg map
                    printf("Received reg map dump command");
                    return mInbuf[0];
                }
                //////////////////////////////////////////////////////////
                // Begin Test Signal Streaming
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0x3){
                    printf("Received ADS Test Signal Stream command \n");
                    return mInbuf[0];

                }
                //////////////////////////////////////////////////////////
                // Restart Device
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0x4){
                    printf("Received SoftAP Mode command\n");
                    return mInbuf[0];
                }
                //////////////////////////////////////////////////////////
                // 
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0x5){
                    sdk_system_restart();
                }
                //////////////////////////////////////////////////////////
                // 
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0x6){
                    // TODO respond with basic ack
                    printf("Sending basic ack\n");
                    char msg[] = "I am here ___";
                    write(mSocket, msg, sizeof(msg));
                }
                //////////////////////////////////////////////////////////
                // 
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0x7){
                    return mInbuf[0];

                }

                //JCR

                //////////////////////////////////////////////////////////
                // ADC_send_registers
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0xd){
                    return mInbuf[0];
                }
                //////////////////////////////////////////////////////////
                // ADC_rcv_registers
                //////////////////////////////////////////////////////////
                else if (mInbuf[0] == 0xe){
                    return mInbuf[0];
                }
                return 0;
            }
            else if (r==0){
                return 0;
            }
            else {
                // TODO Handle Error
                printf("_tcp_handle_command r=%d\n",r);
                printf("NOT closing socket: %d\n",mSocket);
                close(mSocket);
                return -1;
            }
        }

        int _initialize(StorageManager* sm)
        {

            storageManager = sm;
            // Get network params from spiffs
            char ssid[30];
            int ssid_len = sm->retrieve_ssid(ssid);
            if (ssid_len > 3){

            }
            else {
                printf("No valid ssid found in memory\n");
                return -1;
            }

            char pass[30];
            int pass_len = sm->retrieve_pass(pass);
            if (pass_len > 3){

            }
            else {
                printf("No valid pass key found in memory\n");
                return -1;
            }

            printf("Initializing WiFi config");

            struct sdk_station_config config;

            //pass_len = strlen(WIFI_PASS)+1;
            //ssid_len = strlen(WIFI_SSID)+1;
            //memcpy(config.ssid, WIFI_SSID, ssid_len);
            //memcpy(config.password, WIFI_PASS, pass_len);
            memcpy(config.ssid, ssid, ssid_len+1);
            memcpy(config.password, pass, pass_len+1);

            /* required to call wifi_set_opmode before station_set_config */
            printf("Attempting to connect with ssid: %s, sz: %d, pass: %s, sz: %d\n", config.ssid, ssid_len, config.password, pass_len);
            sdk_wifi_set_opmode(STATION_MODE);
            sdk_wifi_station_set_config(&config);
            //sdk_wifi_station_connect();
            return 0;
        }

        int _read_ready(bool check_alive){

            // Periodically check connection
            if (check_alive && (mKeepAliveCounter++ > 5E4)){
                mKeepAliveCounter = 0;
                int retry_counter = 0;
                int result = -1;
                while (result = write(mSocket, "ACK", 3), result < 0){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    if (++retry_counter > 3)
                    {
                        return -1;
                    }
                }
            }

            if (mSocket < 0){
                printf("mSocket < 0 - ERR\n");
                return -1;
            }

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(mSocket, &fds);

            //set both timval struct to zero
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0;

            int rsel = select( mSocket + 1, &fds , NULL, NULL, &tv );
            if (rsel < 1) {
                return 0;
            }
            else {
                // this is the only situation (i.e. socket is valid and ready to read) that should allow calling read()
                return 1;
            }
        }


        void _stream_task(ADS* ads) {

            // Setup remote address
            //struct ip_addr my_host_ip;
            //IP4_ADDR(&my_host_ip, 192, 168, 1, 202);

            struct sockaddr_in my_sockaddr_in;
            //my_sockaddr_in.sin_addr.s_addr = my_host_ip.addr;
            my_sockaddr_in.sin_addr.s_addr = mHostIp.addr;
            my_sockaddr_in.sin_len = sizeof(struct sockaddr_in);
            my_sockaddr_in.sin_family = AF_INET;
            my_sockaddr_in.sin_port = htons(mHostPort);
            std::fill_n(my_sockaddr_in.sin_zero, 8, (char)0x0);

            struct addrinfo res;
            res.ai_addr = (struct sockaddr*) (void*)(&my_sockaddr_in);
            res.ai_addrlen = sizeof(struct sockaddr_in);
            res.ai_family = AF_INET;
            res.ai_socktype = SOCK_DGRAM;

            // Setup local address
            struct sockaddr_in my_sock_addr_x;
            my_sock_addr_x.sin_addr.s_addr = htonl(INADDR_ANY);
            my_sock_addr_x.sin_len = sizeof(struct sockaddr_in);
            my_sock_addr_x.sin_family = AF_INET;
            my_sock_addr_x.sin_port = htons(mHostPort);
            std::fill_n(my_sock_addr_x.sin_zero, 8, (char)0x0);

            struct addrinfo res_x;
            res_x.ai_addr = (struct sockaddr*) (void*)(&my_sock_addr_x);
            res_x.ai_addrlen = sizeof(struct sockaddr_in);
            res_x.ai_family = AF_INET;
            res_x.ai_socktype = SOCK_DGRAM;

            while(1) {

                // Allocated socket
                int s = socket(res_x.ai_family, res_x.ai_socktype, 0);
                if(s < 0) {
                    printf("... Failed to allocate UDP socket.\r\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    continue;
                }
                printf("... allocated UDP socket\r\n");

                // Bind local socket
                if(bind(s, res_x.ai_addr, res_x.ai_addrlen) < 0) {
                    close(s);
                    printf("bind failed.\r\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    continue;
                }
                printf("... UDP bind success\r\n");

                // Init stream variables
                byte outbuf[pkt_size] = {0};
                byte inbuf[inbuf_size] = {0};
                uint8_t pkt_cnt = 0;
                uint8_t drop_count = 0;
                bool local_buf_acked = true;
                int inWaiting = 0;
                int heapSize = 0;


                printf("Seting up ADS\n");
                //ads->configureTestSignal();
                printf("Begin streaming\n");
                ads->startStreaming();

                /******************************** 
                  Main stream loop
                 *********************************/

                while(1) {

                    // If new data in local buf, send it
                    if (!local_buf_acked) {
                        int err = sendto(s, outbuf, pkt_size, 0, res.ai_addr, res.ai_addrlen );
                        if (err < 0) {
                            inWaiting = ads->getQueueSize();
                            heapSize = xPortGetFreeHeapSize();
                            printf("... socket send failed with err: %d, queue size: %d, heap: %d\n",err, inWaiting, heapSize);
                            close(s);
                            return;
                        }
                    }

                    // If local buf is free, check queue for new packet's worth of data
                    if (local_buf_acked){
                        inWaiting = ads->getQueueSize();
                        if (inWaiting >= 57){
                            // Fill local buffer with new data
                            if (ads->getDataPacket(outbuf)){ 
                                local_buf_acked = false;
                            }
                            else{
                                printf("Failed to load outbuf, file: %s, line: %d\n",__FILE__,__LINE__);
                            }
                        }
                    }

                    // Blocking read for ACK on currently buffered packet
                    recvfrom(s, inbuf, inbuf_size, 0, res.ai_addr, &(res.ai_addrlen)); 
                    if (inbuf[0] == outbuf[0])
                    {
                        // It's a proper ACK, move to next packet
                        outbuf[0] = ++pkt_cnt;
                        local_buf_acked = true;

                        // Get status data
                        outbuf[1] = (inWaiting >> 8) & 0xff;
                        outbuf[2] = inWaiting & 0xff;
                        heapSize = xPortGetFreeHeapSize();
                        outbuf[3] = (heapSize >> 16) & 0xff;
                        outbuf[4] = (heapSize >> 8)  & 0xff;
                        outbuf[5] = (heapSize >> 0)  & 0xff;
                    }
                    else  {
                        if (inbuf[2] == 0xff){
                            ads->stopStreaming();
                            close(s);
                            printf("Received TERMINATE command\n");
                            return;
                        }
                        drop_count++;
                    }
                }
            }
        }
};
