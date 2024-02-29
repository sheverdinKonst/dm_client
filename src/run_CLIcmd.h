//
// Created by sheverdin on 10/3/23.
//

#ifndef DM_CLIENT_RUN_CLICMD_H
#define DM_CLIENT_RUN_CLICMD_H

#include "dm_mainHeader.h"

#define COMMAND_MAX_LENGTH 100

#define DM_TOK_DELIM "\t\r\n\a"
int getDeviceInfo(search_out_msg_t  *searchOutMsg);
int getNETinfo(search_out_msg_t     *searchOutMsg);
int getSystemInfo(search_out_msg_t  *searchOutMsg);
int getBoardInfo(search_out_msg_t   *searchOutMsg);
int getUCInfo(search_out_msg_t      *searchOutMsg);
int getGateway(search_out_msg_t     *searchOutMsg);

void getDeviceName (char *devName);


#endif //DM_CLIENT_RUN_CLICMD_H
