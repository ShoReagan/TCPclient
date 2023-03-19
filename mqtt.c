// MQTT Library (framework only)
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: -
// Target uC:       -
// System Clock:    -

// Hardware configuration:
// -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "mqtt.h"
#include "timer.h"
#include "uart0.h"

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void processMqtt(etherHeader *ether, socket *s, uint8_t *buffer)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    mqttHeader *mqtt = (mqttHeader*)((uint8_t*)tcp->data);

    *buffer = mqtt->controlHeader;
}

void sendMqttMessage(etherHeader *ether, socket s, uint8_t str[], uint8_t topicLen, uint8_t type)
{
    uint8_t i;
    uint32_t sum;
    uint16_t tmp16;
    uint16_t tcpLength;
    uint16_t offsetFieldNum = 0;
    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];
    uint8_t *copyData1;
    uint16_t buffer1;

    // Ether frame
    getEtherMacAddress(localHwAddress);
    getIpAddress(localIpAddress);
    for (i = 0; i < HW_ADD_LENGTH; i++)
    {
        ether->destAddress[i] = s.remoteHwAddress[i];
        ether->sourceAddress[i] = localHwAddress[i];
    }
    ether->frameType = htons(TYPE_IP);

    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    ip->rev = 0x4;
    ip->size = 0x5;
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 128;
    ip->protocol = PROTOCOL_TCP;
    ip->headerChecksum = 0;
     for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        ip->destIp[i] = s.remoteIpAddress[i];
        ip->sourceIp[i] = localIpAddress[i];
    }
    uint8_t ipHeaderLength = ip->size * 4;
    // TCP header
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip + (ip->size * 4));


    //mqtt stuff
    mqttHeader* mqtt = (mqttHeader*)tcp->data;

//    uint8_t publish[10] = {0x00, 0x03, 0x41, 0x42, 0x43, 0x68, 0x65, 0x6C, 0x6C, 0x6E};
//    uint8_t subscribe[8] = {0x00, 0x01, 0x00, 0x03, 0x41, 0x42, 0x43, 0x00};
//    uint8_t connect[17] = {0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x02, 0x00, 0x3C, 0x00, 0x05, 0x50, 0x51, 0x52, 0x53, 0x54};

    copyData1 = mqtt->variableStuff;

    if(type == CONNECT)
    {
        mqtt->controlHeader = 0x10;
        mqtt->remainingLength = 0x11;

        for (i = 0; i < mqtt->remainingLength; i++)
            copyData1[i] = str[i];
    }
    else if(type == PUBLISH)
    {
        mqtt->controlHeader = 0x30;
        mqtt->remainingLength = 2 + topicLen;
        uint8_t temp = 0;

        copyData1[0] = 0;

        for(i = 2; i < topicLen + 3; i++)
        {
            if(str[i - 2] == '-')
            {
                copyData1[1] = i - 2;
                temp = 1;
            }
            else
                copyData1[i - temp] = str[i - 2];
        }
    }
    else if(type == SUBSCRIBE)
    {
        mqtt->controlHeader = 0x82;
        mqtt->remainingLength = 5 + topicLen;

        copyData1[0] = 0;
        copyData1[1] = 1;
        copyData1[2] = 0;
        copyData1[3] = topicLen;

        for (i = 0; i < topicLen; i++)
                copyData1[i + 4] = str[i];

        copyData1[i + 4] = 0;
    }
    else if(type == UNSUBSCRIBE)
    {
        mqtt->controlHeader = 0xA2;
        mqtt->remainingLength = 4 + topicLen;

        copyData1[0] = 0;
        copyData1[1] = 1;
        copyData1[2] = 0;
        copyData1[3] = topicLen;

        for (i = 0; i < topicLen; i++)
                copyData1[i + 4] = str[i];
    }
    else if(type == DISCONNECT)
    {
        mqtt->controlHeader = 0xE0;
        mqtt->remainingLength = 0;
    }


    tcp->sourcePort = htons(s.localPort);
    tcp->destPort = htons(s.remotePort);
    // adjust lengths
    tcpLength = sizeof(tcpHeader) + mqtt->remainingLength + 2;
    ip->length = htons(ipHeaderLength + tcpLength);
    // 32-bit sum over ip header
    calcIpChecksum(ip);

    //set the offset fields
    offsetFieldNum = ((0x5) << OFS_SHIFT) | PSH | ACK;
    tcp->offsetFields = htons(offsetFieldNum);

    //set window size
    tcp->windowSize = htons(1522);

    tcp->sequenceNumber = htonl(s.sequenceNumber);
    tcp->acknowledgementNumber = htonl(s.acknowledgementNumber);

    sum = 0;
    sumIpWords(ip->sourceIp, 8, &sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    buffer1 = htons(tcpLength);
    sumIpWords(&buffer1, 2, &sum);
    // add tcp header
    tcp->checksum = 0;
    sumIpWords(tcp, tcpLength, &sum);
    tcp->checksum = getIpChecksum(sum);

    // send packet with size = ether + tcp hdr + ip header + tcp_size
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength);
}















