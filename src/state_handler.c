//
// Created by sheverdin on 10/11/23.
//

#include "state_handler.h"
#include "net_utils.h"
#include "run_CLIcmd.h"
#include "parsing.h"
#include "settings_module.h"

static event_FillMsg_Callback_t eventFillMsg_CallbackArray[CLIENT_FILLMSG_NUM_OF_CALLBACKS];
static event_SendMsg_Callback_t eventSendMsg_CallbackArray[CLIENT_SENDMSG_NUM_OF_CALLBACKS];

static error_code_t client_emitEvent_fillmsg(void);
static error_code_t client_registerEvent_fillmsg(event_FillMsg_Callback_t func_fillMsgCallback);
static error_code_t client_unRegisterEvent_fillmsg(event_FillMsg_Callback_t func_fillMsgCallback);

static error_code_t client_emitEvent_sendmsg(void);
static error_code_t client_registerEvent_sendmsg(event_SendMsg_Callback_t func_fillMsgCallback);
static error_code_t client_unRegisterEvent_sendmsg(void);

static error_code_t client_handler(void);
static CLIENT_EVENT_e client_waitCommand(void);
static  error_code_t client_sendSearchMsg(void);
static  error_code_t client_sendSettMsg(void);
static error_code_t client_settings(void);

static pass_id_e checkMacAddress(void);
static pass_id_e checkMD5Sum(void);

static void mainTest(search_out_msg_t *searchMsg);

static uint8_t mainTestFlag = 0;

search_out_msg_t searchOutMsg;
SET_SETTINGS_MSG settingsMsg;
sett_msg_t settMsg;
pass_id_e passFlag = 0;

clientInfo_t clientInfo =
{
    .state = CLIENT_SATE_INIT,
    .event = CLIENT_EVENT_IDLE,
    .count = 0,
    .msgType = type_idle
};

int client_app(void)
{
    error_code_t errorCode = OK;
    openlog("dm_main", LOG_PID, LOG_USER);
    while(1)
    {
        errorCode = client_handler();
        if(errorCode)
        {
            syslog(LOG_ERR, "MAIN Error - %d", errorCode);
        }
    }
    return 0;
}

