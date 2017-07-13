#ifndef _TIME_SYNC_H	// add _8_
#define _TIME_SYNC_H

#include "FreeRTOS.h"
#include "task.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "esp/timer.h"
#include "esp/spi.h"
//#include "esp8266.h"
#include <string.h>
#include "FreeRTOS.h"
//TEST #include "task.hpp"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "ipv4/lwip/ip_addr.h"
#include "ssid_config.h"

#include <inttypes.h>

#include "math.h" //for pow()
#include <algorithm>
//#include "debug_dumps.h"

#define DBG_TS 0

#define WEB_PORT 2050 //TODO: What the fuck does this do?`

#define PACKET_SIZE (8*5)

const int  DEFAULT_TIMEOUT = 1000; //in ms
const int DEFAULT_NUM_EXCHANGES = 8; //standard for NTP

const timer_clkdiv_t FRC2_CLK_DIV = TIMER_CLKDIV_1; //values are *_1, *_16, *_256; divides main clock by this
//ALSO: only used in two places, so don't need uint64_t for calculations
const uint64_t FRC2_ROLLOVER_PERIOD_MS = 30000;
const uint64_t FRC2_CLK_FREQ = \
(FRC2_CLK_DIV == TIMER_CLKDIV_1 ? (80000000) : \
(FRC2_CLK_DIV == TIMER_CLKDIV_16 ? (5000000) : (312500) )
);

const uint64_t FRC2_LOAD_VALUE ((uint64_t)0xffffffff - ( (FRC2_CLK_FREQ/1000) * FRC2_ROLLOVER_PERIOD_MS) );
//math:
//normal load value: 			((unsigned)0xffffffff
//cycles per second: (w/ divider) 	(FRC2_CLK_FREQ)
//cycles per millisecond: 		/ 1000
//cycles per rollover period: 		*FRC2_ROLLOVER_PERIOD_MS)

//Rollover counter
static volatile uint64_t timerRolloverCount = 0;
IRAM void timerIntRoutine(void) { timerRolloverCount++; return; }

typedef unsigned char byte;

class TimeSync
{
	public:
		TimeSync(); //NOTE: This time sync will NOT interact with the ADS what-so-ever, this means that you must manually disable interrupts else this code may be highly buggy / inconsistent
//Functionality removed
//		TimeSync(ADS *_currentADS);
		~TimeSync();
		
		int createConnectionInterface();
		//Call this IMMEDIATELY after starting ADS stream
		uint64_t markADSStartTime();
		uint64_t getADSStartTime();
		int handleSync();
		void setTimeoutMS(int _timeoutms); //pass this 0 to never timeout
		void setNumExchanges(int _numExchanges); //pass this 0 to never stop exchanging packets; WARNING: will loop endlessly unless there is a timeout set
		uint64_t getCurrentMicrosCount();

		uint64_t markT1();
		uint64_t markT2();
		uint64_t getT1();
		uint64_t getT2();
		static int initTimer();
	protected:

	private:
		//int establishHostConnection();
		uint64_t getCyclesCount();


		uint64_t T1;
		uint64_t T2;
		//ADS *currentADS;
		uint64_t ADSStartTime;
		uint64_t microsCount;

		struct addrinfo addrinfo_r, addrinfo_l;
		struct sockaddr_in sockadd_r, sockadd_l;
		struct ip_addr ipadd_r;

		int socketID;
		int timeoutms;
		int numExchanges;
};

uint64_t TimeSync::getCyclesCount() { return timer_get_count(FRC2); }

TimeSync::TimeSync() { 
	T1=0;
	T2=0;
	ADSStartTime = 0;
	microsCount = 0;
	socketID = -1;	
	timeoutms = DEFAULT_TIMEOUT;
	numExchanges = DEFAULT_NUM_EXCHANGES;

	//initTimer();
}
TimeSync::~TimeSync() { }

/* Functionality removed for now
TimeSync(ADS *_currentADS) { currentADS = _currentADS; initTimer(); }
*/

void TimeSync::setTimeoutMS(int _timeoutms) { timeoutms = _timeoutms; return; } 
void TimeSync::setNumExchanges(int _numExchanges) { numExchanges = _numExchanges; return; }
uint64_t TimeSync::markADSStartTime() { ADSStartTime = getCurrentMicrosCount(); return ADSStartTime; }
uint64_t TimeSync::markT1() { T1 = getCurrentMicrosCount(); return T1; }
uint64_t TimeSync::markT2() { T2 = getCurrentMicrosCount(); return T2; }
uint64_t TimeSync::getADSStartTime() { return ADSStartTime; }
uint64_t TimeSync::getT1() { return T1; }
uint64_t TimeSync::getT2() { return T2; }



