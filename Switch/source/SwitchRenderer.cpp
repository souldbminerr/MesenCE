#include "SwitchRenderer.h"
#include "SwitchSaltyNX.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/RenderedFrame.h"
#include "Shared/Video/VideoRenderer.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SwitchLog.h"

namespace
{
const DkVtxAttribState g_attribState[] = {
	{ .bufferId = 0, .isFixed = 0, .offset = 0, .size = DkVtxAttribSize_2x32, .type = DkVtxAttribType_Float, .isBgra = 0 },
	{ .bufferId = 0, .isFixed = 0, .offset = 8, .size = DkVtxAttribSize_2x32, .type = DkVtxAttribType_Float, .isBgra = 0 },
};

const DkVtxBufferState g_vtxbufState[] = {
	{ .stride = sizeof(SwitchRenderer::Vertex), .divisor = 0 },
};
} // namespace

bool SwitchRenderer::LoadShader(DkShader* shader, const char* path)
{
	FILE* f = fopen(path, "rb");
	if(!f) {
		return false;
	}
	fseek(f, 0, SEEK_END);
	uint32_t size = (uint32_t)ftell(f);
	rewind(f);

	uint32_t codeOffset = _codeMemOffset;
	_codeMemOffset += (size + DK_SHADER_CODE_ALIGNMENT - 1) & ~(DK_SHADER_CODE_ALIGNMENT - 1);

	fread((uint8_t*)dkMemBlockGetCpuAddr(_codeMemBlock) + codeOffset, size, 1, f);
	fclose(f);

	DkShaderMaker shaderMaker;
	dkShaderMakerDefaults(&shaderMaker, _codeMemBlock, codeOffset);
	dkShaderInitialize(shader, &shaderMaker);
	return true;
}

