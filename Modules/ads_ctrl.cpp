#ifndef _ADS_CTRL_h	// add _8_
#define _ADS_CTRL_h

#include "FreeRTOS.h"
#include "task.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "esp/timer.h"
#include "esp/spi.h"
//#include "esp8266.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.hpp"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "ipv4/lwip/ip_addr.h"
#include "ssid_config.h"
//#include "debug_dumps.h"

#include "TimeSync.cpp" //included so we can use timer rollover count

//NOTE: VARIABLES
//Queue size for interrupt->host communication, recommended at least 100; note, the actual size is 100*(sizeof(inADS))
#define SAMPLE_QUEUE_SIZE 250

#define DBG 1

#define HIGH 1
#define LOW 0

// ESP REGISTERS - TODO May not need to be defined if we include some additional header files
#define ESP8266_REG(addr) *((volatile uint32_t *)(0x60000000+(addr)))

//SPI
#define SPI1U ESP8266_REG(0x11C)
#define SPIUSME (1 << 7)

//GPIO16
#define GP16O  ESP8266_REG(0x768)
#define GP16E  ESP8266_REG(0x774)

#define GPFFS_GPIO(p) (((p)==0||(p)==2||(p)==4||(p)==5)?0:((p)==16)?1:3)
#define GP16FFS(f) (((f) & 0x03) | (((f) & 0x04) << 4))

#define GP16C  ESP8266_REG(0x790)
#define GPC16 GP16C

#define GP16F  ESP8266_REG(0x7A0)
#define GPF16 GP16F

//ASD1299 Register Addresses
#define ID      0x00
#define CONFIG1 0x01
#define CONFIG2 0x02
#define CONFIG3 0x03
#define LOFF 0x04
#define CH1SET 0x05
#define CH2SET 0x06
#define CH3SET 0x07
#define CH4SET 0x08
#define CH5SET 0x09
#define CH6SET 0x0A
#define CH7SET 0x0B
#define CH8SET 0x0C
#define BIAS_SENSP 0x0D
#define BIAS_SENSN 0x0E
#define LOFF_SENSP 0x0F
#define LOFF_SENSN 0x10
#define LOFF_FLIP 0x11
#define LOFF_STATP 0x12
#define LOFF_STATN 0x13
#define GPIO_REG 0x14
#define MISC1 0x15
#define MISC2 0x16
#define CONFIG4 0x17

#define OPENBCI_NCHAN (8)  // number of EEG channels
// CHANNEL SETTINGS 
#define POWER_DOWN      (0)
#define GAIN_SET        (1)
#define INPUT_TYPE_SET  (2)
#define BIAS_SET        (3)
#define SRB2_SET        (4)
#define SRB1_SET        (5)
#define YES      	(0x01)
#define NO      	(0x00)

//Channel powered on or off
#define ADSINPUT_ENABLED (0b00000000)
#define ADSINPUT_DISABLED (0b10000000) 

//gainCode choices
#define ADSINPUT_GAIN01 (0b00000000)	// 0x00
#define ADSINPUT_GAIN02 (0b00010000)	// 0x10
#define ADSINPUT_GAIN04 (0b00100000)	// 0x20
#define ADSINPUT_GAIN06 (0b00110000)	// 0x30
#define ADSINPUT_GAIN08 (0b01000000)	// 0x40
#define ADSINPUT_GAIN12 (0b01010000)	// 0x50
#define ADSINPUT_GAIN24 (0b01100000)	// 0x60

//inputType choices
#define ADSINPUT_NORMAL (0b00000000)
#define ADSINPUT_SHORTED (0b00000001)
#define ADSINPUT_BIAS_MEAS (0b00000010)
#define ADSINPUT_MVDD (0b00000011)
#define ADSINPUT_TEMP (0b00000100)
#define ADSINPUT_TESTSIG (0b00000101)
#define ADSINPUT_BIAS_DRP (0b00000110)
#define ADSINPUT_BIAS_DRN (0b00000111)

