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

// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void sendMqttMessage(etherHeader *ether, socket s, uint8_t type)
{
    uint8_t i;
    uint32_t sum;
    uint16_t tmp16;
    uint16_t tcpLength;
    uint16_t offsetFieldNum = 0;
    uint8_t localHwAddress[6];
    uint8_t localIpAddress[4];

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

    tcp->sourcePort = htons(s.localPort);
    tcp->destPort = htons(s.remotePort);
    // adjust lengths
    tcpLength = sizeof(tcpHeader);
    ip->length = htons(ipHeaderLength + tcpLength);
    // 32-bit sum over ip header
    calcIpChecksum(ip);

    //set the offset fields
    offsetFieldNum = ((tcpLength / 4) << OFS_SHIFT) | PSH | ACK; //set the flags and assume data offset is 6 bytes (no options or padding)
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

    //mqtt stuff
    mqttHeader* mqtt = (mqttHeader*)((uint8_t*)tcp + (tcpLength * 4));
    uint8_t tempHeader[10] = {0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x02, 0x00, 0x3C};
    uint8_t tempPayload[7] = {0x00, 0x05, 0x50, 0x4D, 0x52, 0x53, 0x54};
    mqtt->controlHeader = 0x10;
    mqtt->remainingLength = 0x11;
    mqtt->controlHeader = tempHeader;
    mqtt->payload = tempPayload;

    // send packet with size = ether + tcp hdr + ip header + tcp_size
    putEtherPacket(ether, sizeof(etherHeader) + ipHeaderLength + tcpLength + (mqtt->remainingLength + 2));
}















