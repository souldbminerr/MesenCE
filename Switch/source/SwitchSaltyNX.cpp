#include "SwitchSaltyNX.h"
#include <cstring>

static const uint32_t NX_FPS_MAGIC = 0x465053;

struct NxFpsSharedBlock {
    uint32_t MAGIC;
    uint8_t  FPS;
    float    FPSavg;
    bool     pluginActive;
    uint8_t  FPSlocked, FPSmode, ZeroSync, patchApplied;
    uint8_t  API;
    uint32_t FPSticks[10];
    uint8_t  Buffers, SetBuffers, ActiveBuffers, SetActiveBuffers;
    uint8_t  displaySync;
    struct { uint16_t width, height, calls; } renderCalls[8];
    struct { uint16_t width, height, calls; } viewportCalls[8];
    bool     forceOriginalRefreshRate, dontForce60InDocked, forceSuspend;
    uint8_t  currentRefreshRate;
    float    readSpeedPerSecond;
    uint8_t  FPSlockedDocked;
    uint64_t frameNumber;
    int8_t   expectedSetBuffers;
} __attribute__((packed));

static_assert(sizeof(NxFpsSharedBlock) == 174, "NxFpsSharedBlock size");

static SharedMemory g_shmem = {};
static NxFpsSharedBlock* g_block = nullptr;
static uint64_t g_lastTick = 0;
static uint8_t g_tickIndex = 0;
static uint64_t g_frameCount = 0;

bool SwitchSaltyNX::Init()
{
    Handle h = 0;
    Result rc = svcConnectToNamedPort(&h, "SaltySD");
    if (R_FAILED(rc) || h == 0) {
        return false;
    }

    Service srv;
    serviceCreate(&srv, h);

    // Command 7: get shared memory handle
    Handle memHandle = 0;
    rc = serviceDispatch(&srv, 7, .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &memHandle);

    if (R_FAILED(rc) || memHandle == 0) {
        serviceClose(&srv);
        return false;
    }

    shmemLoadRemote(&g_shmem, memHandle, 0x1000, Perm_Rw);
    rc = shmemMap(&g_shmem);
    if (R_FAILED(rc)) {
        shmemClose(&g_shmem);
        serviceClose(&srv);
        return false;
    }

    // Command 0: close session
    uint32_t dummy = 0;
    serviceDispatchIn(&srv, 0, dummy, .in_send_pid = true);
    serviceClose(&srv);

    // Find existing block
    uint8_t* page = (uint8_t*)shmemGetAddr(&g_shmem);
    for (size_t off = 0; off + sizeof(NxFpsSharedBlock) <= 0x1000; off += 4) {
        if (*(uint32_t*)(page + off) == NX_FPS_MAGIC) {
            g_block = (NxFpsSharedBlock*)(page + off);
            return true;
        }
    }

    // Reserve new block (command 6)
    rc = svcConnectToNamedPort(&h, "SaltySD");
    if (R_FAILED(rc)) {
        return false;
    }
    serviceCreate(&srv, h);

    uint32_t sizeIn = (uint32_t)sizeof(NxFpsSharedBlock);
    uint32_t offsetOut = 0;
    SfDispatchParams params2 = {};
    params2.in_send_pid = true;
    rc = serviceDispatchImpl(&srv, 6, &sizeIn, sizeof(sizeIn), &offsetOut, sizeof(offsetOut), params2);
    if (R_FAILED(rc)) {
        serviceClose(&srv);
        return false;
    }

    uint32_t dummy2 = 0;
    serviceDispatchIn(&srv, 0, dummy2, .in_send_pid = true);
    serviceClose(&srv);

    if (offsetOut + sizeof(NxFpsSharedBlock) > 0x1000) {
        return false;
    }

    g_block = (NxFpsSharedBlock*)(page + offsetOut);
    memset(g_block, 0, sizeof(NxFpsSharedBlock));
    g_block->MAGIC = NX_FPS_MAGIC;
    g_block->pluginActive = true;
    g_block->API = 1;

    return true;
}

void SwitchSaltyNX::Shutdown()
{
    if (g_block) g_block = nullptr;
    if (g_shmem.handle) {
        shmemUnmap(&g_shmem);
        shmemClose(&g_shmem);
    }
}

void SwitchSaltyNX::ReportFrame(uint32_t width, uint32_t height, uint64_t systemTick)
{
    if (!g_block) return;

    g_frameCount++;

    if (g_lastTick != 0) {
        uint32_t delta = (uint32_t)(systemTick - g_lastTick);
        g_block->FPSticks[g_tickIndex] = delta;
        g_tickIndex = (g_tickIndex + 1) % 10;
    }
    g_lastTick = systemTick;

    uint64_t sum = 0;
    int valid = 0;
    for (int i = 0; i < 10; i++) {
        if (g_block->FPSticks[i] > 0) {
            sum += g_block->FPSticks[i];
            valid++;
        }
    }
    if (valid > 0) {
        float avg = (float)sum / valid;
        g_block->FPSavg = 19200000.0f / avg;
        g_block->FPS = (uint8_t)g_block->FPSavg;
    }

    g_block->renderCalls[0].width = width;
    g_block->renderCalls[0].height = height;
    g_block->renderCalls[0].calls++;

    g_block->pluginActive = true;
    g_block->frameNumber = g_frameCount;
    g_block->API = 1;
}