#define ADSINPUT_SRB2_OPEN (0b00000000)
#define ADSINPUT_SRB2_CLOSED (0b00001000) 

//test signal choices...ADS1299 datasheet page 41
#define ADSTESTSIG_AMP_1X (0b00000000)
#define ADSTESTSIG_AMP_2X (0b00000100)
#define ADSTESTSIG_PULSE_SLOW (0b00000000)
#define ADSTESTSIG_PULSE_FAST (0b00000001)
#define ADSTESTSIG_DCSIG (0b00000011)
#define ADSTESTSIG_NOCHANGE (0b11111111)

#define SRB1_OPEN (0b00000000)
#define SRB1_CLOSED (0b00100000)

//Lead-off signal choices
#define LOFF_MAG_6NA (0b00000000)
#define LOFF_MAG_24NA (0b00000100)
#define LOFF_MAG_6UA (0b00001000)
#define LOFF_MAG_24UA (0b00001100)
#define LOFF_FREQ_DC (0b00000000)
#define LOFF_FREQ_7p8HZ (0b00000001)
#define LOFF_FREQ_31p2HZ (0b00000010)
#define LOFF_FREQ_FS_4 (0b00000011)
#define PCHAN (0)
#define NCHAN (1)
#define OFF (0)
#define ON (1)

#define DR_250SPS (0b00000110)
#define DR_500SPS (0b00000101)
#define DR_1KSPS (0b00000100)
#define DR_2KSPS (0b00000011)
#define DR_4KSPS (0b00000010)
#define DR_8KSPS (0b00000001)
#define DR_16KSPS (0b00000000)




typedef unsigned char byte;

class ADS
{
	public:
		ADS();
		ADS(uint32_t FREQ_DIVIDER);
		virtual ~ADS();

		bool isStreaming();
		bool isStandby();

		// BASIC ADS COMMANDS - USER ACCESIBLE
		void WAKEUP();
		void STANDBY();
		void RESET();
		void START();
		void STOP();
		void RDATAC(); //Be VERY cautious with this command and the "streaming" variable
		void SDATAC();
		byte RDATA();

		byte RREG(byte REG_NUMBER);
		void RREG(byte REG_NUMBER, byte NUMBER_OF_REGS);
		void WREG(byte REG_NUMBER, byte VALUE);
		void WREG(byte REG_NUMBER, byte NUMBER_OF_REGS_MIN_ONE, byte VALUE);
		void clearSPI();
		byte transfer(byte VALUE);
		void getSamples(byte dataInArray[29]);
		bool getData(byte dataInArray[29]);
		bool getData(byte dataInArray[29], TickType_t xTicksToWait);
		bool getDataFake(byte dataInArray[27], bool toggle);
		int getDataWaiting(byte dataInArray[29], TickType_t xTicksToWait);
		int getDataWaiting(byte dataInArray[29], TickType_t xTicksToWait, TickType_t& tickTimestamp);
		bool getDataPacket(byte dataInArray[1400]);

		//DBG
		void printTaskHandle(TaskHandle_t currTask);



		// REGISTER RELATED COMMANDS
		void receiveRegisterMapFromADS(void);
		void receiveRegisterMapFromArray(byte inputArray[24]);
		void copyRegisterMapToArray(byte inputArray[24]);
		void flushRegisterMapToADS(void);
		void printSerialRegistersFromADS(void);
		void printSerialRegistersFromMemory(void);

		//Caching
		int cacheSamples(StorageManager *sm);//stub
		int loadCachedSamplesToPacket(StorageManager *sm, byte packetBuffer[1400]);

		// Setup
		void setupADS();
		void setupDefaultRegisters();
		void configureTestSignal();
		void startStreaming();
		void stopStreaming();
		int getQueueSize(void);
		void writeCS(bool VAL);
		bool readCS(void);
		bool isDataReady(void);
		bool isDataReady(TickType_t xTicksToWait);