SwitchRenderer::SwitchRenderer(Emulator* emu) : _emu(emu)
{
	// --- Device ---
	DkDeviceMaker deviceMaker;
	dkDeviceMakerDefaults(&deviceMaker);
	_device = dkDeviceCreate(&deviceMaker);
	if(!_device) {
		return;
	}

	DkMemBlockMaker memMaker;

	// --- Swapchain images ---
	DkImageLayoutMaker fbLayoutMaker;
	dkImageLayoutMakerDefaults(&fbLayoutMaker, _device);
	fbLayoutMaker.flags = DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression;
	fbLayoutMaker.format = DkImageFormat_RGBA8_Unorm;
	fbLayoutMaker.dimensions[0] = FbWidth;
	fbLayoutMaker.dimensions[1] = FbHeight;

	DkImageLayout fbLayout;
	dkImageLayoutInitialize(&fbLayout, &fbLayoutMaker);

	uint64_t fbSize = dkImageLayoutGetSize(&fbLayout);
	uint32_t fbAlign = dkImageLayoutGetAlignment(&fbLayout);
	fbSize = (fbSize + fbAlign - 1) & ~(uint64_t)(fbAlign - 1);

	dkMemBlockMakerDefaults(&memMaker, _device, (uint32_t)(FbCount * fbSize));
	memMaker.flags = DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image;
	_swapMemBlock = dkMemBlockCreate(&memMaker);

	DkImage const* swapchainImages[FbCount] = {};

	for(uint32_t i = 0; i < FbCount; i++) {
		swapchainImages[i] = &_swapImages[i];
		dkImageInitialize(&_swapImages[i], &fbLayout, _swapMemBlock, (uint32_t)(i * fbSize));
	}
	DkSwapchainMaker swapMaker;
	dkSwapchainMakerDefaults(&swapMaker, _device, nwindowGetDefault(), swapchainImages, FbCount);
	_swapchain = dkSwapchainCreate(&swapMaker);

	DkImageLayoutMaker upLayoutMaker;
	dkImageLayoutMakerDefaults(&upLayoutMaker, _device);
	upLayoutMaker.format = DkImageFormat_RGBA8_Unorm;
	upLayoutMaker.dimensions[0] = TexSize;
	upLayoutMaker.dimensions[1] = TexSize;
	upLayoutMaker.dimensions[2] = 1;

	DkImageLayout upLayout;
	dkImageLayoutInitialize(&upLayout, &upLayoutMaker);

	uint64_t upSize = dkImageLayoutGetSize(&upLayout);
	uint32_t upAlign = dkImageLayoutGetAlignment(&upLayout);
	upSize = (upSize + upAlign - 1) & ~(uint64_t)(upAlign - 1);

	dkMemBlockMakerDefaults(&memMaker, _device, (uint32_t)upSize);
	memMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image;
	_uploadMemBlock = dkMemBlockCreate(&memMaker);
	dkImageInitialize(&_uploadImage, &upLayout, _uploadMemBlock, 0);

	dkMemBlockMakerDefaults(&memMaker, _device, TexSize * TexSize * 4);
	memMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
	_stageMemBlock = dkMemBlockCreate(&memMaker);
	_stageCpuAddr = (uint8_t*)dkMemBlockGetCpuAddr(_stageMemBlock);
	_stageGpuAddr = dkMemBlockGetGpuAddr(_stageMemBlock);

	dkMemBlockMakerDefaults(&memMaker, _device, 0x1000);
	memMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
	_descMemBlock = dkMemBlockCreate(&memMaker);
	_descGpuAddr = dkMemBlockGetGpuAddr(_descMemBlock);

	dkMemBlockMakerDefaults(&memMaker, _device, 0x1000);
	memMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
	_vtxMemBlock = dkMemBlockCreate(&memMaker);
	_vtxCpuAddr = (Vertex*)dkMemBlockGetCpuAddr(_vtxMemBlock);
	_vtxGpuAddr = dkMemBlockGetGpuAddr(_vtxMemBlock);

	dkMemBlockMakerDefaults(&memMaker, _device, CodeMemSize);
	memMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code;
	_codeMemBlock = dkMemBlockCreate(&memMaker);
	_codeMemOffset = 0;
	LoadShader(&_vertexShader, "romfs:/shaders/texture_vsh.dksh");
	LoadShader(&_fragmentShader, "romfs:/shaders/texture_fsh.dksh");

	dkMemBlockMakerDefaults(&memMaker, _device, CmdMemSize);
	memMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
	_cmdMemBlock = dkMemBlockCreate(&memMaker);

	DkCmdBufMaker cmdMaker;
	dkCmdBufMakerDefaults(&cmdMaker, _device);
	_cmdbuf = dkCmdBufCreate(&cmdMaker);

	dkCmdBufAddMemory(_cmdbuf, _cmdMemBlock, 0, CmdMemSize);

	DkQueueMaker queueMaker;
	dkQueueMakerDefaults(&queueMaker, _device);
	queueMaker.flags = DkQueueFlags_Graphics;
	_queue = dkQueueCreate(&queueMaker);

	dkSamplerDefaults(&_sampler);
	_sampler.minFilter = DkFilter_Nearest;
	_sampler.magFilter = DkFilter_Nearest;
	_sampler.wrapMode[0] = DkWrapMode_ClampToEdge;
	_sampler.wrapMode[1] = DkWrapMode_ClampToEdge;
	_sampler.wrapMode[2] = DkWrapMode_ClampToEdge;

	_initialized = true;

	uint32_t maxBufSize = (TexSize * TexSize * sizeof(uint32_t) + 0xFFF) & ~0xFFF;
	DkMemBlockMaker frameMemMaker;
	dkMemBlockMakerDefaults(&frameMemMaker, _device, maxBufSize);
	frameMemMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
	_frontMemBlock = dkMemBlockCreate(&frameMemMaker);
	_backMemBlock = dkMemBlockCreate(&frameMemMaker);
	if(_frontMemBlock && _backMemBlock) {
		_frontBuffer = (uint32_t*)dkMemBlockGetCpuAddr(_frontMemBlock);
		_backBuffer = (uint32_t*)dkMemBlockGetCpuAddr(_backMemBlock);
		_frontGpuAddr = dkMemBlockGetGpuAddr(_frontMemBlock);
		_backGpuAddr = dkMemBlockGetGpuAddr(_backMemBlock);
		if(!_frontGpuAddr || !_backGpuAddr) {
			SwitchLog("Bad GPU address\n");
		}
	} else {
		SwitchLog("Fallback to CPU\n");
		_frontBuffer = new uint32_t[TexSize * TexSize];
		_backBuffer = new uint32_t[TexSize * TexSize];
		_frontGpuAddr = 0;
		_backGpuAddr = 0;
	}

	_emu->GetVideoRenderer()->RegisterRenderingDevice(this);
}