uint64_t TimeSync::getCurrentMicrosCount()
{
	if(DBG_TS) printf("%u %u %u %u %u\n", (uint32_t)(getCyclesCount() - FRC2_LOAD_VALUE), (uint32_t)(timerRolloverCount * FRC2_CLK_FREQ), (uint32_t)FRC2_LOAD_VALUE,(uint32_t)FRC2_CLK_FREQ,(uint32_t)getCyclesCount());
	return ((getCyclesCount() - FRC2_LOAD_VALUE) + (timerRolloverCount * ( (FRC2_CLK_FREQ/1000) * FRC2_ROLLOVER_PERIOD_MS))) / (FRC2_CLK_FREQ / 1000000);
	//current ticks plus all rollover ticks:	(getCyclesCount() - FRC2_LOAD_VALUE) + (timerRolloverCount * FRC2_CLK_FREQ) 
	//divided by ticks per uS:			/ (FRC2_CKL_FREQ / 1000000.0);
}

int TimeSync::initTimer(void) //TODO: try / catch errors AND add in dbg code
{
	timer_set_interrupts(FRC2,false);
	timer_set_run(FRC2,false);
	timer_set_reload(FRC2,true); //set autoreload - not sure if it does this automatically or not?
	timer_set_divider(FRC2, FRC2_CLK_DIV); //FRC2_CLK_DIV is a #define'd value
	timer_set_load(FRC2, FRC2_LOAD_VALUE); //currently set s.t. this rolls over every FRC2_ROLLOVER_PERIOD_MS
	_xt_isr_attach(INUM_TIMER_FRC2, timerIntRoutine);
	timer_set_interrupts(FRC2, true);
	timer_set_run(FRC2,true);
	return 0;
}



//TODO store redundant variables in class?
int TimeSync::createConnectionInterface()
{	
	// Fill in ipaddr_r structure appropriately
	IP4_ADDR(&ipadd_r, 0, 0, 0, 0); // will be overwritten by recvfrom
	
	// Setup remote address ??? - C++
	sockadd_r.sin_addr.s_addr = ipadd_r.addr;
	sockadd_r.sin_len = sizeof(struct sockaddr_in);
	sockadd_r.sin_family = AF_INET;
	sockadd_r.sin_port = htons(WEB_PORT);
	std::fill_n(sockadd_r.sin_zero, 8, (char)0x0); //Different than C - does this matter?

	addrinfo_r.ai_addr = (struct sockaddr*) (void*)(&sockadd_r);
	addrinfo_r.ai_addrlen = sizeof(struct sockaddr_in);
	addrinfo_r.ai_family = AF_INET;
	addrinfo_r.ai_socktype = SOCK_DGRAM;

	//JCR - Are these needed? Present in HostCommManager
	struct in_addr *addr = &((struct sockaddr_in *)addrinfo_r.ai_addr)->sin_addr;
	if(DBG_TS) printf("TS: DNS lookup succeeded. IP(R)=%s\r\n", inet_ntoa(*addr));

	//SETUP LOCAL ADDR
	sockadd_l.sin_addr.s_addr = htonl(INADDR_ANY); // don't need to alloc address?
	sockadd_l.sin_len = sizeof(struct sockaddr_in);
	sockadd_l.sin_family = AF_INET;
	sockadd_l.sin_port = htons(WEB_PORT);
	std::fill_n(sockadd_l.sin_zero, 8, (char)0x0); //Different than C - does this matter?

	addrinfo_l.ai_addr = (struct sockaddr*) (void*)(&sockadd_l);
	addrinfo_l.ai_addrlen = sizeof(struct sockaddr_in);
	addrinfo_l.ai_family = AF_INET;
	addrinfo_l.ai_socktype = SOCK_DGRAM;

	//JCR - Are these needed? Present in HostCommManager
	struct in_addr *addr_l = &((struct sockaddr_in *)addrinfo_l.ai_addr)->sin_addr;
	if(DBG_TS) printf("TS: DNS lookup succeeded. IP(L)=%s\r\n", inet_ntoa(*addr_l));

	// Allocate socket
	int s = socket(addrinfo_l.ai_family, addrinfo_l.ai_socktype, 0);
	if(s < 0)
	{
		if(DBG_TS) printf("TS: Failed to allocate socket, returned %d.\r\n", s);
		return -2;
	}
	else if(DBG_TS) printf("TS: Allocated socket: %d\r\n", s);

	// Bind local socket
	if(bind(s, addrinfo_l.ai_addr, addrinfo_l.ai_addrlen) < 0)
	{
		close(s);
		if(DBG_TS) printf("TS: Bind failed.\r\n");
		return -3;
	}
	else if(DBG_TS) printf("TS: Bind success\r\n");
	
	//Save socket to class variable
	socketID=s;

	return s;
}