	protected:

		byte regMap[24];

		//CONFIG
		void setupIO16();
		void setupSPI(uint32_t FREQ_DIVIDER);

		void setupDRDY(void);
		void killDRDY(void);

		// BASIC ADS COMMANDS - HIDDEN
		byte _WAKEUP();
		byte _STANDBY();
		byte _RESET();
		byte _START();
		byte _STOP();
		byte _RDATAC();
		byte _SDATAC();
		byte _RDATA();

	private:
		void DRDYInterruptHandle(uint8_t gpio_num);
		bool dataReadyIsRunning = false;

		const int DRDY_PIN = 4;

		TickType_t xMaxBlockTime;

		void killStandby(void);
		void killStreaming(void);
		bool streaming = false;
		bool standby = false;


		const char *ADS_REG_NAMES[24] = {
			"ID",
			"CONFIG1",
			"CONFIG2",
			"CONFIG3",
			"LOFF",
			"CH1SET",
			"CH2SET",
			"CH3SET",
			"CH4SET",
			"CH5SET",
			"CH6SET",
			"CH7SET",
			"CH8SET",
			"BIAS_SENSP",
			"BIAS_SENSN",
			"LOFF_SENSP",
			"LOFF_SENSN",
			"LOFF_FLIP",
			"LOFF_STATP",
			"LOFF_STATN",
			"GPIO",
			"MISC1",
			"MISC2",
			"CONFIG4"
		};

};





// TODO - NEW! Add to header

/**
 * Predefinded SPI frequency dividers
#define SPI_FREQ_DIV_125K SPI_GET_FREQ_DIV(64, 10) ///< 125kHz
#define SPI_FREQ_DIV_250K SPI_GET_FREQ_DIV(32, 10) ///< 250kHz
#define SPI_FREQ_DIV_500K SPI_GET_FREQ_DIV(16, 10) ///< 500kHz
#define SPI_FREQ_DIV_1M   SPI_GET_FREQ_DIV(8, 10)  ///< 1MHz
#define SPI_FREQ_DIV_2M   SPI_GET_FREQ_DIV(4, 10)  ///< 2MHz
#define SPI_FREQ_DIV_4M   SPI_GET_FREQ_DIV(2, 10)  ///< 4MHz
#define SPI_FREQ_DIV_8M   SPI_GET_FREQ_DIV(5,  2)  ///< 8MHz
#define SPI_FREQ_DIV_10M  SPI_GET_FREQ_DIV(4,  2)  ///< 10MHz
#define SPI_FREQ_DIV_20M  SPI_GET_FREQ_DIV(2,  2)  ///< 20MHz
#define SPI_FREQ_DIV_40M  SPI_GET_FREQ_DIV(1,  2)  ///< 40MHz
#define SPI_FREQ_DIV_80M  SPI_GET_FREQ_DIV(1,  1)  ///< 80MHz
 */


// CLASS


ADS::ADS()
{
	ADS(SPI_FREQ_DIV_1M);
	return;
}

ADS::ADS(uint32_t FREQ_DIVIDER)
{
	for(int i = 0; i < 24; ++i)
		regMap[i] = 0;
	//    streaming = false;
	//    standby = false;
	//    dataReadyIsRunning = false;
	//    dataIsReady = false;
	//    DRDYBackgroundTask = NULL;
	//    xMaxBlockTime = pdMS_TO_TICKS( 5 );
	setupSPI(FREQ_DIVIDER);
	setupADS();
	setupDefaultRegisters();
	STANDBY();
	return;
}

ADS::~ADS()
{
	//empty for now
	return;
}

bool ADS::isStreaming()
{
	return streaming;
}
bool ADS::isStandby()
{
	return standby;
}
void ADS::killStandby()
{
	if(standby)
	{
		_WAKEUP(); //TODO - is this necessary?
		vTaskDelay(1 / portTICK_PERIOD_MS); //Only need a delay of ~2uS
		standby = false;
	}
	return;
}

