#pragma once
#include <stdint.h>

#define repeat(n) for (byte _i##__COUNTER__ = 0; _i##__COUNTER__ < (n); ++_i##__COUNTER__)

template<typename T> using ptr = T*;
template<typename T> using constptr = const T*;

using byte = uint8_t;
using sbyte = int8_t;
using word = uint16_t;
using sword = int16_t;
