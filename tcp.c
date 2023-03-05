// TCP Library (framework only)
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
#include "tcp.h"
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

// Determines whether packet is TCP packet
// Must be an IP packet
bool isTcp(etherHeader* ether)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader* tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    uint32_t sum = 0;
    bool ok;
    uint16_t tmp16;
    ok = (ip->protocol == PROTOCOL_TCP);
    if (ok)
    {
        // 32-bit sum over pseudo-header
        sumIpWords(ip->sourceIp, 8, &sum);
        tmp16 = ip->protocol;
        sum += (tmp16 & 0xff) << 8;
        tmp16 = htons(ntohs(ip->length) - (ip->size * 4));
        sumIpWords(&tmp16, 2, &sum);
        // add tcp header and data
        sumIpWords(tcp, ntohs(ip->length) - (ip->size * 4), &sum);
        ok = (getIpChecksum(sum) == 0);
    }
    return ok;
}

// TODO: Add functions here
void getTcpMessageSocket(etherHeader *ether, socket *s)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    uint8_t i;
    for (i = 0; i < HW_ADD_LENGTH; i++)
        s->remoteHwAddress[i] = ether->sourceAddress[i];
    for (i = 0; i < IP_ADD_LENGTH; i++)
        s->remoteIpAddress[i] = ip->sourceIp[i];
    s->remotePort = ntohs(tcp->sourcePort);
    s->localPort = ntohs(tcp->destPort);
}

// get sequence number and acknowledgment number 
void processTcp(etherHeader *ether, socket *s)
{
    ipHeader *ip = (ipHeader*)ether->data;
    uint8_t ipHeaderLength = ip->size * 4;
    tcpHeader *tcp = (tcpHeader*)((uint8_t*)ip + ipHeaderLength);
    s->sequenceNumber = ntohs(tcp->sequenceNumber + 1);
    s->acknowledgementNumber = ntohs(tcp->acknowledgementNumber + 1);
}

void sendTcpMessage(etherHeader *ether, socket s, uint16_t flags, uint8_t data[], uint16_t dataSize)
{
    uint8_t i;
    uint32_t sum;
    uint16_t tmp16;
    uint16_t tcpLength;
    uint16_t offsetFieldNum = 0;
    uint8_t *copyData;
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
    tcpLength = sizeof(tcpHeader) + dataSize;
    ip->length = htons(ipHeaderLength + tcpLength);
    // 32-bit sum over ip header
    calcIpChecksum(ip);
    // copy data
    copyData = tcp->data;
    for (i = 0; i < dataSize; i++)
        copyData[i] = data[i];

    //set the offset fields
    offsetFieldNum = ((tcpLength / 4) << OFS_SHIFT) | flags; //set the flags and assume data offset is 6 bytes (no options or padding)
    tcp->offsetFields = htons(offsetFieldNum);

    //set window size
    tcp->windowSize = htons(1522);

//    if(flags & 0x0002)
//    {
//        tcp->sequenceNumber = htons(0);
//        tcp->acknowledgementNumber = htons(0);
//    }
//    else
//    {
        tcp->sequenceNumber = htons(s.sequenceNumber);
        tcp->acknowledgementNumber = htons(s.acknowledgementNumber);
    //}

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

//window size is 1500 bytes, or 1 frame
//data offset is the number of bytes until data
//data offset and flags are in offset fields
//sequence number is random but should set to 0 for debugging when initial tcp message is sent
//first tcp message has the syn flag set without the ack flag set, set the sequence number in the socket at this time
//increment sequence number by n every time a message is sent, where n is the number of bytes in the message
//ack number is next expected sequence number