static error_code_t client_handler(void)
{
    openlog("dm_info", LOG_PID, LOG_USER);
    switch (clientInfo.state)
    {
        case CLIENT_SATE_IDLE:
        {

        }
        break;

        case CLIENT_SATE_INIT:
        {
            net_socketInit();
            client_registerEvent_fillmsg(&getDeviceInfo);
            client_registerEvent_fillmsg(&getNETinfo);
            client_registerEvent_fillmsg(&getTimeInfo);
            client_registerEvent_fillmsg(&getBoardInfo);
            client_registerEvent_fillmsg(&getSystemInfo);
            clientInfo.state = CLIENT_STATE_WAIT;
        }
        break;

        case CLIENT_STATE_WAIT:
        {
            openlog("dm_cmd", LOG_PID, LOG_USER);
            syslog(LOG_ERR, "Wait Command");
            closelog();
            clientInfo.event |= client_waitCommand();
            if (clientInfo.event & CLIENT_EVENT_FIND)
            {
                clientInfo.msgType = type_searchMsg;
                clientInfo.event &= (~CLIENT_EVENT_FIND);
                clientInfo.state = CLIENT_STATE_FILL_MSG;
                clientInfo.count = 0;
            }

            if (clientInfo.event & CLIENT_EVENT_SETTINGS)
            {
                clientInfo.msgType = type_settingsMsg;
                clientInfo.event &= (~CLIENT_EVENT_SETTINGS);
                clientInfo.state = CLIENT_STATE_SETTINGS;
                clientInfo.count = 0;
            }
        }
        break;

        case CLIENT_STATE_SEND:
        {
            error_code_t errorCode = OK;
            clientInfo.state = CLIENT_STATE_RELOAD;
            errorCode |= client_emitEvent_sendmsg();
            errorCode |= client_unRegisterEvent_sendmsg();
            return errorCode;
        }
        break;

        case CLIENT_STATE_FILL_MSG:
        {
            syslog(LOG_INFO, "CLIENT_STATE_FILL_MSG");
            client_emitEvent_fillmsg();
            searchOutMsg.struct1.msg_type = (uint8_t) ack_find_device;
            if (clientInfo.event & CLIENT_EVENT_MSG_READY)
            {
                syslog(LOG_INFO, "dm CLIENT_EVENT_MSG_READY");
                clientInfo.event &= (~CLIENT_EVENT_MSG_READY);
                client_registerEvent_sendmsg(client_sendSearchMsg);
                clientInfo.state = CLIENT_STATE_SEND;
            }
        }
        break;

        case CLIENT_STATE_SETTINGS:
        {
            error_code_t errorCode = OK;
            syslog(LOG_DEBUG, "dm STATE SETTINGS");

            errorCode = client_settings();
            for(int i = 0; i < 32; i++)
            {
                settingsMsg.settMsg.settings_struct.pass_md5[i] = 0;
            }
            settingsMsg.settMsg.settings_struct.msg_type = (uint8_t) ack_to_set_config;
            settingsMsg.settMsg.settings_struct.pass_md5[0] = errorCode;

            switch (errorCode)
            {
                case ERR_AUTHENTICATION:
                {
                    syslog(LOG_ERR, "auth. Error - %d", errorCode);
                    client_registerEvent_sendmsg(client_sendSettMsg);
                    clientInfo.state = CLIENT_STATE_SEND;
                }
                break;

                case OK:
                {
                    syslog(LOG_INFO, "OK  - %d", errorCode);
                    client_registerEvent_sendmsg(client_sendSettMsg);
                    SETTINGS_INFO_t *setInfo = getSettingInfo();
                    if(setInfo->applySystem == 1)
                    {
                        clientInfo.event |= CLIENT_EVENT_SYS_RELOAD;
                    }
                    if(setInfo->applyNetwork)
                    {
                        clientInfo.event |= CLIENT_EVENT_NET_RELOAD;
                    }
                    clientInfo.state = CLIENT_STATE_SEND;
                }
                break;

                case INFO_MAC_NOT_IDENTICAL:
                    clientInfo.state = CLIENT_STATE_WAIT;
                break;
            }
        }
        break;

        case CLIENT_STATE_RELOAD:
        {
            if (clientInfo.event & CLIENT_EVENT_SYS_RELOAD)
            {
                clientInfo.event &= (~CLIENT_EVENT_SYS_RELOAD);
                runSettingsCmd_f *runSettingsCmd = getRunSettingsCmd();
                runSettingsCmd[SETTINGS_SYS_RELOAD](" ");
            }

            if (clientInfo.event & CLIENT_EVENT_NET_RELOAD)
            {
                clientInfo.event &= (~CLIENT_EVENT_NET_RELOAD);
                runSettingsCmd_f *runSettingsCmd = getRunSettingsCmd();
                runSettingsCmd[SETTINGS_NET_RELOAD](" ");
            }
            clientInfo.state = CLIENT_STATE_WAIT;
        }
        break;

    }
    closelog();
    return 0;
}

static  error_code_t client_sendSettMsg(void)
{
    uint8_t *bufferAddress = getBufferAddress();
    memcpy(bufferAddress, settingsMsg.settMsg.buff, sizeof(settingsMsg.settMsg.buff));
    int error = net_sendMsg();
    return error;
}

