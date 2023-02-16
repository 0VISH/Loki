#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "xenon.hh"

#if(XE_PLAT_WIN)
#include <windows.h>
#endif

#if(XE_DBG)
#include <typeinfo>
#endif

#include "debug/timer.cc"
#include "debug/exception.cc"
#include "mem.cc"
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