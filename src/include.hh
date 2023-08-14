#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mutex>

#if(SIMD)
#include <immintrin.h>
#endif

#include "map.cc"
#include "basic.hh"
#include "mem.cc"
#include "ds.cc"
#include "threadpool.hh"
#include "time.cc"
#if(PLAT_WIN)
#include "windowsOS.cc"
#elif(PLAT_LIN)
#include "linuxOS.cc"
#endif

#include "debug/include.hh"

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
