//
// Created by sheverdin on 10/3/23.
//

#include "dm_mainHeader.h"
#include "run_CLIcmd.h"
#include "parsing.h"
#include <ctype.h>
#include <string.h>

#define COMMAND_LEN (64)

const char *delimSpace  = " ";
const char *delim1 = "/";
const char *delimDot = ".";
const char *delimDoubleDot = ":";
const char *delimEqual = "=";

static void getConfigParam(cmd_type_e cmdType, char *configParam, char * logMgs);
static void getMngmntnDevice(cmd_type_e cmdType, char *param, char * logMgs);

static void getIpaddr(search_out_msg_t *searchOutMsg);
static void getMacaddr(search_out_msg_t *searchOutMsg);
static void getMask(search_out_msg_t *searchOutMsg);
static void geGateWay(search_out_msg_t *searchOutMsg);

typedef struct
{
    char ifName[16];
    char deviceName[16];
    char mngtDevice[16];
}CONFIG_PARAM_t;

CONFIG_PARAM_t configParam;

const char CommandTable[CMD_MAX][COMMAND_LEN] =
{
    {"/etc/tf_device_monitor_scripts/getNetDeviceInfo.lua if_name"},   // 0
    {"/etc/tf_device_monitor_scripts/getNetDeviceInfo.lua dev_name"},  // 1
    {"/etc/tf_device_monitor_scripts/getMngtDevice.lua"},              // 2
    {"/etc/tf_device_monitor_scripts/getNetData.lua ipaddr"},          // 3
    {"/etc/tf_device_monitor_scripts/getNetData.lua netmask"},         // 4
    {"/etc/tf_device_monitor_scripts/getNetData.lua macaddr"},         // 5
    {"/etc/tf_device_monitor_scripts/getNetData.lua gateWay"},         // 6
    {"etc/tf_device_monitor_scripts/getUptime.lua"},                   // 7
    {"/etc/tf_device_monitor_scripts/getBoardInfo.lua"},               // 8
    {"/etc/tf_device_monitor_scripts/getSystemInfo.lua"},              // 9
};

/// ===============================================================
int getDeviceInfo(search_out_msg_t *searchOutMsg)
{
    char *logMsg = "Interface";
    openlog("dm_ConfigParam", LOG_PID, LOG_USER);
    getConfigParam(CMD_DEVICE_IF_NAME, configParam.ifName, logMsg);
    syslog(LOG_ERR, "%s: %s", logMsg, configParam.ifName);
    closelog();
    logMsg = "Device Name";
    getConfigParam(CMD_DEVICE_DEV_NAME, configParam.deviceName, logMsg);
    syslog(LOG_ERR, "%s: %s", logMsg, configParam.deviceName);
    logMsg = "mngmnt Device";
    getMngmntnDevice(CMD_DEVICE_MNGT, configParam.mngtDevice, logMsg);
    syslog(LOG_ERR, "%s: %s", logMsg, configParam.mngtDevice);
    return 0;
}

static void getConfigParam(cmd_type_e cmdType, char *param, char * logMgs)
{
    char output[OUTPUT_MAX_LENGTH];
    FILE *pipe = openPipe(CommandTable[cmdType]);
    openlog("dm_info", LOG_PID, LOG_USER);
    uint8_t lineCount = 0;
    while(fgets(output, OUTPUT_MAX_LENGTH, pipe))
    {
        lineCount++;
        if (lineCount == 1)
        {
            if (strlen(output) > 0)
            {
                strncpy(param, output, strlen(output));
            } else
            {
                syslog(LOG_INFO, "No %s found", logMgs);
            }
        }
        else
        {
            syslog(LOG_ERR, "In system more than 1 %s - count %d", logMgs, lineCount);
        }
    }
    closelog();
    closePipe(pipe);
}

