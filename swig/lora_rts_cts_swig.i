/* -*- c++ -*- */

#define LORA_RTS_CTS_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "lora_rts_cts_swig_doc.i"

%{
#include "lora_rts_cts/RTSSender.h"
#include "lora_rts_cts/CTSSender.h"
#include "lora_rts_cts/cadDetect.h"
#include "lora_rts_cts/RTSSendToken.h"
#include "lora_rts_cts/RTSReceiveToken.h"
#include "lora_rts_cts/CTSReceiveToken.h"
#include "lora_rts_cts/CTSSendToken.h"
%}

%include "lora_rts_cts/RTSSender.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, RTSSender);
%include "lora_rts_cts/CTSSender.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, CTSSender);
%include "lora_rts_cts/cadDetect.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, cadDetect);
%include "lora_rts_cts/RTSSendToken.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, RTSSendToken);
%include "lora_rts_cts/RTSReceiveToken.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, RTSReceiveToken);
%include "lora_rts_cts/CTSReceiveToken.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, CTSReceiveToken);
%include "lora_rts_cts/CTSSendToken.h"
GR_SWIG_BLOCK_MAGIC2(lora_rts_cts, CTSSendToken);
