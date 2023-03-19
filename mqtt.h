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

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include <stdbool.h>
#include "tcp.h"

#define CONNECT 1
#define PUBLISH 2
#define SUBSCRIBE 3
#define UNSUBSCRIBE 4
#define DISCONNECT 5

typedef struct _mqttHeader // 20 or more bytes
{
  uint8_t controlHeader;
  uint8_t remainingLength;
  uint8_t variableStuff[0];
} mqttHeader;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void sendMqttMessage(etherHeader *ether, socket s, uint8_t str[], uint8_t topicLen, uint8_t type);
void processMqtt(etherHeader *ether, socket *s, uint8_t *buffer);

#endif