int TimeSync::handleSync(void)
{
	char outBuffer[PACKET_SIZE] = {0};
	uint8_t packetSentCounter = 0;
	uint8_t dropCount = 0;
	int sockOperationStatus = 0;
	char inBuffer[PACKET_SIZE] = {0};
	uint8_t SYNC_TOKEN = 0x01;
	bool wasStreaming = false;
int errval = 0;
	// Check for valid number of exchanges
	if(numExchanges < 0)
	{
		if(DBG_TS) printf("Invalid number of exchanges: %d, setting to default of %d", numExchanges, DEFAULT_NUM_EXCHANGES);
		numExchanges=DEFAULT_NUM_EXCHANGES;
	}

	//Check for valid timeout
	if(timeoutms < 0)
	{
		if(DBG_TS) printf("Invalid timeout: %d, setting to default of %dms", numExchanges,DEFAULT_TIMEOUT);
		timeoutms=DEFAULT_TIMEOUT;
	}

	//Check to see whether or not we're controlling ADS
	//NOTE: Possibly depricated functionality
/* Removed for now
	if((currentADS != NULL) && ((*currentADS).isStreaming()))
	{
		(*currentADS).stopStreaming();
		wasStreaming = true;
	}
*/

	errval=createConnectionInterface();
	if(errval < 0)
	{
		if(DBG_TS) printf("TS: Establish host connection error: %d, exiting.\n", errval);
		//close(socketID); taken care of by establishHostConnection();
		return errval;
	}

	// Set blocking
	int nbset = 0;
	int ctlr = lwip_ioctl(socketID, FIONBIO, &nbset);
	uint64_t vals[5] = {0};

	//Set timeout in ms - 0ms means no timeout
	//NOTE: Needs customs sockets.c that implements changing receive timeout
	//NOTE: Check bottom of this file for reference on how to implement if you haven't
	lwip_setsockopt(socketID, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeoutms, sizeof(timeoutms));
	lwip_setsockopt(socketID, SOL_SOCKET, SO_SNDTIMEO, (void*)&timeoutms, sizeof(timeoutms));

	
	uint64_t t2 = 0;

	//Send over WiFi
	for(int i=0 ; (i < numExchanges) || (!numExchanges) ; ++i) //if numExchanges == 0 then no maximum amount of iterations, and rcvStatus is negative only in the case of an error
	{

		// Blocking recv from mntp server for specific timeout
		sockOperationStatus=recvfrom(socketID, inBuffer, PACKET_SIZE, 0, addrinfo_r.ai_addr, &(addrinfo_r.ai_addrlen));
		if(sockOperationStatus < 0)
		{
			if(DBG_TS) printf("TS: Error in receiving packet, errno: %d\n", sockOperationStatus);
			close(socketID);
			return sockOperationStatus;
		}
		
		// Generate t2
		t2=markT2();

		// Copy t2
		int idx = 16;
		memcpy((inBuffer+idx),&t2,sizeof(t2));
		memcpy((inBuffer+idx+8),&t2,sizeof(t2));

		// Send pkt 2
		sockOperationStatus=sendto(socketID, inBuffer, PACKET_SIZE, 0, addrinfo_r.ai_addr, addrinfo_r.ai_addrlen );
		if(sockOperationStatus < 0)
		{
			if(DBG_TS) printf("TS: Socket send failed, error: %d\r\n", sockOperationStatus);
			close(socketID);
			return sockOperationStatus;
		}
	}	
//TEST	if((currentADS != NULL) && wasStreaming)
//TEST		(*currentADS).startStreaming();
	close(socketID);
	return 0; //A-OK
}








#endif

// JCR 04-29-17
// RCV and SND timeout functionality MUST be added to sockets.c !
//Beginning at line 1890:
/*
#if LWIP_SO_SNDTIMEO
    case SO_SNDTIMEO:
      netconn_set_sendtimeout(sock->conn, *(int*)optval);
      break;
...
...    
    case SO_RCVTIMEO:
      netconn_set_recvtimeout(sock->conn, *(int*)optval);
      break;
...
...
*/
