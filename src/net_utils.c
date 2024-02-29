//
// Created by sheverdin on 10/3/23.
//

#include <arpa/inet.h>
#include "net_utils.h"
#include "run_CLIcmd.h"

static void error(char *message);
static uint8_t net_closeSocket(int socket);

int socketRead;
struct sockaddr_in client_addr;
socklen_t addr_len = sizeof(client_addr);
uint8_t buffer[MAX_BUFFER_SIZE];

static size_t bufferLen = 0;

int net_socketInit(void)
{
    memset(&client_addr, 0, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(PORT);
    client_addr.sin_addr.s_addr =  htonl(INADDR_ANY);

    if ((socketRead = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        syslog(LOG_ALERT, "Socket creation failed");
        return (EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(socketRead, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        syslog(LOG_ALERT, "Setting SO_REUSEADDR option failed");
        close(socketRead);
        return (EXIT_FAILURE);
    }

    if (bind(socketRead, (const struct sockaddr*)&client_addr, sizeof(client_addr)) < 0)
    {
        syslog(LOG_ALERT, "Binding failed");
        close(socketRead);
        return (EXIT_FAILURE);
    }

    closelog();

    openlog("dm_socket", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Client listening on port %d...\n", PORT);
    closelog();
    return EXIT_SUCCESS;
}

uint8_t net_getCommand(void)
{
    uint8_t senderIP[IPV4_LEN] = {0,0,0,0};
    openlog("dm_cmd", LOG_PID, LOG_USER);
    uint8_t msgType;

    bufferLen = recvfrom(socketRead, (char*)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
    senderIP[3] = (client_addr.sin_addr.s_addr)&(0xFF);
    senderIP[2] = (client_addr.sin_addr.s_addr>>8*1)&(0xFF);
    senderIP[1] = (client_addr.sin_addr.s_addr>>8*2)&(0xFF);
    senderIP[0] = (client_addr.sin_addr.s_addr>>8*3)&(0xFF);

    if (bufferLen < 0)
    {
        syslog(LOG_ERR, "Error Received msg");
    }
    buffer[bufferLen] = '\0';
    msgType = buffer[0]&0xFF;
    bufferLen++;

    syslog(LOG_INFO, "Received msg from IP %d.%d.%d.%d,  Command: %.2x", senderIP[0], senderIP[1], senderIP[2], senderIP[3], msgType);
    closelog();
    return msgType;
}

// @depricated
void getMac(uint8_t *settingsBuff)
{
    for (int i = 1; i <= MAC_LEN; i++)
    {
        settingsBuff[i - 1] = buffer[i];
    }
}

// @depricated
void getMD5(uint8_t *settingsBuff)
{
    for (int i = 7; i < MD5_LEN+7; i++)
    {
        settingsBuff[i - 7] = buffer[i];
    }
}

uint8_t *getBufferAddress(void)
{
    return buffer;
}

void getBuffer(uint8_t *settingsBuffer, uint32_t index)
{
    if(bufferLen>0)
    {
        memcpy(settingsBuffer, buffer+index, bufferLen-index);
    }
}

void getBufferLen(size_t *len)
{
    *len = bufferLen-39;
}

uint8_t net_sendMsg(void)
{
    int broadcastEnable = 1;
    struct sockaddr_in client_addr_1;

    int sockSend_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockSend_fd < 0)
    {
        openlog("dm_socket", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Failed to create socket Send");
        closelog();
        return ERR_NETWORK;
    }

    //if (bind(sockSend_fd, (const struct sockaddr*)&client_addr_1, sizeof(client_addr_1)) < 0)
    //{
    //    syslog(LOG_ALERT, "Binding failed");
    //    close(socketRead);
    //    return (EXIT_FAILURE);
    //}

    //if (setsockopt(sockSend_fd, SOL_SOCKET, SO_BROADCAST|SO_DONTROUTE|SO_ERROR, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    if (setsockopt(sockSend_fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        openlog("dm_socket", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Failed to enable broadcast");
        closelog();
        return ERR_NETWORK;
    }
    char * devName = "";
    getDeviceName(devName);
    openlog("dm_TEST", LOG_PID , LOG_USER);
    syslog(LOG_ERR, "DEV name create socket  = %s", devName);
    closelog();

    int rc = setsockopt(sockSend_fd, SOL_SOCKET, SO_BINDTODEVICE, devName, strlen(devName)-1);
    if (rc<0)
    {
        syslog(LOG_ERR,"SO_BINDTODEVICE error RC = %d", rc);
    }

    memset(&client_addr_1, 0, sizeof(client_addr_1));
    client_addr_1.sin_family = AF_INET;
    client_addr_1.sin_port = htons(PORT);
    client_addr_1.sin_addr.s_addr = inet_addr("255.255.255.255");
    
    openlog("dm_socket", LOG_PID | LOG_PERROR, LOG_USER);
    ssize_t sendSize = 0;
    syslog(LOG_ERR,"buffer len = %zu", bufferLen-1);
    sendSize = sendto(sockSend_fd, buffer, bufferLen-1, 0, (struct sockaddr *)&client_addr_1, sizeof(client_addr_1));

    syslog(LOG_ERR, "Send massage = %zu", sendSize);
    closelog();

    if (sendSize < 0)
    {
        openlog("dm_socket", LOG_PID | LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "Error send massage");
        closelog();
        return ERR_NETWORK;
    }

    net_closeSocket(sockSend_fd);
    return EXIT_SUCCESS;
}

static uint8_t net_closeSocket(int socket)
{
    if (close(socket) < 0)
    {
        syslog(LOG_ERR, "Error socket close");
        return (ERR_NETWORK);
    }
    return EXIT_SUCCESS;
}

static void printError(char *message)
{
    perror(message);
    //exit(EXIT_FAILURE);
}
