/******************************************************************************
 * Copyright (C) 2014 -2016  Espressif System
 *
 * FileName: user_upgrade.c
 *
 * Description: downlaod upgrade userbin file from upgrade server
 *
 * Modification history:
 * 2015/7/3, v1.0 create this file.
*******************************************************************************/



#define PROGMEM  __attribute__((section(".irom.text")))
#define STORE_ATTR __attribute__((aligned(4)))

#define PSTR(s) (__extension__({ static const char __c[] STORE_ATTR __attribute__((section(".irom.text"))) = {s}; &__c[0];}))

#include <espressif/esp_common.h>
#include <espressif/user_interface.h>

#include <etstimer.h>
#include <espressif/esp_system.h>
#include <espressif/esp_timer.h>
#include <espressif/osapi.h>

#include "lwip/mem.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "upgrade.h"
#include "interface.h"
#include "websocket.h"


/*the size cannot be bigger than below*/
#define UPGRADE_DATA_SEG_LEN 1460
#define UPGRADE_RETRY_TIMES 10

LOCAL sdk_os_timer_t upgrade_10s;
LOCAL uint32_t totallength = 0;
LOCAL uint32_t sumlength = 0;
LOCAL BOOL flash_erased=0;
LOCAL BOOL giveup =0;

char *precv_buf=NULL;
sdk_os_timer_t upgrade_timer;
TaskHandle_t *pxCreatedTask=NULL;


////////////////////////////////////////////////
struct upgrade_param {
    uint32_t fw_bin_addr;
    uint16_t fw_bin_sec;
    uint16_t fw_bin_sec_num;
    uint16_t fw_bin_sec_earse;
    uint8_t extra;
    uint8_t save[4];
    uint8_t *buffer;
};

typedef enum {
    FLASH_SIZE_4M_MAP_256_256 = 0,  /**<  Flash size : 4Mbits. Map : 256KBytes + 256KBytes */
    FLASH_SIZE_2M,                  /**<  Flash size : 2Mbits. Map : 256KBytes */
    FLASH_SIZE_8M_MAP_512_512,      /**<  Flash size : 8Mbits. Map : 512KBytes + 512KBytes */
    FLASH_SIZE_16M_MAP_512_512,     /**<  Flash size : 16Mbits. Map : 512KBytes + 512KBytes */
    FLASH_SIZE_32M_MAP_512_512,     /**<  Flash size : 32Mbits. Map : 512KBytes + 512KBytes */
    FLASH_SIZE_16M_MAP_1024_1024,   /**<  Flash size : 16Mbits. Map : 1024KBytes + 1024KBytes */
    FLASH_SIZE_32M_MAP_1024_1024,   /**<  Flash size : 32Mbits. Map : 1024KBytes + 1024KBytes */
    FLASH_SIZE_32M_MAP_2048_2048,   /**<  attention: don't support now ,just compatible for nodemcu;
                                           Flash size : 32Mbits. Map : 2048KBytes + 2048KBytes */
    FLASH_SIZE_64M_MAP_1024_1024,   /**<  Flash size : 64Mbits. Map : 1024KBytes + 1024KBytes */
    FLASH_SIZE_128M_MAP_1024_1024   /**<  Flash size : 128Mbits. Map : 1024KBytes + 1024KBytes */

} flash_size_map;


LOCAL struct upgrade_param *upgrade;

//extern SpiFlashChip *flashchip;

LOCAL bool OUT_OF_RANGE(uint16_t erase_sec)
{
	uint8_t spi_size_map = sdk_flashchip.chip_size;
	uint16_t sec_num = 0;
	uint16_t start_sec = 0;
	if (spi_size_map == FLASH_SIZE_8M_MAP_512_512 ||
			spi_size_map ==FLASH_SIZE_16M_MAP_512_512 ||
			spi_size_map ==FLASH_SIZE_32M_MAP_512_512){
			start_sec = (sdk_system_upgrade_userbin_check() == USER_BIN2)? 1:129;
			sec_num = 123;
	} else if(spi_size_map == FLASH_SIZE_16M_MAP_1024_1024 ||
			spi_size_map == FLASH_SIZE_32M_MAP_1024_1024){
			start_sec = (sdk_system_upgrade_userbin_check() == USER_BIN2)? 1:257;
			sec_num = 251;
	} else {
			start_sec = (sdk_system_upgrade_userbin_check() == USER_BIN2)? 1:65;
			sec_num = 59;
	}
	if((erase_sec >= start_sec) &&(erase_sec <= (start_sec + sec_num)))
	{
		return false;
	} else {
		return true;
	}
	
}