SwitchRenderer::~SwitchRenderer()
{
	if(_queue) {
		dkQueueWaitIdle(_queue);
		dkQueueDestroy(_queue);
	}
	if(_cmdbuf) {
		dkCmdBufDestroy(_cmdbuf);
	}
	if(_cmdMemBlock) {
		dkMemBlockDestroy(_cmdMemBlock);
	}
	if(_codeMemBlock) {
		dkMemBlockDestroy(_codeMemBlock);
	}
	if(_vtxMemBlock) {
		dkMemBlockDestroy(_vtxMemBlock);
	}
	if(_descMemBlock) {
		dkMemBlockDestroy(_descMemBlock);
	}
	if(_uploadMemBlock) {
		dkMemBlockDestroy(_uploadMemBlock);
	}
	if(_stageMemBlock) {
		dkMemBlockDestroy(_stageMemBlock);
	}
	if(_swapchain) {
		dkSwapchainDestroy(_swapchain);
	}
	if(_swapMemBlock) {
		dkMemBlockDestroy(_swapMemBlock);
	}
	if(_device) {
		dkDeviceDestroy(_device);
	}
	if(_frontMemBlock) dkMemBlockDestroy(_frontMemBlock);
	if(_backMemBlock) dkMemBlockDestroy(_backMemBlock);
	delete[] _frontBuffer;
	delete[] _backBuffer;
}

void SwitchRenderer::SetFrameSize(uint32_t width, uint32_t height)
{
	if(width == _frameWidth && height == _frameHeight) {
		return;
	}
	_frameWidth = width;
	_frameHeight = height;
}

void SwitchRenderer::UpdateFrame(RenderedFrame& frame)
{
	if(!_initialized || frame.FrameBuffer == nullptr) {
		SwitchLog("Can't update frame due to renderer error\n");
		return;
	}
	SetFrameSize(frame.Width, frame.Height);
	memcpy(_backBuffer, frame.FrameBuffer, frame.Width * frame.Height * sizeof(uint32_t));
	_hasFrame = true;
}

void SwitchRenderer::ClearFrame()
{
	if(_frontBuffer && _backBuffer) {
		memset(_backBuffer, 0, _frameWidth * _frameHeight * sizeof(uint32_t));
		_hasFrame = true;
	}
}

void SwitchRenderer::PresentFrame(uint32_t* argb, uint32_t width, uint32_t height)
{
	if(width == 0 || height == 0 || width > TexSize || height > TexSize) {
		SwitchLog("Can't present invalid frame\n");
		return;
	}

	// Bulk copy ARGB bytes; fragment shader swizzles channels (see texture_fsh.glsl).
	for(uint32_t y = 0; y < height; y++) {
		memcpy(_stageCpuAddr + (height - 1 - y) * width * 4, argb + y * width, width * 4);
	}

	DkCopyBuf copySrc = { _stageGpuAddr, 0, 0 };

	// TODO: Make this work with 1080p FB in docked
	float scale = std::min((float)FbWidth / (float)width, (float)FbHeight / (float)height);
	float dstW = (float)width * scale;
	float dstH = (float)height * scale;
	float dstX = ((float)FbWidth - dstW) * 0.5f;
	float dstY = ((float)FbHeight - dstH) * 0.5f;

	float u = (float)width / (float)TexSize;
	float v = (float)height / (float)TexSize;
	_vtxCpuAddr[0] = { -1.0f, -1.0f, 0.0f, 0.0f };
	_vtxCpuAddr[1] = { 3.0f, -1.0f, 2.0f * u, 0.0f };
	_vtxCpuAddr[2] = { -1.0f, 3.0f, 0.0f, 2.0f * v };

	int slot = dkQueueAcquireImage(_queue, _swapchain);

	dkCmdBufClear(_cmdbuf);

	DkImageView uploadDestView;
	dkImageViewDefaults(&uploadDestView, &_uploadImage);
	DkImageRect copyDst = { 0, 0, 0, width, height, 1 };
	dkCmdBufCopyBufferToImage(_cmdbuf, &copySrc, &uploadDestView, &copyDst, 0);

	dkCmdBufBarrier(_cmdbuf, DkBarrier_Full, DkInvalidateFlags_Image | DkInvalidateFlags_Descriptors);

	struct
	{
		DkImageDescriptor image;
		DkSamplerDescriptor sampler;
	} descriptors;

	DkImageView uploadView;
	dkImageViewDefaults(&uploadView, &_uploadImage);
	dkImageDescriptorInitialize(&descriptors.image, &uploadView, false, false);
	dkSamplerDescriptorInitialize(&descriptors.sampler, &_sampler);
	dkCmdBufPushData(_cmdbuf, _descGpuAddr, &descriptors, sizeof(descriptors));
	dkCmdBufBindImageDescriptorSet(_cmdbuf, _descGpuAddr, 1);
	dkCmdBufBindSamplerDescriptorSet(_cmdbuf, _descGpuAddr + sizeof(DkImageDescriptor), 1);

	// Render target + letterboxed viewport.
	DkImageView targetView;
	dkImageViewDefaults(&targetView, &_swapImages[slot]);
	dkCmdBufBindRenderTarget(_cmdbuf, &targetView, NULL);

	DkViewport fullViewport = { 0.0f, 0.0f, (float)FbWidth, (float)FbHeight, 0.0f, 1.0f };
	DkScissor fullScissor = { 0, 0, FbWidth, FbHeight };
	dkCmdBufSetViewports(_cmdbuf, 0, &fullViewport, 1);
	dkCmdBufSetScissors(_cmdbuf, 0, &fullScissor, 1);

	dkCmdBufClearColorFloat(_cmdbuf, 0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 1.0f);

	DkViewport viewport = { dstX, dstY, dstW, dstH, 0.0f, 1.0f };
	int32_t scX = (int32_t)dstX;
	int32_t scY = (int32_t)dstY;
	uint32_t scW = (uint32_t)dstW;
	uint32_t scH = (uint32_t)dstH;
	DkScissor scissor = { scX, scY, scW, scH };
	dkCmdBufSetViewports(_cmdbuf, 0, &viewport, 1);
	dkCmdBufSetScissors(_cmdbuf, 0, &scissor, 1);

	DkShader const* shaders[] = { &_vertexShader, &_fragmentShader };
	DkRasterizerState rasterizerState;
	DkColorState colorState;
	DkColorWriteState colorWriteState;
	dkRasterizerStateDefaults(&rasterizerState);
	dkColorStateDefaults(&colorState);
	dkColorWriteStateDefaults(&colorWriteState);

	dkCmdBufBindShaders(_cmdbuf, DkStageFlag_GraphicsMask, shaders, sizeof(shaders) / sizeof(shaders[0]));
	dkCmdBufBindRasterizerState(_cmdbuf, &rasterizerState);
	dkCmdBufBindColorState(_cmdbuf, &colorState);
	dkCmdBufBindColorWriteState(_cmdbuf, &colorWriteState);
	dkCmdBufBindTexture(_cmdbuf, DkStage_Fragment, 0, dkMakeTextureHandle(0, 0));
	dkCmdBufBindVtxAttribState(_cmdbuf, g_attribState, sizeof(g_attribState) / sizeof(g_attribState[0]));
	dkCmdBufBindVtxBufferState(_cmdbuf, g_vtxbufState, sizeof(g_vtxbufState) / sizeof(g_vtxbufState[0]));
	dkCmdBufBindVtxBuffer(_cmdbuf, 0, _vtxGpuAddr, sizeof(Vertex) * 3);
	dkCmdBufDraw(_cmdbuf, DkPrimitive_Triangles, 3, 1, 0, 0);

	DkCmdList list = dkCmdBufFinishList(_cmdbuf);
	dkQueueSubmitCommands(_queue, list);
	dkQueuePresentImage(_queue, _swapchain, slot);
}