void ADS::killStreaming()
{
	if(streaming)
	{
		_SDATAC();
		_STOP();
		streaming = false;
	}
	return;
}

// BASIC ADS COMMANDS - HIDDEN
byte ADS::_WAKEUP() { return spi_transfer_8(1, 0x02); } // Wake-up from standby mode
byte ADS::_STANDBY() { return spi_transfer_8(1, 0x04); } // Enter Standby mode
byte ADS::_RESET() { return spi_transfer_8(1, 0x06); } // Reset the device registers to default
byte ADS::_START() { return spi_transfer_8(1, 0x08); } // Start and restart (synchronize) conversions
byte ADS::_STOP() { return spi_transfer_8(1, 0x0A); } // Stop conversion
byte ADS::_RDATAC() { return spi_transfer_8(1, 0x10); } // Enable Read Data Continuous mode (default mode at power-up)
byte ADS::_SDATAC() { return spi_transfer_8(1, 0x11); } // Stop Read Data Continuous mode
byte ADS::_RDATA() { return spi_transfer_8(1, 0x12); } // Read data by command; supports multiple read back


// BASIC ADS COMMANDS - USER ACCESIBLE
void ADS::WAKEUP()
{// TODO - double check logic
	if(!(streaming && !standby)) killStandby(); //Kill standby only if it is not streaming and on standby
	return;
}

void ADS::STANDBY()
{
	killStreaming();
	_STANDBY();
	standby = true;
	return;
}

void ADS::RESET()
{
	killStandby();
	killStreaming();
	_RESET();
	return;
}

void ADS::START()
{
	killStandby();
	_START();
	return;
}

void ADS::STOP()
{
	killStandby();
	killStreaming();
	_STOP();
	return;
}

void ADS::RDATAC() //Be VERY cautious with this command and the "streaming" variable
{
	killStandby();
	_RDATAC();
	return;
}

void ADS::SDATAC()
{
	_SDATAC();
	streaming = false;
	return;
}

byte ADS::RDATA()
{
	killStreaming();
	return _RDATA();
}

//TODO - With WREG update registers in memory
//RREG / WREG
byte ADS::RREG(byte REG_NUMBER)
{
	spi_transfer_8(1, 0b00100000 | REG_NUMBER);
	spi_transfer_8(1, 0);
	return spi_transfer_8(1, 0x00);
}

void ADS::RREG(byte REG_NUMBER, byte NUMBER_OF_REGS_MIN_ONE)
{
	spi_transfer_8(1, 0b00100000 | REG_NUMBER);
	spi_transfer_8(1, NUMBER_OF_REGS_MIN_ONE);
	return;
}

void ADS::WREG(byte REG_NUMBER, byte VALUE)
{
	spi_transfer_8(1, 0b01000000 | REG_NUMBER);
	spi_transfer_8(1, 0);
	spi_transfer_8(1, VALUE);
	return;
}

void ADS::WREG(byte REG_NUMBER, byte NUMBER_OF_REGS_MIN_ONE, byte VALUE)
{
	spi_transfer_8(1, 0b01000000 | REG_NUMBER);
	spi_transfer_8(1, NUMBER_OF_REGS_MIN_ONE);
	for(int i = 0; i <= NUMBER_OF_REGS_MIN_ONE; ++i) spi_transfer_8(1, VALUE);
	return;
}

// MISC
void ADS::clearSPI()
{
	for(int i = 0; i < 27; ++i)
		spi_transfer_8(1, 0x00);
	return;
}

// This is specifically a user function to access spi_transfer_8
// Do not use inside class
byte ADS::transfer(byte VALUE) { return spi_transfer_8(1,VALUE); }

void ADS::getSamples(byte dataInArray[27])
{
	for(int i = 2; i < 29; ++i)
		dataInArray[i] = spi_transfer_8(1,0x00);
	return; 
}

