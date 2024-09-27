/*
 * Copyright 2016 karawin (http://www.karawin.fr)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <espressif/esp_common.h>
#include <espressif/user_interface.h>
#include <espressif/esp_softap.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_system.h>
#include <etstimer.h>
#include <espressif/esp_timer.h>
#include <espressif/osapi.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <lwip/api.h>
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/opt.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"

#include <lwip/apps/mdns.h>
#include <mdnsresponder.h>

#include <esp/uart.h>

#include "webserver.h"
#include "webclient.h"
#include "buffer.h"
#include "extram.h"
#include "vs1053.h"
#include "ntp.h"
#include "telnet.h"
#include "servers.h"
#include "interface.h"

//#define DNSSD_PROTO_TCP 0  // Define TCP protocol as 0 if it's not defined


const char striDEF0[] ICACHE_RODATA_ATTR STORE_ATTR  = {"The default AP is  WifiKaRadio. Connect your wifi to it.\nThen connect a webbrowser to 192.168.4.1 and go to Setting\nMay be long to load the first time.Be patient.%c"};
const char striDEF1[] ICACHE_RODATA_ATTR STORE_ATTR  = {"Erase the database and set ssid, password and ip's field%c"};
const char striAP[] ICACHE_RODATA_ATTR STORE_ATTR  = {"AP1: %s, AP2: %s\n"};
const char striSTA1[] ICACHE_RODATA_ATTR STORE_ATTR  = {" AP1:Station Ip: %d.%d.%d.%d\n"};
const char striSTA2[] ICACHE_RODATA_ATTR STORE_ATTR  = {" AP2:Station Ip: %d.%d.%d.%d\n"};
const char striTRY[] ICACHE_RODATA_ATTR STORE_ATTR  = {"Trying AP%d %s ,  I: %d status: %d\n"};
const char striTASK[] ICACHE_RODATA_ATTR STORE_ATTR  = {"%s task: %x\n"};
const char striHEAP[] ICACHE_RODATA_ATTR STORE_ATTR  = {"Heap size: %d\n"};
const char striUART[] ICACHE_RODATA_ATTR STORE_ATTR  = {"UART READY%c"};
const char striWATERMARK[] ICACHE_RODATA_ATTR STORE_ATTR  = {"watermark %s: %d  heap:%d\n"};

//ip
static char localIp[20] = {"0.0.0.0"};

//      void uart_div_modify(int no, unsigned int freq);
//	struct sdk_station_config config;

uint8_t FlashOn = 5,FlashOff = 5;
uint8_t FlashCount = 0xFF;
uint8_t FlashVolume = 0;

char* getIp() { return (localIp); }

void testtask(void* p) {
	struct device_settings *device;	
/*
	int uxHighWaterMark;
	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	printf(striWATERMARK,"testtask",uxHighWaterMark,xPortGetFreeHeapSize( ));
*/

	vTaskDelay(10);

	while(FlashCount==0xFF) {
		vTaskDelay(FlashOff);
		
		// save volume if changed		
		device = getDeviceSettings();
		if (device != NULL)
		{	
			if (device->vol != clientIvol)
			{ 
				device->vol = clientIvol;
				saveDeviceSettings(device);
//	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
//	printf(striWATERMARK,"testtask",uxHighWaterMark,xPortGetFreeHeapSize( ));
			}
			free(device);
		}
	}
//	printf("t0 end\n");
	vTaskDelete( NULL ); // stop the task
}

void set_dhcp_hostname(char* hostname) {
    struct dhcp *dhcp_client;

    // Set the hostname for the network interface (global default)
    netif_set_hostname(netif_default, hostname);

    // Access the DHCP client structure
    dhcp_client = netif_dhcp_data(netif_default);

    // Check if the DHCP client is available
    if (dhcp_client != NULL) {
        printf("DHCP client is running. Hostname set to: %s\n", hostname);
    } else {
        printf("DHCP client not available.\n");
    }
}