void SwitchRenderer::Render(RenderSurfaceInfo& emuHud, RenderSurfaceInfo& scriptHud)
{
	(void)emuHud;
	(void)scriptHud;
	if(!_initialized) {
		return;
	}

	uint32_t width = 0, height = 0;
	uint32_t* present = nullptr;

	{
		dkQueueWaitIdle(_queue);

		if(!_hasFrame) {
			int slot = dkQueueAcquireImage(_queue, _swapchain);
			dkCmdBufClear(_cmdbuf);
			DkImageView targetView;
			dkImageViewDefaults(&targetView, &_swapImages[slot]);
			dkCmdBufBindRenderTarget(_cmdbuf, &targetView, NULL);
			DkViewport fullViewport = { 0.0f, 0.0f, (float)FbWidth, (float)FbHeight, 0.0f, 1.0f };
			DkScissor fullScissor = { 0, 0, FbWidth, FbHeight };
			dkCmdBufSetViewports(_cmdbuf, 0, &fullViewport, 1);
			dkCmdBufSetScissors(_cmdbuf, 0, &fullScissor, 1);
			dkCmdBufClearColorFloat(_cmdbuf, 0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 1.0f);
			DkCmdList list = dkCmdBufFinishList(_cmdbuf);
			dkQueueSubmitCommands(_queue, list);
			dkQueuePresentImage(_queue, _swapchain, slot);
			return;
		}

		std::swap(_frontBuffer, _backBuffer);
		width = _frameWidth;
		height = _frameHeight;
		if(_frontBuffer && width > 0 && height > 0) {
			present = _frontBuffer;
		}
	}

	if(present) {
		PresentFrame(present, width, height);
	}
	SwitchSaltyNX::ReportFrame(width > 0 ? width : FbWidth, height > 0 ? height : FbHeight, armGetSystemTick());
}

void SwitchRenderer::Reset()
{
}

void SwitchRenderer::SetFullscreenMode(FullscreenSettings settings)
{
	(void)settings;
}
