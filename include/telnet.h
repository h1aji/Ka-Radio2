/*
 * Copyright 2017 karawin (http://www.karawin.fr)
 *
 * quick and dirty telnet inplementation for wifi webradio
 */

#ifndef __TELNET_H__
#define __TELNET_H__
// max size of the WS Message Header


#include <espressif/esp_common.h>
#include <espressif/esp_softap.h>
#include <espressif/esp_wifi.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

//#include "cencode_inc.h"
#include <stdbool.h>

#define NBCLIENTT 5
//#define MAXDATAT 256

//extern const char strtWELCOME[] ;
extern int telnetclients[NBCLIENTT];

// public:
// init some data
void telnetinit(void);
// a demand received, accept it
bool telnetAccept(int tsocket);
// a socket with a telnet
bool telnetnewclient(int socket);
// a socket with a telnet closed
void telnetremoveclient(int socket);
// is socket a telnet?
bool istelnet(int socket);

//write a txt data
void telnetWrite(uint32_t len,const char *fmt, ...);
//broadcast a txt data to all clients

// the telnet server task
void telnetTask(void* pvParams) ;

#endif