//Use this to toggle/read CS - user accesible
void ADS::writeCS(bool VAL) { VAL ? (GP16O |= 1) : (GP16O &= ~1); }
bool ADS::readCS(void) { return (GP16O & 1); }


// GLOBAL Semaphore and Task Handle
QueueHandle_t xDataReadyQueue; // Do not need to set NULL

// Passed by queue
struct inADSData
{
	uint32_t timercount = 0;
	uint16_t timerrollovercount = 0; 
	byte inDataArray[28] = {0}; //TODO: make sure all calls / uses know that size has changed from 30 -> 28, we don't use first two bytes anymore.
	//TODO: We could save an extra byte here; inDataArray only needs to be 27 (actually only 24 if discounting stat bytes)
} s_tmpDataBuffer;
typedef struct inADSData inADSData;

int ADS::getQueueSize(void){
	return uxQueueMessagesWaiting(xDataReadyQueue);
}


bool ADS::getDataFake(byte dataInArray[29], bool toggle)
{
	for (int i = 5; i < 29; i++){
		if ((i-5)%3 == 0 && toggle){
			dataInArray[i] = 0xff;
			dataInArray[i+1] = 0xff;
			dataInArray[i+2] = 0xff;
		}
		else if ((i-5)%3 == 0 && !toggle){
			dataInArray[i] = 0;
			dataInArray[i+1] = 0;
			dataInArray[i+2] = 1;
		}
	}
	return true;
}

bool ADS::getData(byte dataInArray[29])
{
	return ADS::getData(dataInArray, pdMS_TO_TICKS( 10 ));
}

bool ADS::getData(byte dataInArray[29], TickType_t xTicksToWait)
{
	if(!dataReadyIsRunning) setupDRDY();
	if(xDataReadyQueue == NULL) printf("Queue failed to create!\n");

	if (xQueueReceive(xDataReadyQueue, &s_tmpDataBuffer, xTicksToWait) == pdTRUE) { 
		memcpy(dataInArray + 3, s_tmpDataBuffer.inDataArray + 3, 24);
		return true;
	}
	else{
		return false; //ticks are in 10ms
	}
}

int ADS::getDataWaiting(byte dataInArray[29], TickType_t xTicksToWait)
{
	if(!dataReadyIsRunning) setupDRDY();
	if(xDataReadyQueue == NULL) printf("Queue failed to create!\n");

	int inWaiting = uxQueueMessagesWaiting(xDataReadyQueue);

	if (xQueueReceive(xDataReadyQueue, &s_tmpDataBuffer, xTicksToWait) == pdTRUE) { 
		memcpy(dataInArray + 3, s_tmpDataBuffer.inDataArray + 3, 24);
		return inWaiting;
	}
	else{
		return -1; //ticks are in 10ms
	}
}

bool ADS::getDataPacket(byte dataInArray[1400])
{
	if(!dataReadyIsRunning) setupDRDY();
	if(xDataReadyQueue == NULL) {
		printf("Queue failed to create!\n");
		return false;
	}

	for (int i = 0; i < 57; i++) {
		if (xQueueReceive(xDataReadyQueue, &s_tmpDataBuffer, 0) == pdTRUE) {
			if(i==0) {
				uint64_t tmpTimeVal = ((uint64_t)s_tmpDataBuffer.timercount + ((uint64_t)s_tmpDataBuffer.timerrollovercount * ( (FRC2_CLK_FREQ/1000) * FRC2_ROLLOVER_PERIOD_MS))) / (uint64_t)(FRC2_CLK_FREQ / 1000000);
				memcpy(dataInArray + 6, &tmpTimeVal,8); //This copies over uS timestamp
			}
			// leave 24 spaces up from for status bytes, only copy data bytes
			// Copy elements 3-26 (i.e. 24 elements) from inDataArray
			memcpy(dataInArray + 24 + (i*24), s_tmpDataBuffer.inDataArray + 3, 24);
		}
		else {
			printf("xQueueReceive failed, file: %s, line: %d\n", __FILE__, __LINE__);
			return false;
		}
	}
	return true;
}