static  error_code_t client_sendSearchMsg(void)
{
    openlog("dm_Buffer", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Read buffer address 1");

    uint8_t *bufferAddres = getBufferAddress();
    syslog(LOG_INFO, "Read buffer address 2");
    memcpy(bufferAddres, searchOutMsg.buff, sizeof(searchOutMsg.buff));

    syslog(LOG_INFO, "Read buffer address 2");
    int error = net_sendMsg();

    closelog();
    //if (mainTestFlag)
    {
        mainTest(&searchOutMsg);
    }
    return error;
}

static CLIENT_EVENT_e client_waitCommand(void)
{
    uint8_t cmd = 0;
    uint8_t data[MAX_BUFFER_SIZE];

    cmd = net_getCommand();
    CLIENT_EVENT_e event = CLIENT_EVENT_IDLE;

    switch(cmd)
    {
        case request_find_device:
        {
            event = CLIENT_EVENT_FIND;
        }
        break;

        case request_set_config:
        {
            event = CLIENT_EVENT_SETTINGS;
        }
        break;

        case request_to_update:
        {
            event = CLIENT_EVENT_UPDATE;
        }
        break;

        case ack_to_update:
        case ack_find_device:
        case ack_to_set_config:
        {
            event = CLIENT_EVENT_IDLE;
        }
        break;

        default:
        {
            openlog("dm_cmd", LOG_PID, LOG_USER);
            syslog(LOG_ERR, "Error wrong Command");
            closelog();
        }
    }
    return event;
}

static error_code_t client_settings(void)
{
    error_code_t errorCode = OK;
    DYNAMIC_MAP_CMD *settingsCmdMap = NULL;
    pass_id_e testID = 0;

    passFlag |= pass_mac;
    passFlag |= pass_md5;
    getBuffer((uint8_t*)&settingsMsg.settMsg.buff, 0);
    getBufferLen((size_t*)&settingsMsg.len);

    passFlag |= checkMacAddress();
    passFlag |= checkMD5Sum();
    openlog("dm_md5", LOG_PID, LOG_USER);
    syslog(LOG_ERR, "passFlag_1= %d", passFlag);

    if (passFlag & pass_mac)
    {
        if (passFlag & pass_md5)
        {
            syslog(LOG_ERR, "passFlag_2= %d", passFlag);
            //getBuffer((uint8_t*)&settingsMsg.settMsg.buff, SETTING_MSG_INDEX);
            splited_line_t splitedLineData;
            split_line((char *) &settingsMsg.settMsg.settings_struct.msg, "#", &splitedLineData);

            settingsCmdMap = getDynamicMapCmd_ptr(splitedLineData.splitLineMax + 1);
            errorCode = getSettingsCmdArray(splitedLineData, settingsCmdMap);

            errorCode = settings_handler(settingsCmdMap, splitedLineData.splitLineMax);
            free(settingsCmdMap);
        }
        else
        {
            errorCode = ERR_AUTHENTICATION;
        }
        closelog();
    }
    else
    {
        errorCode = INFO_MAC_NOT_IDENTICAL;
    }
    return errorCode;
}

static pass_id_e checkMacAddress(void)
{
    getNETinfo(&searchOutMsg);
    uint8_t  macAddress[MAC_LEN] = {0, 0, 0, 0, 0, 0};

    for (int i=0; i < MAC_LEN; i++)
    {
        if (searchOutMsg.struct1.mac[i] != settingsMsg.settMsg.settings_struct.mac[i])
        {
            i = MAC_LEN;
            passFlag &= (~pass_mac);
        }
    }
    return passFlag;
}

static pass_id_e checkMD5Sum()
{
    char output[MD5_LEN];
    uint8_t md5buff[MD5_LEN];

    FILE *fd = fopen("etc/dmhash", "r");
    if(fd == NULL)
    {
        openlog("dm_auth_err", LOG_PID | LOG_PERROR, LOG_AUTHPRIV);
        syslog(LOG_ERR, "Error open file");
        passFlag &= (~pass_md5);
        closelog();
    }
    else
    {
        fread(md5buff, sizeof(md5buff), MD5_LEN, fd);
        printf("check_pass = %s\n", md5buff);
        for (int i = 0; i < MD5_LEN; i++)
        {
            if (settingsMsg.settMsg.settings_struct.pass_md5[i] != md5buff[i])
            {
                i = MD5_LEN;
                openlog("dm_auth", LOG_PID | LOG_PERROR, LOG_AUTHPRIV);
                syslog(LOG_ERR, "auth. failed. md5 not corrected");
                passFlag &= (~pass_md5);
            }
        }
    }
    return passFlag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static error_code_t client_emitEvent_fillmsg(void)
{
    if(clientInfo.count < CMD_MAX)
    {
        if (eventFillMsg_CallbackArray[clientInfo.count] != NULL)
        {
            eventFillMsg_CallbackArray[clientInfo.count](&searchOutMsg);
        }
        openlog("dm_Fill_MSG", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "Count: %d", clientInfo.count);
        closelog();
        clientInfo.count++;
    }
    else if (clientInfo.count >= CMD_MAX)
    {
        openlog("dm_Fill_MSG", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "count >= CMD_MAX: ");
        closelog();
        clientInfo.event |= CLIENT_EVENT_MSG_READY;
    }
    return OK;
}

static error_code_t client_registerEvent_fillmsg(event_FillMsg_Callback_t func_fillMsgCallback)
{
    for (int i = 0 ; i < CLIENT_FILLMSG_NUM_OF_CALLBACKS ; i++)
    {
        if (eventFillMsg_CallbackArray[i] == NULL)
        {
            eventFillMsg_CallbackArray[i] = func_fillMsgCallback;
            return OK;
        }
    }
    return ERR_UNAVAILABLE_RESOURCE;
}

static error_code_t client_unRegisterEvent_fillmsg(event_FillMsg_Callback_t func_fillMsgCallback)
{
    for (int i = 0 ; i < CLIENT_FILLMSG_NUM_OF_CALLBACKS ; i++)
    {
        if (eventFillMsg_CallbackArray[i] == func_fillMsgCallback)
        {
            eventFillMsg_CallbackArray[i] = NULL;
            return OK;
        }
    }
    return ERR_OBJ_NOT_FOUND;
}

///=======================================================================================================
static error_code_t client_emitEvent_sendmsg(void)
{
    if (eventSendMsg_CallbackArray[clientInfo.msgType] != NULL)
    {
        eventSendMsg_CallbackArray[clientInfo.msgType]();
        return OK;
    }
    return ERR_OBJ_NOT_FOUND;
}

static error_code_t client_registerEvent_sendmsg(event_SendMsg_Callback_t func_fillMsgCallback)
{
    if (eventSendMsg_CallbackArray[clientInfo.msgType] == NULL)
    {
        eventSendMsg_CallbackArray[clientInfo.msgType] = func_fillMsgCallback;
        return OK;
    }
    return ERR_UNAVAILABLE_RESOURCE;
}

static error_code_t client_unRegisterEvent_sendmsg(void)
{
    if (eventSendMsg_CallbackArray[clientInfo.msgType] != NULL)
    {
        eventSendMsg_CallbackArray[clientInfo.msgType] = NULL;
        return OK;
    }
    return ERR_OBJ_NOT_FOUND;
}

static void mainTest(search_out_msg_t *searchMsg)
{
    printf("##   DEVICE TYPE: %d\n", searchMsg->struct1.dev_type);
    printf("##   IP: \t");
    for (int i = 0; i < IPV4_LEN; i++)
    {
        printf("%d", searchMsg->struct1.ip[i]);
        printf(" ");
    }
    printf("\n");

    printf("##   MAC: \t");
    for (int i = 0; i < MAC_LEN; i++)
    {
        printf("%d", searchMsg->struct1.mac[i]);
        printf(" ");
    }
    printf("\n");
    printf("##   DESCRIPTION: \n");
    printf("-> %s\n", searchMsg->struct1.dev_descr);
    printf("##   LOCATION: \n");
    printf("-> %s\n", searchMsg->struct1.dev_loc);
    printf("##   UP_TIME: \t");

    uint32_t uptime = 0;
    printf(" %x ", searchMsg->struct1.uptime[3]);
    printf(" %x ", searchMsg->struct1.uptime[2]);
    printf(" %x ", searchMsg->struct1.uptime[1]);
    printf(" %x \t", searchMsg->struct1.uptime[0]);

    uptime |= (searchMsg->struct1.uptime[3] & 0xFFFFFFFF) << 8 * 3;
    uptime |= (searchMsg->struct1.uptime[2] & 0xFFFFFFFF) << 8 * 2;
    uptime |= (searchMsg->struct1.uptime[1] & 0xFFFFFFFF) << 8 * 1;
    uptime |= (searchMsg->struct1.uptime[0] & 0xFFFFFFFF);
    printf(" = %d\n", uptime);

    printf("##   FIRMWARE VERSION: \t");
    printf(" %d %d %d\n", searchMsg->struct1.firmware[0], searchMsg->struct1.firmware[1],
           searchMsg->struct1.firmware[2]);

    printf("##  MASK: \t");
    for (int i = 0; i < IPV4_LEN; i++)
    {
        printf("%d", searchMsg->struct1.mask[i]);
        printf(" ");
    }
    printf("\n");

    printf("##   GATEWAY: \t");
    for (int i = 0; i < IPV4_LEN; i++)
    {
        printf("%d", searchMsg->struct1.gate[i]);
        printf(" ");
    }
    printf("\n");
}

void getMainBuff(uint8_t *mainBuff)
{
    {
        memcpy(mainBuff, searchOutMsg.buff, sizeof(searchOutMsg.buff));
    }
}

void setMainTestFlag(uint8_t value)
{
    mainTestFlag = value;
}

