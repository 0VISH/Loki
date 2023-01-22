#pragma once

#include "xenon.hh"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if(XE_PLAT_WIN)
#include <windows.h>
#endif

#if(XE_DBG)
#include <typeinfo>
#endif
#include "mem.cc"
#include "ds.cc"
#include "error.cc"
#include "frontend/lexer.cc"
#include "frontend/parser.cc"
#include "frontend/type.cc"