//-------------------------
// mDNS management
//-------------------------
void initMDNS(const char* host, uint32_t ip)
{
    // Initialize mDNS
    mdns_init();

    // Convert the uint32_t IP address to ip_addr_t
    ip_addr_t ip_addr;
    IP4_ADDR(&ip_addr, (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

    // Register the mDNS service directly using the facility function
    mdns_add_facility(host, "_http", NULL, mdns_TCP + mdns_Browsable, 80, 600);
}

//-------------------------
// Wifi  management
//-------------------------
void initWifi()
{
//-------------------------
// AP Connection management
//-------------------------
	uint16_t ap = 0;
	int i = 0;	
	char hostn[HOSTLEN];
	struct ip_info *info;
	struct device_settings *device;
	struct device_settings1* device1;
	struct sdk_station_config* config;

	set_dhcp_hostname("WifiKaRadio");

	device = getDeviceSettings();
	device1 = getDeviceSettings1();  // extention of saved data

	config = malloc(sizeof(struct sdk_station_config));
	info = malloc(sizeof(struct ip_info));
	
// if device1 not initialized, erase it and copy pass2 to the new place
	if (device1->cleared != 0xAABB)
	{		
		eeErasesettings1();
		device1->cleared = 0xAABB; //marker init done
		memcpy(device1->pass2,device->pass2, 64);
		saveDeviceSettings1(device1);	
	}		
	sdk_wifi_set_opmode_current(STATION_MODE);
	sdk_wifi_station_set_auto_connect(false);
	sdk_wifi_get_ip_info(STATION_IF, info); // ip netmask gw
	sdk_wifi_station_get_config_default(config); //ssid passwd
	if ((device->ssid[0] == 0xFF)&& (device->ssid2[0] == 0xFF) )  {eeEraseAll(); device = getDeviceSettings();} // force init of eeprom
	if (device->ssid2[0] == 0xFF) {device->ssid2[0] = 0; device1->pass2[0] = 0; }
	printf(striAP,device->ssid,device->ssid2);
		
	// Check if it's the first use
	if ((strlen(device->ssid) == 0) || (device->ssid[0] == 0xFF)) {
	        printf("First use\n");
	
	        // Set IP addresses (replace deprecated IP4_ADDR with ip4addr_aton or directly assign)
		IP4_ADDR(&(info->ip), 192, 168, 1, 254);
		IP4_ADDR(&(info->netmask), 0xFF, 0xFF,0xFF, 0);
		IP4_ADDR(&(info->gw), 192, 168, 1, 254);


	        // Copy IP information to device settings
	        memcpy(&device->ipAddr, &info->ip, sizeof(ip4_addr_t));
	        memcpy(&device->mask, &info->netmask, sizeof(ip4_addr_t));
	        memcpy(&device->gate, &info->gw, sizeof(ip4_addr_t));
	
	        // Copy SSID and password from config to device
	        strcpy(device->ssid, config->ssid);
	        strcpy(device->pass, config->password);
	
	        // Enable DHCP
	        device->dhcpEn = true;
	
	        // Set static IP configuration in the TCP/IP adapter
		sdk_wifi_set_ip_info(STATION_IF, &info);
	
	        // Save device settings (assumes saveDeviceSettings is implemented elsewhere)
	        saveDeviceSettings(device);
	}
	
// set for AP1 //
//-------------//
	IP4_ADDR(&(info->ip), device->ipAddr[0], device->ipAddr[1],device->ipAddr[2], device->ipAddr[3]);
	IP4_ADDR(&(info->netmask), device->mask[0], device->mask[1],device->mask[2], device->mask[3]);
	IP4_ADDR(&(info->gw), device->gate[0], device->gate[1],device->gate[2], device->gate[3]);

	strcpy(config->ssid, device->ssid);
	strcpy(config->password, device->pass);

	sdk_wifi_station_set_config(config);
	if (!device->dhcpEn) {
        //  if ((strlen(device->ssid)!=0)&&(device->ssid[0]!=0xff)&&(!device->dhcpEn))
        //  conn = true; //static ip
            sdk_wifi_station_dhcpc_stop();
            sdk_wifi_set_ip_info(STATION_IF, info);
        }
	printf(striSTA1,(info->ip.addr&0xff), ((info->ip.addr>>8)&0xff), ((info->ip.addr>>16)&0xff), ((info->ip.addr>>24)&0xff));
        sdk_wifi_station_connect();

  //	printf("DHCP: 0x%x\n Device: Ip: %d.%d.%d.%d\n",device->dhcpEn,device->ipAddr[0], device->ipAddr[1], device->ipAddr[2], device->ipAddr[3]);
  //	printf("\nI: %d status: %d\n",i,wifi_station_get_connect_status());

  i = 0;
  while ((sdk_wifi_station_get_connect_status() != STATION_GOT_IP)) {
    printf(striTRY, ap + 1, config -> ssid, i, sdk_wifi_station_get_connect_status());
    FlashOn = FlashOff = 40;
    vTaskDelay(400); //  ms
    if ((strlen(config -> ssid) == 0) || (sdk_wifi_station_get_connect_status() == STATION_WRONG_PASSWORD) 
                                      || (sdk_wifi_station_get_connect_status() == STATION_CONNECT_FAIL)
                                      || (sdk_wifi_station_get_connect_status() == STATION_NO_AP_FOUND)) {
      // try AP2 //
      if ((strlen(device -> ssid2) > 0) && (ap < 1)) {
        i = -1;
        sdk_wifi_station_disconnect();
        // set for AP2 //
        //-------------//
				IP4_ADDR(&(info->ip), device->ipAddr[0], device->ipAddr[1],device->ipAddr[2], device->ipAddr[3]);
				IP4_ADDR(&(info->netmask), device->mask[0], device->mask[1],device->mask[2], device->mask[3]);
				IP4_ADDR(&(info->gw), device->gate[0], device->gate[1],device->gate[2], device->gate[3]);
				strcpy(config->ssid,device->ssid2);
				strcpy(config->password,device1->pass2);
				sdk_wifi_station_set_config(config);
				if (!device->dhcpEn)
				{
					sdk_wifi_station_dhcpc_stop();
					sdk_wifi_set_ip_info(STATION_IF, info);
				}
				sdk_wifi_station_connect();
				printf(striSTA2,(info->ip.addr&0xff), ((info->ip.addr>>8)&0xff), ((info->ip.addr>>16)&0xff), ((info->ip.addr>>24)&0xff));
        //----------------
        ap++;
      }
      else i = 10; // go to SOFTAP_MODE
    }
    i++;
    if (i >= 10) { // AP mode
      printf(PSTR("%c"), 0x0d);
      sdk_wifi_station_disconnect();
      FlashOn = 10;
      FlashOff = 200;
      vTaskDelay(100);
      //printf(PSTR("Config not found%c%c"),0x0d,0x0d);
      saveDeviceSettings(device);
			printf(striDEF0,0x0d);
			printf(striDEF1,0x0d);
			struct sdk_softap_config *apconfig;
      apconfig = malloc(sizeof(struct sdk_softap_config));
      sdk_wifi_set_opmode_current(SOFTAP_MODE);
      vTaskDelay(10);
      sdk_wifi_softap_get_config(apconfig);
      vTaskDelay(10);
      strcpy(apconfig->ssid, "KaRadio");
      apconfig->ssid_len = 0;
      //printf("passwd: %s\nhidden: %d\nmaxc: %d\nauth: %d\n",apconfig->password,apconfig->ssid_hidden,apconfig->max_connection,apconfig->authmode);
			if (sdk_wifi_softap_set_config(apconfig) != true)printf(PSTR("softap failed%c%c"),0x0d,0x0d);
      vTaskDelay(1);
      sdk_wifi_get_ip_info(1, info);
      //printf(striSTA1,(info->ip.addr&0xff), ((info->ip.addr>>8)&0xff), ((info->ip.addr>>16)&0xff), ((info->ip.addr>>24)&0xff));
      vTaskDelay(10);
      //			conn = true;
      free(apconfig);
      break;
    }
  }
  //wifi_station_set_reconnect_policy(true);
  // update device info
  if (sdk_wifi_get_opmode() == SOFTAP_MODE) sdk_wifi_get_ip_info(SOFTAP_IF, info);
  else sdk_wifi_get_ip_info(STATION_IF, info); // ip netmask gw
  sdk_wifi_station_get_config(config);
  /*
  	IPADDR2_COPY(&device->ipAddr, &info->ip);
  	IPADDR2_COPY(&device->mask, &info->netmask);
  	IPADDR2_COPY(&device->gate, &info->gw);
  */
	memcpy(&device->ipAddr, &info->ip, sizeof(&device->ipAddr));
	memcpy(&device->mask, &info->netmask, sizeof(&device->mask));
	memcpy(&device->gate, &info->gw, sizeof(&device->gate));

  saveDeviceSettings(device);
	printf(striSTA1,(info->ip.addr&0xff), ((info->ip.addr>>8)&0xff), ((info->ip.addr>>16)&0xff), ((info->ip.addr>>24)&0xff));
	kasprintf(localIp,PSTR("%d.%d.%d.%d"),(info->ip.addr&0xff), ((info->ip.addr>>8)&0xff), ((info->ip.addr>>16)&0xff), ((info->ip.addr>>24)&0xff));
	// set modem sleep per default
	//sdk_wifi_set_sleep_type(MODEM_SLEEP_T);
	if ((strlen(device1->hostname) >= HOSTLEN) ||
		(strlen(device1->hostname) == 0) || (device1->hostname[0] ==  0xff))
	{
		strcpy(hostn,"WifiKaRadio");
		strcpy(device1->hostname,hostn);
		saveDeviceSettings1(device1);
	}
	else strcpy(hostn,device1->hostname);
	printf(PSTR("HOSTNAME: %s\nLocal IP: %s\n"),hostn,localIp);
	initMDNS(hostn,info->ip.addr);
	
	free(info);
	free(device);
	free(device1);
	free(config);

}


/******************************************************************************
 * FunctionName : checkUart
 * Description  : Check for a valid uart baudrate
 * Parameters   : baud
 * Returns      : baud
*******************************************************************************/
uint32_t checkUart(uint32_t speed)
{
	uint32_t valid[] = {1200,2400,4800,9600,14400,19200,28800,38400,57600,76880,115200,230400};
	int i = 0;
	for (i; i<12; i++) {
		if (speed == valid[i]) return speed;
	}
	return 115200; // default
}


void uartInterfaceTask(void *pvParameters) {
  char tmp[255];
  struct device_settings *device;
  uint32_t uspeed;

  initWifi();
  uint16_t ap = 0;
  int i = 0;
  uint8_t maxap;

  /*	int uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
  	printf("watermark uartInterfaceTask: %d  %d\n","uartInterfaceTask",uxHighWaterMark,xPortGetFreeHeapSize( ));
  */

  int t = 0;
  for (t = 0; t < sizeof(tmp); t++) tmp[t] = 0;
  t = 0;

  //autostart
  device = getDeviceSettings();
  currentStation = device->currentstation;
  VS1053_I2SRate(device->i2sspeed);
  clientIvol = device->vol;

  uspeed = device->uartspeed;
  uspeed = checkUart(uspeed);
  uart_set_baud(0, uspeed);

  printf(striUART, 0x0d);

  ap = 0;
  ap = sdk_system_adc_read();
  if (ap < 10) {
    adcdiv = 0; // no panel adc grounded
    kprintf(PSTR("No panel%c"), 0x0d);
  } else {
    // read adc to see if it is a nodemcu with adc dividor
    if (ap < 400) adcdiv = 3;
    else adcdiv = 1;
    kprintf(PSTR("ADC Div: %d from adc: %d\n"), adcdiv, ap);
  }

  FlashOn = 190;
  FlashOff = 10;

  if ((sdk_wifi_get_opmode() == STATION_MODE)&&(device->autostart ==1))
	{
		kprintf(PSTR("autostart: playing:%d, currentstation:%d\n"),device->autostart,device->currentstation);
		vTaskDelay(10);
		playStationInt(device->currentstation);
  }

  free(device);

  while (1) {
    while (1) {
      int c = uart_getc(0);
      if (c != -1) {
        if ((char) c == '\r') break;
        if ((char) c == '\n') break;
        tmp[t] = (char) c;
        t++;
        if (t == sizeof(tmp) - 1) t = 0;
      }
      switchCommand(); // hardware panel of command
    }
    checkCommand(t, tmp);
    /*	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    	printf("watermark uartInterfaceTask: %d  heap:%d\n",uxHighWaterMark,xPortGetFreeHeapSize( ));
    */
    for (t = 0; t < sizeof(tmp); t++) tmp[t] = 0;
    t = 0;
  }
}


/******************************************************************************
 * FunctionName : test_upgrade
 * Description  : check if it is an upgrade. Convert if needed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
/*void test_upgrade(void)
{
	uint8_t autotest;
	struct device_settings *settings;
	struct shoutcast_info* station;
	int j;
	eeGetOldData(0x0C070, &autotest, 1);
	printf ("Upgrade autotest before %d\n",autotest);
	if (autotest == 3) // old bin before 1.0.6
	{
		autotest = 0; //patch espressif 1.4.2 see http://bbs.espressif.com/viewtopic.php?f=46&t=2349
		eeSetOldData(0x0C070, &autotest, 1);
		printf ("Upgrade autotest after %d\n",autotest);
		settings = getOldDeviceSettings();
		saveDeviceSettings(settings);
		free(settings);
		eeEraseStations();
		for(j=0; j<192; j++){
			station = getOldStation(j) ;	
			saveStation(station, j);
			free(station);			
			vTaskDelay(1); // avoid watchdog
		}
		
	}		
}
*/


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void) {
  struct device_settings *device;
  uint32_t uspeed;

  //REG_SET_BIT(0x3ff00014, BIT(0));
  //system_update_cpu_freq(SYS_CPU_160MHZ);
  sdk_system_update_cpu_freq(160); // See more at: http://www.esp8266.com/viewtopic.php?p=8107#p8107

  TaskHandle_t pxCreatedTask;
  Delay(100);
  getFlashChipRealSize();
  device = getDeviceSettings();
  uspeed = device->uartspeed;
  free(device);
  uspeed = checkUart(uspeed);
  uart_set_baud(0, uspeed);

  VS1053_HW_init(); // init spi
  extramInit();

  printf(PSTR("\nuart speed: %d\n"), uspeed);
  initBuffer();

  printf(PSTR("Release %s, Revision %s\n"), RELEASE, REVISION);
  printf(PSTR("SDK %s\n"), sdk_system_get_sdk_version());
  sdk_system_print_meminfo();
  printf(PSTR("Heap size: %d\n"), xPortGetFreeHeapSize());
  clientInit();
  //Delay(10);

  //flash_size_map size_map = system_get_flash_size_map();
  //printf (PSTR("size_map: %d\n"),size_map);

  printf(PSTR("Flash size: %d\n"),getFlashChipRealSize());

  xTaskCreate(testtask, "t0", 140, NULL, 1, &pxCreatedTask); // DEBUG/TEST 130
  printf(striTASK,"t0",pxCreatedTask);
  xTaskCreate(uartInterfaceTask, "t1", 370, NULL, 2, &pxCreatedTask); // 350
  printf(striTASK, "t1",pxCreatedTask);
  xTaskCreate(vsTask, "t2", 240, NULL,5, &pxCreatedTask); //380 230
  printf(striTASK,"t2",pxCreatedTask);
  xTaskCreate(clientTask, "t3", 500, NULL, 6, &pxCreatedTask); // 340
  printf(striTASK,"t3",pxCreatedTask);
  xTaskCreate(serversTask, "t4", 370, NULL, 4, &pxCreatedTask); //380
  printf(striTASK,"t4",pxCreatedTask);
  printf (striHEAP,xPortGetFreeHeapSize( ));
}