static void getMngmntnDevice(cmd_type_e cmdType, char *param, char * logMgs)
{
    openlog("dm_info", LOG_PID, LOG_USER);
    char output[OUTPUT_MAX_LENGTH];
    char cmd[OUTPUT_MAX_LENGTH];
    strncpy(cmd, "\0", strlen("\0"));
    snprintf(cmd, sizeof(cmd), "%s %s",  CommandTable[cmdType], configParam.ifName);
    FILE *pipe = openPipe(cmd);
    uint8_t lineCount = 0;
    while(fgets(output, OUTPUT_MAX_LENGTH, pipe))
    {
        lineCount++;
        if (lineCount == 1)
        {
            if (strlen(output) > 0)
            {
                strncpy(param, output, strlen(output));
            } else
            {
                syslog(LOG_INFO, "No %s found", logMgs);
            }
        }
        else
        {
            syslog(LOG_ERR, "In system more than 1 %s - count %d", logMgs, lineCount);
        }
    }
    closelog();
    closePipe(pipe);
}

int getNETinfo(search_out_msg_t *searchOutMsg)
{
    uint8_t interfaceFlag = 0;
    uint8_t findMacFlag   = 0;
    uint8_t findIpFlag    = 0;
    openlog("dm_net_info", LOG_PID, LOG_USER);

    getIpaddr(searchOutMsg);
    getMask(searchOutMsg);
    getMacaddr(searchOutMsg);
    geGateWay(searchOutMsg);

    // if (!interfaceFlag)
    // {
    //     openlog("dm_err", LOG_PID, LOG_USER);
    //     syslog(LOG_ERR, "Interface not found");
    //     closelog();
    // }
    // if (!findMacFlag)
    // {
    //     openlog("dm_err", LOG_PID , LOG_USER);
    //     syslog(LOG_ERR, "MAC address not found");
    //     closelog();
    // }
    // if (!findIpFlag)
    // {
    //     openlog("dm_err", LOG_PID, LOG_USER);
    //     syslog(LOG_ERR, "IP address not found");
    //     closelog();
    // }
    // interfaceFlag   = 0;
    // findIpFlag      = 0;
    // findMacFlag     = 0;

    closelog();
    return 0;
}

static void getIpaddr(search_out_msg_t *msg)
{
    openlog("dm_net_info", LOG_PID, LOG_USER);
    char output[OUTPUT_MAX_LENGTH];
    char *logMsg = "IP address: ";
    char cmd[OUTPUT_MAX_LENGTH];
    strncpy(cmd, "\0", strlen("\0"));
    snprintf(cmd, sizeof(cmd), "%s %s",  CommandTable[CMD_NET_INFO_IP], configParam.ifName);

    FILE *pipe = openPipe(cmd);
    for (int i = 0; i < IPV4_LEN; i++)
    {
        fgets(output, OUTPUT_MAX_LENGTH, pipe);
        syslog(LOG_INFO, "param %s %s",logMsg, output);
        long ipBit = 0;
        ipBit = strtol(output, NULL, 10);
        if (ipBit >= 0 & ipBit < 256) {
            msg->struct1.ip[i] = ipBit;
        }
        else {
            syslog(LOG_INFO, "Wrong Ip bit %ld ", ipBit);
        }
    }
    closelog();
    closePipe(pipe);
}

static void getMacaddr(search_out_msg_t *msg)
{
    openlog("dm_net_info", LOG_PID, LOG_USER);
    char output[OUTPUT_MAX_LENGTH];
    char cmd[OUTPUT_MAX_LENGTH];
    char *logMsg = "Mac: ";
    strncpy(cmd, "\0", strlen("\0"));
    snprintf(cmd, sizeof(cmd), "%s %s",  CommandTable[CMD_NET_INFO_MAC], configParam.deviceName);

    FILE *pipe = openPipe(cmd);
    for (int i = 0; i < MAC_LEN; i++)
    {
        fgets(output, OUTPUT_MAX_LENGTH, pipe);
        long macBit = 0;
        macBit = strtol(output, NULL, 16);
        syslog(LOG_INFO, "logMgs: %s %s %ld", logMsg, output, macBit);
        if (macBit >= 0 & macBit < 256)
        {
            msg->struct1.mac[i] = macBit;
        } else
        {
            syslog(LOG_INFO, "Wrong mac bit %ld ", macBit);
        }
    }
    closelog();
    closePipe(pipe);
}