/******************************************************************************
 * FunctionName : wsUpgrade
 * Description  : send the OTA feedback to websockets
 * Parameters   : number to send as string
 * Returns      : none
*******************************************************************************/
void wsUpgrade(const char* str,int count,int total)
{
	char answer[80];
	memset(answer,0,80);
	if (strlen(str)!= 0)
	{
		kasprintf(answer,PSTR("{\"upgrade\":\"%s\"}"),str);
		kprintf(answer);kprintf(PSTR("\n"));
	}
	else		
	{
		uint32_t value = count*100/total;
/*		if (value >= 100)
			strcpy(answer,PSTR("{\"upgrade\":\"Done. Refresh the page.\"}"));
		else
*/
		if (value == 0)
		{
			strcpy(answer,"{\"upgrade\":\"Started.\"}");
			kprintf(answer);kprintf(PSTR("\n"));
		}
		else
		{
//			strcpy(answer,PSTR("{\"upgrade\":\"%d / 100\"}"));
			sprintf(answer,"{\"upgrade\":\"%d / 100\"}",value);
			kprintf(PSTR("Written  %d  of  %d\n"),count,total);
		}
	}
//printf("answer: %s\n",answer);
	websocketbroadcast(answer, strlen(answer));
	vTaskDelay(5);
}



/******************************************************************************
 * FunctionName : user_upgrade_internal
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL bool system_upgrade_internal(struct upgrade_param *upgrade, uint8_t *data, u32 len)
{
    bool ret = false;
    uint16_t secnm=0;
    if(data == NULL || len == 0)
    {
        return true;
    }

    /*got the sumlngth,erase all upgrade sector*/
    if(len > SPI_FLASH_SEC_SIZE ) {
        upgrade->fw_bin_sec_earse=upgrade->fw_bin_sec;

        secnm=((upgrade->fw_bin_addr + len)>>12) + (len&0xfff?1:0);
        while(upgrade->fw_bin_sec_earse != secnm) {
            portENTER_CRITICAL();
			if( OUT_OF_RANGE( upgrade->fw_bin_sec_earse) )
			{
				printf("fw_bin_sec_earse:%d, Out of range\n",upgrade->fw_bin_sec_earse);
				break;
			
			}
			else
			{
//				printf("spi flash erase sector %d\n",upgrade->fw_bin_sec_earse);
				WRITE_PERI_REG(0x60000914, 0x73);
				 sdk_spi_flash_erase_sector(upgrade->fw_bin_sec_earse);
				 upgrade->fw_bin_sec_earse++;
			}
			portEXIT_CRITICAL();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        printf(PSTR("flash erase over\n"));
        return true;
    }
    
    upgrade->buffer = (uint8_t *)zalloc(len + upgrade->extra);

    memcpy(upgrade->buffer, upgrade->save, upgrade->extra);
    memcpy(upgrade->buffer + upgrade->extra, data, len);

    len += upgrade->extra;
    upgrade->extra = len & 0x03;
    len -= upgrade->extra;

    if(upgrade->extra<=4)
        memcpy(upgrade->save, upgrade->buffer + len, upgrade->extra);
    else
        printf(PSTR("ERR3:arr_overflow,%u,%d\n"),__LINE__,upgrade->extra);

    do {
        if (upgrade->fw_bin_addr + len >= (upgrade->fw_bin_sec + upgrade->fw_bin_sec_num) * SPI_FLASH_SEC_SIZE) {
            break;
        }

        if (sdk_spi_flash_write(upgrade->fw_bin_addr, (uint32_t *)upgrade->buffer, len) != SPI_FLASH_RESULT_OK) {
            break;
        }
        ret = true;
        upgrade->fw_bin_addr += len;
    } while (0);

    free(upgrade->buffer);
    upgrade->buffer = NULL;
    return ret;
}

