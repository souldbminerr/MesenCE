#pragma once

#define MemoryType LibnxMemoryType
#define DebugEventType LibnxDebugEventType
#include <switch.h>
#undef MemoryType
#undef DebugEventType

#include <cstdint>

class SwitchSaltyNX
{
public:
    static bool Init();
    static void Shutdown();
    static void ReportFrame(uint32_t width, uint32_t height, uint64_t systemTick);
};