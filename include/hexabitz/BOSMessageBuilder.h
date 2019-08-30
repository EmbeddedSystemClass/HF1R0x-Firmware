#ifndef BOS_MESSAGE_BUILDER_H
#define BOS_MESSAGE_BUILDER_H

#include "hexabitz/BOS.h"
#include "hexabitz/BOSMessage.h"

#include <stdint.h>
#include <stdbool.h>

namespace hstd {

void setCLIRespDefault(bool enable);
void setMessRespDefault(bool enable);
void setTraceDefault(bool enable);

Message make_message(Addr_t dest, uint16_t code);
Message make_message(Addr_t dest, Addr_t src, uint16_t code);
}


#endif /* BOS_MESSAGE_BUILDER_H */