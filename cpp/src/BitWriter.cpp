//
// Created by yitzk on 9/17/2026.
//

#include "BitWriter.h"


BitWriter::BitWriter(uint8_t* buffer):buffer(buffer), iter(buffer), offset(0)
{
}

