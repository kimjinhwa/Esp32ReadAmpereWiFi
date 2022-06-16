#pragma once
#ifndef MYESPTELNET_H
#define MYESPTELNET_H
#include "esptelnet.h"

class myESPTelnet : public ESPTelnet
{
public:
    char getchar;
    uint8_t receivedLength;
    myESPTelnet()
    {
        getchar = 0x00;
        receivedLength = 0;
    };
    int printf(const char *format, ...)
    {
        char buf[256];
        va_list ap;
        va_start(ap, format);
        vsnprintf(buf, 256, format, ap);
        if (client && isClientConnected(client))
            client.print(String(buf));
        va_end(ap);
        return 0;
    };
};
#endif