int ADS::getDataWaiting(byte dataInArray[29], TickType_t xTicksToWait, TickType_t& tickTimestamp)
{
	if(!dataReadyIsRunning) setupDRDY();
	if(xDataReadyQueue == NULL) printf("Queue failed to create!\n");

	int inWaiting = uxQueueMessagesWaiting(xDataReadyQueue);

	if (xQueueReceive(xDataReadyQueue, &s_tmpDataBuffer, xTicksToWait) == pdTRUE) { 
		memcpy(dataInArray + 3, s_tmpDataBuffer.inDataArray + 3, 24);        
		//tickTimestamp = s_tmpDataBuffer.tickCount; //TODO FIX OR DELETE
		return inWaiting;
	}
	else{
		return -1; //ticks are in 10ms
	}
}

void ADS::setupDRDY(void)
{
	gpio_enable(DRDY_PIN, GPIO_INPUT);
	dataReadyIsRunning = true;
	xDataReadyQueue = xQueueCreate(SAMPLE_QUEUE_SIZE, sizeof(inADSData));
	if(xDataReadyQueue == NULL) printf("Queue failed to create!\n");
	gpio_set_interrupt(DRDY_PIN, GPIO_INTTYPE_EDGE_NEG, (void(*)(uint8_t))&ADS::DRDYInterruptHandle);
	return;
}

//TODO - Implement kill DRDY
void ADS::killDRDY(void)
{
	if(xDataReadyQueue != NULL) vQueueDelete(xDataReadyQueue);
	xDataReadyQueue = NULL;
	dataReadyIsRunning = false;
	//TODO - disable DRDY interrupt here
	return;
}

// Interrupt only struct
inADSData interruptStruct;

void ADS::DRDYInterruptHandle(uint8_t gpio_num)
{
	if(xDataReadyQueue != NULL)
	{
		for(int i = 0; i < 27; ++i) // ...why the hell don't we use the first two bytes??? TODO: fixed this, make sure it didn't break anything else
		{
			interruptStruct.inDataArray[i] = spi_transfer_8(1,0x00);
		}
		interruptStruct.timercount = uint32_t(timer_get_count(FRC2) - FRC2_LOAD_VALUE); //gives current nuymber of counts (not the actual timer valuer because FRC2 counts up)
		//TODO Use TimeSync.cpp
		interruptStruct.timerrollovercount = timerRolloverCount; //this is from TimeSync.cpp
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xQueueSendFromISR(xDataReadyQueue, &interruptStruct, &xHigherPriorityTaskWoken);
	}
}







//Caching
int ADS::cacheSamples(StorageManager *sm)
{
	int samplesCached = 0;
	if(xDataReadyQueue == NULL) {
		printf("Queue failed to create! Line: %d\n", __LINE__);
		return -1;
	}

	//Buffer passed to write_cache
	char cacheBlock[240] = {0};

	//Samples currently in queue
	int inWaiting = uxQueueMessagesWaiting(xDataReadyQueue);
	for(int i = (inWaiting / 8); i > 0; --i) //Check to make sure we have enough samples to cache (min is 8) and update after each pass
	{
		for(int j = 0; j < 8; ++j)
		{
			if(xQueueReceive(xDataReadyQueue, &s_tmpDataBuffer, 0) == pdTRUE)
			{
				memcpy((cacheBlock + j*30), &(s_tmpDataBuffer.timerrollovercount), 2);
				memcpy((cacheBlock + 2 + j*30), &(s_tmpDataBuffer.timercount), 4);

				// leave 24 spaces up from for status bytes, only copy data bytes
				// Copy elements 3-26 (i.e. 24 elements) from inDataArray
				memcpy((cacheBlock + 6 + j*30), s_tmpDataBuffer.inDataArray + 3, 24);
			}
			else
			{
				printf("xQueueReceive failed, file: %s, line: %d\n", __FILE__, __LINE__);
				return -1;
			}
		}
		sm->write_cache(cacheBlock);
	}
	return (inWaiting / 8); //TODO double check this
}

