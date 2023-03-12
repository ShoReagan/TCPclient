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

typedef struct _mqttHeader // 20 or more bytes
{
  uint8_t controlHeader;
  uint8_t remainingLength;
  uint8_t*  variableHeader;
  uint8_t* payload;
} mqttHeader;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void sendMqttMessage(etherHeader *ether, socket s, uint8_t type);

#endif