/******************************************************************************
 * FunctionName : system_get_fw_start_sec
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
uint16_t system_get_fw_start_sec()
{
	if(upgrade != NULL) {
		return upgrade->fw_bin_sec;
	} else {
		return 0;
	}
}

/******************************************************************************
 * FunctionName : user_upgrade
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
bool system_upgrade(uint8_t *data, uint32_t len)
{
    bool ret;

/* for connect data debug
    if(len < 1460){
        char *precv_buf = (char*)malloc(1480);
        memcpy(precv_buf, data,len);
        memcpy(precv_buf+len,"\0\r\n",3);
        printf("%s\n",precv_buf);
        free(precv_buf);
    }
*/
    ret = system_upgrade_internal(upgrade, data, len);

    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade_init
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
void system_upgrade_init(void)
{
        uint32_t user_bin2_start,user_bin1_start;
	uint8_t spi_size_map = sdk_flashchip.chip_size;
	
	if (upgrade == NULL) 
	{
          upgrade = (struct upgrade_param *)zalloc(sizeof(struct upgrade_param));
        }
	
	user_bin1_start = 1; 

	if (spi_size_map == FLASH_SIZE_8M_MAP_512_512 || 
			spi_size_map ==FLASH_SIZE_16M_MAP_512_512 ||
			spi_size_map ==FLASH_SIZE_32M_MAP_512_512){
			user_bin2_start = 129;
			upgrade->fw_bin_sec_num = 123;
	} else if(spi_size_map == FLASH_SIZE_16M_MAP_1024_1024 || 
			spi_size_map == FLASH_SIZE_32M_MAP_1024_1024){
			user_bin2_start = 257;
			upgrade->fw_bin_sec_num = 251;
	} else {
			user_bin2_start = 65;
			upgrade->fw_bin_sec_num = 59;
	}
   
    upgrade->fw_bin_sec = (sdk_system_upgrade_userbin_check() == USER_BIN1) ? user_bin2_start : user_bin1_start;

    upgrade->fw_bin_addr = upgrade->fw_bin_sec * SPI_FLASH_SEC_SIZE;
    
    upgrade->fw_bin_sec_earse = upgrade->fw_bin_sec;
}

/******************************************************************************
 * FunctionName : system_upgrade_deinit
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
void  system_upgrade_deinit(void)
{
	if (upgrade != NULL) {
		free(upgrade);
		upgrade = NULL;
	}else {
		return;
	}
}

/******************************************************************************
 * FunctionName : upgrade_deinit
 * Description  :
 * Parameters   :
 * Returns      : none
*******************************************************************************/
void LOCAL upgrade_deinit(void)
{
    if (sdk_system_upgrade_flag_check() != UPGRADE_FLAG_START)
    {
        system_upgrade_deinit();
        //system_upgrade_reboot();
    }
}

/******************************************************************************
 * FunctionName : upgrade_data_load
 * Description  : parse the data from server,send fw data to system interface 
 * Parameters   : pusrdata--data from server,
 *              : length--length of the pusrdata
 * Returns      : none
 *  
 * first data from server:
 * HTTP/1.1 200 OK
 * Server: nginx/1.6.2
 * Date: Tue, 14 Jul 2015 09:15:51 GMT
 * Content-Type: application/octet-stream
 * Content-Length: 282448
 * Connection: keep-alive
 * Content-Disposition: attachment;filename=user2.bin
 * Vary: Cookie
 * X-RateLimit-Remaining: 3599
 * X-RateLimit-Limit: 3600
 * X-RateLimit-Reset: 1436866251
*******************************************************************************/
BOOL upgrade_data_load(char *pusrdata, unsigned short length)
{
    char *ptr = NULL;
    char *ptmp2 = NULL;
    char lengthbuffer[32];

    
    if (totallength == 0 && (ptr = (char *)strstr(pusrdata, "\r\n\r\n")) != NULL &&
            (ptr = (char *)strstr(pusrdata, "Content-Length")) != NULL) {

 //       kprintf("\n pusrdata %s\n",pusrdata);

        ptr = (char *)strstr(pusrdata, "Content-Length: ");
        if (ptr != NULL) {
            ptr += 16;
            ptmp2 = (char *)strstr(ptr, "\r\n");

            if (ptmp2 != NULL) {
                memset(lengthbuffer, 0, sizeof(lengthbuffer));
                
                if((ptmp2 - ptr)<=32)
                     memcpy(lengthbuffer, ptr, ptmp2 - ptr);
                else
                     kprintf(PSTR("ERR1:arr_overflow,%u,%d\n"),__LINE__,(ptmp2 - ptr));
                
                sumlength = atoi(lengthbuffer);
                kprintf(PSTR("userbin sumlength:%d \n"),sumlength);
                
                ptr = (char *)strstr(pusrdata, "\r\n\r\n");
                length -= ptr - pusrdata;
                length -= 4;
                totallength += length;

                /*at the begining of the upgrade,we get the sumlength 
                 *and erase all the target flash sectors,return false
                 *to close the connection, and start upgrade again.  
                 */
                if(FALSE==flash_erased){
//					kprintf("userbin sumlength:%d  flash:%d\n",sumlength,flash_erased);
                    flash_erased=system_upgrade(ptr + 4, sumlength);
					kprintf(PSTR("userbin sumlength:%d  flash:%d\n"),sumlength,flash_erased);
                    return flash_erased;
                }else{
                    system_upgrade(ptr + 4, length);
                }
            } else {
                kprintf(PSTR("ERROR:Get sumlength failed%c"),0x0d);
                return false;
            }
        } else {
            kprintf(PSTR("ERROR:Get Content-Length failed%c"),0x0d);
            return false;
        }
        
    } 
    else {
        if(totallength != 0){
            totallength += length;
            
            if(totallength > sumlength){
                kprintf(PSTR("strip the 400 error mesg%c"),0x0d);
                length =length -(totallength- sumlength);
            }
            
//            kprintf(">>>recv %dB, %dB left\n",totallength,sumlength-totallength);
            system_upgrade(pusrdata, length);
            
        } else {
            kprintf(PSTR("server response with something else,check it!%c"),0x0d);
            return false;
        }
    }

    return true;
}