int ADS::loadCachedSamplesToPacket(StorageManager *sm, byte packetBuffer[1400])
{
	char cacheInBuf[240] = {0};
	uint32_t timercount_l = 0;
	uint16_t timerrollovercount_l = 0;
	for(int i = 0; i < 8; ++i)
	{
		if(sm->read_cache(cacheInBuf))
		{
			if(i==0) //indicates that this will be the sample used to timestamp the packet
			{
				memcpy(&timerrollovercount_l, (cacheInBuf + 0), 2);
				memcpy(&timercount_l, (cacheInBuf + 2), 4);
				uint64_t tmpTimeVal = 
				((uint64_t)timercount_l + ((uint64_t)timerrollovercount_l * ( (FRC2_CLK_FREQ/1000) * FRC2_ROLLOVER_PERIOD_MS)))
				/ (uint64_t)(FRC2_CLK_FREQ / 1000000);
				
				memcpy(packetBuffer + 6, &tmpTimeVal,8); //This copies over uS timestamp
			}
			for(int j = 0; j < 8; ++j)
			{
				// leave 24 spaces up from for status bytes, only copy data bytes
				memcpy((packetBuffer + 24 + (i*24*8) + (j*8)), (cacheInBuf + 6 + (j*8)), 24);
			}

		}
		else
		{
			packetBuffer[15] = i & 0xff;
			return i*8; //if less than 56 samples are cached we return that value
		}
	}
	packetBuffer[15] = 56; //I believe this is the byte we will use
	return 56; //this assumes we have made it through 57 writes; return magic number 75
}










// REGISTER RELATED COMMANDS
void ADS::receiveRegisterMapFromADS(void)
{
	killStandby();
	killStreaming();
	RREG(ID, 23);
	for(int i = 0; i < 24; i++)
		regMap[i] = spi_transfer_8(1, 0x00);
	STANDBY();
	return;
}

void ADS::receiveRegisterMapFromArray(byte inputArray[24]) //TODO HEADER
{
	killStandby();
	killStreaming();
	for(int i = 0; i < 24; i++)
		regMap[i] = inputArray[i];
	STANDBY();
	return;
}

void ADS::copyRegisterMapToArray(byte inputArray[24])
{
	for(int i = 0; i < 24; ++i)
		inputArray[i] = regMap[i];
	return;
}

void ADS::flushRegisterMapToADS(void)
{
	killStandby();
	killStreaming();
	for(int i = 0; i < 24; ++i)
		WREG(i, 0, regMap[i]);
	STANDBY();
	return;
}

void ADS::printSerialRegistersFromADS(void)
{

	byte tmpArr[24] = {0};

	if(DBG)
	{
		killStandby();
		killStreaming();
		RREG(ID, 23);
		for(int i = 0; i < 24; ++i){
			tmpArr[i] = spi_transfer_8(1, 0x00);
		}
		for(int i = 0; i < 24; ++i){
			printf("\n%s : %X", ADS_REG_NAMES[i], tmpArr[i]);
		}
		STANDBY();
		return;
	}
}

void ADS::printSerialRegistersFromMemory(void)
{
	if(DBG)
	{
		killStandby();
		killStreaming();
		for(int i = 0; i < 24; ++i)
			printf("\n%s : %X", ADS_REG_NAMES[i], regMap[i]);
		STANDBY();
		return;
	}
}

// SETUP

//Internal function, called with setupSPI
void ADS::setupIO16()
{
	GPF16 = GP16FFS(GPFFS_GPIO(16));
	GPC16 = 0;
	GP16E |= 1; //Set to output
	return;
}

