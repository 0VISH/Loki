#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <immintrin.h>

#include "xenon.hh"

#include "mem.cc"

#if(XE_PLAT_WIN)
#include "windowsOS.cc"
#elif(XE_PLAT_LIN)
#include "linuxOS.cc"
#endif

#if(XE_DBG)
#include <typeinfo>
#endif

#include "debug/timer.cc"
#include "debug/exception.cc"
#include "ds.cc"
#include "report.cc"
#include "frontend/lexer.cc"
#include "frontend/type.hh"
#include "frontend/parser.cc"
#include "frontend/type.cc"
#include "frontend/entity.cc"
#include "xenon.cc"

#if(XE_DBG)
#include "debug/tester.cc"
#endif
