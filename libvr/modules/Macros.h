//
//  Macros.h
//  kof
//
//  Created by shanqiang on 15/2/26.
//  Copyright (c) 2015年 shanqiang. All rights reserved.
//

#ifndef kof_Macros_h
#define kof_Macros_h

#include <stdarg.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace std;

typedef unsigned int len_type;

static const int MAX_LOG_LENGTH = 16*1024;

static std::string log_(const char *format, ...)
{
    char buf[MAX_LOG_LENGTH];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_LOG_LENGTH-3, format, args);
    strcat(buf, "\n");
    va_end(args);
    
    return buf;
}

#define THROW_EXP(format, ...) {   \
ostringstream ostr; \
ostr << __DATE__" " << __TIME__ << " " << __FILE__" " << __FUNCTION__ << " " << __LINE__ << "line " << (log_(format, ##__VA_ARGS__));   \
throw std::invalid_argument(ostr.str());    \
}

#endif
