#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mutex>

#if(SIMD)
#include <immintrin.h>
#endif

#include "mem.cc"
#include "ds.cc"
#include "time.cc"
#if(WIN)
#include "windowsOS.cc"
#elif(LIN)
#include "linuxOS.cc"
#endif

#include "debug/include.hh"

#include "report.cc"
#include "frontend/include.hh"
#include "midend/include.hh"
#include "backend/include.hh"
#include "loki.cc"