static void getMask(search_out_msg_t *msg)
{
    openlog("dm_net_info", LOG_PID, LOG_USER);
    char output[OUTPUT_MAX_LENGTH];
    char cmd[OUTPUT_MAX_LENGTH];
    char *logMsg = "Mask: ";
    strncpy(cmd, "\0", strlen("\0"));
    snprintf(cmd, sizeof(cmd), "%s %s",  CommandTable[CMD_NET_INFO_MASK], configParam.ifName);

    FILE *pipe = openPipe(cmd);
    for (int i = 0; i < IPV4_LEN; i++) {
        fgets(output, OUTPUT_MAX_LENGTH, pipe);
        long ipBit = 0;
        ipBit = strtol(output, NULL, 10);
        syslog(LOG_INFO, "logMgs: %s %s %ld", logMsg, output, ipBit);
        if (ipBit >= 0 & ipBit < 256 ) {
            msg->struct1.mask[i] = ipBit;
        }
        else {
            syslog(LOG_INFO, "Wrong Ip bit %ld ", ipBit);
        }
    }
    closelog();
    closePipe(pipe);
}

static void geGateWay(search_out_msg_t *msg)
{
    openlog("dm_net_info", LOG_PID, LOG_USER);
    char output[OUTPUT_MAX_LENGTH];
    char cmd[OUTPUT_MAX_LENGTH];
    char *logMsg = "Gateway: ";
    strncpy(cmd, "\0", strlen("\0"));
    snprintf(cmd, sizeof(cmd), "%s %s",  CommandTable[CMD_NET_INFO_GATE_WAY], configParam.mngtDevice);

    FILE *pipe = openPipe(cmd);
    long ipBit = 0;
    for (int i = 0; i < IPV4_LEN; i++)
    {
        fgets(output, OUTPUT_MAX_LENGTH, pipe);
        ipBit = strtol(output, NULL, 10);
        syslog(LOG_INFO, "logMgs: %s %s %ld", logMsg, output, ipBit);
        if (ipBit >= 0 & ipBit < 256 ) {
            msg->struct1.gate[i] = ipBit;
        }
        else {
            syslog(LOG_INFO, "Wrong Ip bit %ld ", ipBit);
        }
    }
    closelog();
    closePipe(pipe);
}

int getTimeInfo(search_out_msg_t *searchOutMsg)
{
    char output[OUTPUT_MAX_LENGTH];
    uint8_t  uptimeFlag = 0;
    FILE *pipe = openPipe(CommandTable[CMD_TIME_INFO]);
    uint8_t lineCount = 0;
    while (fgets(output, OUTPUT_MAX_LENGTH, pipe))
    {
        uptimeFlag = 1;
        lineCount ++;
        if (lineCount == 1)
        {
            syslog(LOG_INFO, "Uptime output: %s", output);
            uint32_t d_uptime = strtol(output, NULL, 10);
            syslog(LOG_INFO, "Uptime : %d", d_uptime);
            searchOutMsg->struct1.uptime[0] = (d_uptime) & (0xFF);
            searchOutMsg->struct1.uptime[1] = (d_uptime >> 8 * 1) & (0xFF);
            searchOutMsg->struct1.uptime[2] = (d_uptime >> 8 * 2) & (0xFF);
            searchOutMsg->struct1.uptime[3] = (d_uptime >> 8 * 3) & (0xFF);
            syslog(LOG_INFO, "struct1.uptime 1: %x", searchOutMsg->struct1.uptime[0]);
            syslog(LOG_INFO, "struct1.uptime 2: %x", searchOutMsg->struct1.uptime[1]);
            syslog(LOG_INFO, "struct1.uptime 3: %x", searchOutMsg->struct1.uptime[2]);
            syslog(LOG_INFO, "struct1.uptime 4 %x", searchOutMsg->struct1.uptime[3]);
        }
        else {
            syslog(LOG_ERR, "In system more than 1 Uptime - count: %d", lineCount);
        }
    }
    if(!uptimeFlag)
    {
        openlog("dm_err", LOG_PID, LOG_USER);
        syslog(LOG_ERR, "Uptime not found");
        closelog();
    }
    closePipe(pipe);
    return EXIT_SUCCESS;
}