/******************************************************************************
 * FunctionName : upgrade_task
 * Description  : task to connect with target server and get firmware data 
 * Parameters   : pvParameters--save the server address\port\request frame for
 *              : the upgrade server\call back functions to tell the userapp
 *              : the result of this upgrade task
 * Returns      : none
*******************************************************************************/
void upgrade_task(void *pvParameters)
{
    int recbytes;
    int sta_socket;
    int retry_count = 0;
    struct ip_info ipconfig;
    
    struct upgrade_server_info *server = pvParameters;

    flash_erased=FALSE;
    precv_buf = (char*)malloc(UPGRADE_DATA_SEG_LEN);
    if(NULL == precv_buf){
        kprintf(PSTR("upgrade_task,memory exhausted, check it%c"),0x0d);
    }
    
    while (retry_count++ < UPGRADE_RETRY_TIMES) {
		
		if (giveup) {kprintf(PSTR("giveup !\r%c"),0x0d);break;}
        
        sdk_wifi_get_ip_info(STATION_IF, &ipconfig);

        /* check the ip address or net connection state*/
        while (ipconfig.ip.addr == 0) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            sdk_wifi_get_ip_info(STATION_IF, &ipconfig);
        }
        
        sta_socket = socket(PF_INET,SOCK_STREAM,0);
        if (-1 == sta_socket) {
            close(sta_socket);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            kprintf(PSTR("socket fail !\r%c"),0x0d);
            continue;
        }

        /*for upgrade connection debug*/
        //server->sockaddrin.sin_addr.s_addr= inet_addr("192.168.1.170");

        if(0 != connect(sta_socket,(struct sockaddr *)(&server->sockaddrin),sizeof(struct sockaddr))) {
            close(sta_socket);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
//            kprintf(PSTR("connect fail!\n"));
			wsUpgrade("Connect to server failed!" , 0,100);
            continue;
        }
        kprintf(PSTR("Connect ok!%c"),0x0d);

        system_upgrade_init();
        sdk_system_upgrade_flag_set(UPGRADE_FLAG_START);

        if(write(sta_socket,server->url,strlen(server->url)) < 0) {
            close(sta_socket);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            kprintf(PSTR("send fail%c"),0x0d);
            continue;
        }
        kprintf(PSTR("Request send success%c"),0x0d);

        while((recbytes = read(sta_socket, precv_buf, UPGRADE_DATA_SEG_LEN)) > 0) {
            if(FALSE==flash_erased){
					close(sta_socket);
					kprintf(PSTR("pre erase flash!%c"),0x0d);
					if(false == upgrade_data_load(precv_buf,recbytes)){
					kprintf(PSTR("upgrade data error!%c"),0x0d);
					close(sta_socket);
					flash_erased=FALSE;
					vTaskDelay(1000 / portTICK_PERIOD_MS);
 //         	    break;
				}
//                upgrade_data_load(precv_buf,recbytes);
                break;                    
            }
            
            if(false == upgrade_data_load(precv_buf,recbytes)) {
                kprintf(PSTR("upgrade data error!%c"),0x0d);
                close(sta_socket);
                flash_erased=FALSE;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;
            }
//			kprintf(PSTR("Have written image length %d  of  %d\n"),totallength,sumlength);
			wsUpgrade( "",totallength,sumlength);
            /*this two length data should be equal, if totallength is bigger, 
             *maybe data wrong or server send extra info, drop it anyway*/
            if(totallength >= sumlength) {
//                kprintf(PSTR("upgrade data load finish.%c"),0x0d);
				wsUpgrade("upgrade load ok.",0,100);
                close(sta_socket);
                goto finish;
            }

//            kprintf("upgrade_task %d word left\n",uxTaskGetStackHighWaterMark(NULL));
            
        }
        
        if(recbytes <= 0) {
            close(sta_socket);
            flash_erased=FALSE;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
//            kprintf(PSTR("ERROR:read data fail!\r%c"),0x0d);
			wsUpgrade("read data fail!",0,100);
        }

        totallength =0;
        sumlength = 0;
    }
    
