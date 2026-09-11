#pragma once

void ColorPrintf(const char* format, ...);

#define printf(...) ColorPrintf(__VA_ARGS__)