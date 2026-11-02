#pragma once

// See SwitchInput.h: libnx's `MemoryType`/`DebugEventType` collide with
// Mesen's enums; rename the libnx ones for this TU.
#define MemoryType LibnxMemoryType
#define DebugEventType LibnxDebugEventType
#include <switch.h>
#undef MemoryType
#undef DebugEventType

#include <deko3d.h>
#include "Shared/Interfaces/IRenderingDevice.h"
#include "Shared/RenderedFrame.h"
#include "Utilities/SimpleLock.h"

class Emulator;

class SwitchRenderer : public IRenderingDevice
{
public:
	struct Vertex
	{
		float x, y; // NDC position
		float u, v; // texture coordinates
	};

private:
	static constexpr uint32_t FbWidth = 1280;
	static constexpr uint32_t FbHeight = 720;
	static constexpr uint32_t FbCount = 2;
	static constexpr uint32_t CodeMemSize = 64 * 1024;
	static constexpr uint32_t CmdMemSize = 16 * 1024;

	static constexpr uint32_t TexSize = 1024;

	Emulator* _emu = nullptr;

	// Emulator frame
	SimpleLock _frameLock;
	RenderedFrame _pendingFrame;
	uint32_t _frameWidth = 0;
	uint32_t _frameHeight = 0;
	bool _hasFrame = false;

	// deko3d objects
	DkDevice _device = nullptr;
	DkQueue _queue = nullptr;
	DkSwapchain _swapchain = nullptr;
	DkImage _swapImages[FbCount] = {};
	DkMemBlock _swapMemBlock = nullptr;

	DkImage _uploadImage = {};
	DkMemBlock _uploadMemBlock = nullptr;

	// Linear staging buffer for the ARGB->RGBA swizzle
	DkMemBlock _stageMemBlock = nullptr;
	uint8_t* _stageCpuAddr = nullptr;
	DkGpuAddr _stageGpuAddr = 0;

	DkMemBlock _descMemBlock = nullptr;
	DkGpuAddr _descGpuAddr = 0;

	DkMemBlock _vtxMemBlock = nullptr;
	Vertex* _vtxCpuAddr = nullptr;
	DkGpuAddr _vtxGpuAddr = 0;

	DkMemBlock _codeMemBlock = nullptr;
	uint32_t _codeMemOffset = 0;
	DkShader _vertexShader = {};
	DkShader _fragmentShader = {};

	DkMemBlock _cmdMemBlock = nullptr;
	DkCmdBuf _cmdbuf = nullptr;

	DkSampler _sampler = {};

	bool _initialized = false;

	void SetFrameSize(uint32_t width, uint32_t height);
	void PresentFrame(const uint32_t* argb, uint32_t width, uint32_t height);
	bool LoadShader(DkShader* shader, const char* path);

public:
	explicit SwitchRenderer(Emulator* emu);
	~SwitchRenderer() override;

	bool IsInitialized() { return _initialized; }

	void UpdateFrame(RenderedFrame& frame) override;
	void ClearFrame() override;
	void Render(RenderSurfaceInfo& emuHud, RenderSurfaceInfo& scriptHud) override;
	void Reset() override;
	void SetFullscreenMode(FullscreenSettings settings) override;
};