//Configure SPI basics
void ADS::setupSPI(uint32_t FREQ_DIVIDER)
{
	if(DBG) printf("\nSetting up SPI");
	//Bus 1, Mode 1 (CPOL=0 CPHA=1), 1MHz, MSB, little endian, manually toggle CS
	spi_init(1, SPI_MODE1, SPI_FREQ_DIV_1M, true, SPI_BIG_ENDIAN, true);

	//Setup CPHA - TODO may not be necessary with new RTOS version?
	SPI1U |= (SPIUSME);

	//Configure as output, etc...
	setupIO16();
	writeCS(LOW);

	return;
}

void ADS::setupADS()
{
	if(DBG) printf("\nBeginning ADS setup.");

	/*
	   I don't know why this is necessary - I believe it
	   synchronizes communication with the ADS. Appears to
	   not work without toggling CS high and low.
	 */
	writeCS(HIGH);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	writeCS(LOW);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	writeCS(HIGH);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	writeCS(LOW);    

	if(DBG) printf("\nToggled CS");

	RESET();
	vTaskDelay(1 / portTICK_PERIOD_MS); //Only need a delay of 8uS
	SDATAC();
	clearSPI();
	if(DBG) printf("\nRREG response, \"ID\" is %X", RREG(ID));

	STANDBY();
	return;
}

void ADS::setupDefaultRegisters()
{
	killStandby();
	killStreaming();
	WREG(CONFIG1, 0, 0b10010000 | DR_250SPS);
	//CONFIG2 is exclusively test signals; don't need unless using them
	//WREG(CONFIG2, 0, 0b11010001); //Test sig driven internally(4), decrease pulse frequency(1,0)
	WREG(CONFIG3, 0, 0b11101101); //Internal reference buffer enabled(7), bias ref signal generated internally(3), bias buffer enabled(2), bias sense diasbled(1), bias LOFF disconnected(0)
	WREG(BIAS_SENSN, 0, 0b00000000); //Turn off all connections to bias on N side (not used)
	WREG(BIAS_SENSP, 0, 0b00000000); //Turn off all connections to bias on N side (not used)
	WREG(CH1SET, 7, ADSINPUT_ENABLED | ADSINPUT_GAIN24 | ADSINPUT_NORMAL | ADSINPUT_SRB2_OPEN); //Write all eight channels to reasonable defaults
	WREG(MISC1, SRB1_CLOSED); //Our system uses SRB1; SRB2 not used
	STANDBY();

}


void ADS::configureTestSignal()
{
	killStandby();
	killStreaming();
	WREG(CONFIG2, 0, 0b11010001); //Test sig driven internally(4), decrease pulse frequency(1,0)
	WREG(CONFIG3, 0, 0b11101101); //Internal reference buffer enabled(7), bias ref signal generated internally(3), bias buffer enabled(2), bias sense diasbled(1), bias LOFF disconnected(0)
	WREG(BIAS_SENSN, 0, 0b00000000); //Turn off all connections to bias on N side (not used)
	//WREG(CH1SET, 7, ADSINPUT_ENABLED | ADSINPUT_GAIN24 | ADSINPUT_SHORTED | ADSINPUT_SRB2_OPEN); //Write all eight channels - test signal
	WREG(CH1SET, 7, ADSINPUT_ENABLED | ADSINPUT_GAIN24 | ADSINPUT_TESTSIG | ADSINPUT_SRB2_OPEN); //Write all eight channels - test signal
	WREG(CONFIG1, 0, 0b10010000 | DR_250SPS);
	STANDBY();
}

void ADS::startStreaming() {
	if(streaming) return;
	if(!dataReadyIsRunning) setupDRDY();
	killStandby();

	_SDATAC();
	clearSPI();
	_START();
	_RDATAC();
	streaming = true;
}

void ADS::stopStreaming() {
	killStandby();
	killDRDY();

	_SDATAC();
	clearSPI();
	_STOP();
	streaming = false;
	STANDBY();
}





#endif