int getBoardInfo(search_out_msg_t *searchOutMsg)
{
    char output[OUTPUT_MAX_LENGTH];
    char model[]        = "model";
    char version[]      = "version";
    uint8_t modelFlag   = 0;
    uint8_t versionFlag = 0;
    splited_line_t splitLineBoard;

    FILE *pipe = openPipe(CommandTable[CMD_BOARD_INFO]);

    while (fgets(output, OUTPUT_MAX_LENGTH, pipe) != NULL)
    {
        syslog(LOG_INFO, "model type LUA: %s", output);
        uint8_t modelType = strtol(output, NULL, 10);
        if (modelType > 0) {
            searchOutMsg->struct1.dev_type = modelType;
        }
        else {
            searchOutMsg->struct1.dev_type = 0;
            syslog(LOG_ERR, "Model not found");
        }
    }
    closelog();
    closePipe(pipe);

    char versionStr[] = "12.34.56";
    char versionCat[6] = "";
    versionFlag = 1;
    uint32_t version_num = 0;
   //// TODO Konst: In this time this part is sendBox. version number is const
    split_line(versionStr, delimDot, &splitLineBoard);
    searchOutMsg->struct1.firmware[2] = strtol(splitLineBoard.tokens[0], NULL, 10);
    searchOutMsg->struct1.firmware[1] = strtol(splitLineBoard.tokens[1], NULL, 10);
    searchOutMsg->struct1.firmware[0] = strtol(splitLineBoard.tokens[2], NULL, 10);
    return 0;
}

int getSystemInfo(search_out_msg_t *searchOutMsg)
{
    char output[OUTPUT_MAX_LENGTH];
    uint8_t locationFlag    = 0;
    uint8_t descriptionFlag = 0;

    FILE *pipe = openPipe(CommandTable[CMD_SYSTEM_INFO]);
    uint8_t lineCount = 0;
    openlog("dm_system", LOG_PID , LOG_USER);
    while (fgets(output, OUTPUT_MAX_LENGTH, pipe))
    {
        lineCount++;
        if (lineCount == 1) {
            locationFlag = 1;
            removeCharacter(output,'\'');
            for (int i = 0; i< MAX_LENGTH; i++ )
            {
                searchOutMsg->struct1.dev_loc[i] = '\0';
            }
            strncpy(searchOutMsg->struct1.dev_loc, output, strlen(output));
            syslog(LOG_INFO, "Location: %s", searchOutMsg->struct1.dev_loc);
            closelog();
        }
        else if (lineCount == 2) {
            descriptionFlag = 1;
            removeCharacter(output,'\'');
            for (int i = 0; i< MAX_LENGTH; i++ )
            {
                searchOutMsg->struct1.dev_descr[i] = '\0';
            }
            strncpy(searchOutMsg->struct1.dev_descr,output, strlen(output));
            syslog(LOG_INFO, "Description: %s", searchOutMsg->struct1.dev_descr);
        }
        else if (lineCount > 2) {
            syslog(LOG_INFO, "System Info more than 2 Line, count %d", lineCount);
        }
    }
    closelog();
    if(!locationFlag)
    {
        openlog("dm_err", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error: Location not found");
        closelog();
    }
    if(!descriptionFlag)
    {
        openlog("dm_err", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error: Description not found");
        closelog();
    }
    closePipe(pipe);
    return 0;
}

FILE *openPipe(const char *cmdStr)
{
    FILE *pipe = popen(cmdStr, "r");
    if (!pipe)
    {
        openlog("dm_err", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error to run command %s", cmdStr);
        closelog();
    }
    return pipe;
}

void closePipe(FILE *pipe)
{
    int status = pclose(pipe);

    if (!status)
    {
        return;
    }
    else
    {
        openlog("dm_err", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error close pipe, Status =  %d", status);
        closelog();
    }
}

void getDeviceName(char *devName)
{

    strncpy(devName, configParam.mngtDevice, strlen(configParam.mngtDevice));
}

void getDeviceNameLen(uint8_t *devNameLen)
{
    *devNameLen = strlen(configParam.mngtDevice);
    printf("dev name str = %d\n", *devNameLen);
}


