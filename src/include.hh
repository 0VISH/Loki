#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <immintrin.h>
#include <mutex>

#include "basic.hh"
#include "mem.cc"
#include "ds.cc"
#include "threadpool.hh"

#if(PLAT_WIN)
#include "windowsOS.cc"
#elif(PLAT_LIN)
#include "linuxOS.cc"
#endif

#if(DBG)
#include <typeinfo>
#endif

#if(DBG)
#include "debug/timer.cc"
#include "debug/exception.cc"
#endif
#include "report.cc"
#include "threadpool.cc"
#include "frontend/lexer.cc"
#include "frontend/type.hh"
#include "frontend/parser.cc"
#include "frontend/type.cc"
#include "frontend/entity.cc"
#include "midend/bytecode.cc"
#include "midend/vm.cc"
#include "loki.cc"

#if(DBG)
#include "debug/tester.cc"
#endif
