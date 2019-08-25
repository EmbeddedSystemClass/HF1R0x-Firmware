#include "hexabitz/BOSMessageBuilder.h"


static bool cliFlag_ = false;
static bool messFlag_ = false;
static bool traceFlag_ = false;



void hstd::setCLIRespDefault(bool enable)
{
	if (enable)
		setMessRespDefault(true);
	
	cliFlag_ = enable;
}

void hstd::setMessRespDefault(bool enable)
{
	messFlag_ = enable;
}

void hstd::setTraceDefault(bool enable)
{
	traceFlag_ = enable;
}

hstd::Message hstd::make_message(Addr_t dest, uint16_t code)
{
	return make_message(dest, Addr_t(), code);
}

hstd::Message hstd::make_message(Addr_t dest, Addr_t src, uint16_t code)
{
	hstd::Message message;
	
	message.setSource(src);
	message.setDest(dest);
	message.setCode(code);

	message.setCLIOnlyFlag(cliFlag_);
	message.setMessOnlyFlag(messFlag_);
	message.setTraceFlag(traceFlag_);

	return message;
}