finish:

    sdk_os_timer_disarm(&upgrade_timer);

	if(upgrade_crc_check(system_get_fw_start_sec(),sumlength) != 0)
	{
		kprintf(PSTR("upgrade crc check failed !%c"),0x0d);
		server->upgrade_flag = false;
        sdk_system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
	}

    if(NULL != precv_buf) {
        free(precv_buf);
    }
    
    totallength = 0;
    sumlength = 0;
    flash_erased=FALSE;

    if(retry_count == UPGRADE_RETRY_TIMES)
    {
        /*retry too many times, fail*/
        server->upgrade_flag = false;
        sdk_system_upgrade_flag_set(UPGRADE_FLAG_IDLE);

    }
    else
    {
        if (server->upgrade_flag == true)
			sdk_system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
    }
    
    upgrade_deinit();
    
    kprintf(PSTR("\n Exit upgrade task.%c"),0x0d);
    if (server->check_cb != NULL) {
        server->check_cb(server);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : upgrade_check
 * Description  : check the upgrade process, if not finished in 300S,exit
 * Parameters   : pvParameters--save the server address\port\request frame for
 * Returns      : none
*******************************************************************************/
LOCAL void  upgrade_check(struct upgrade_server_info *server)
{
    /*network not stable, upgrade data lost, this may be called*/
//    vTaskDelete(pxCreatedTask);
	giveup = true;
    sdk_os_timer_disarm(&upgrade_timer);
    
    if(NULL != precv_buf) {
        free(precv_buf);
    }
    
    totallength = 0;
    sumlength = 0;
    flash_erased=FALSE;

    /*take too long to finish,fail*/
    server->upgrade_flag = false;
    sdk_system_upgrade_flag_set(UPGRADE_FLAG_IDLE);

    upgrade_deinit();
    
//    kprintf(PSTR("\n upgrade failed.%c"),0x0d);
	wsUpgrade("upgrade failed!" , 0,100);
    if (server->check_cb != NULL) {
        server->check_cb(server);
    }

}

/******************************************************************************
 * FunctionName : system_upgrade_start
 * Description  : task to connect with target server and get firmware data 
 * Parameters   : pvParameters--save the server address\port\request frame for
 *              : the upgrade server\call back functions to tell the userapp
 *              : the result of this upgrade task
 * Returns      : true if task created successfully, false failed.
*******************************************************************************/

BOOL system_upgrade_start(struct upgrade_server_info *server)
{
    portBASE_TYPE ret = 0;
    wsUpgrade( "",0,100);
    if(NULL == pxCreatedTask){
        ret = xTaskCreate(upgrade_task, "upgrade_task", 360, server, 8, pxCreatedTask);//224   1024, 890 left

        if(pdPASS == ret){
            sdk_os_timer_disarm(&upgrade_timer);
            sdk_os_timer_setfn(&upgrade_timer, (sdk_os_timer_func_t *)upgrade_check, server);
            sdk_os_timer_arm(&upgrade_timer, server->check_times, 0);
        }
    }
 
    return(pdPASS == ret);
}
