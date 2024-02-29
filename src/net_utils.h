//
// Created by sheverdin on 10/3/23.
//

#ifndef UNTITLED_NET_UTILS_H
#define UNTITLED_NET_UTILS_H

#include "dm_mainHeader.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT (6123)

int net_socketInit(void);
uint8_t net_getCommand(void);
uint8_t net_sendMsg(void);
uint8_t *getBufferAddress(void);
void getBuffer(uint8_t *settingsBuffer, uint32_t index);
void getBufferLen(size_t *len);

#endif //UNTITLED_NET_UTILS_H
