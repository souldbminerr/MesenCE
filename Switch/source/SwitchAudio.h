#pragma once

#define MemoryType LibnxMemoryType
#define DebugEventType LibnxDebugEventType
#include <switch.h>
#undef MemoryType
#undef DebugEventType

#include "Shared/Interfaces/IAudioDevice.h"
#include "Utilities/SimpleLock.h"
#include <vector>

class Emulator;

class SwitchAudio : public IAudioDevice
{
private:
	static constexpr uint32_t SamplesPerBuffer = 2048;
	static constexpr uint32_t BufferCount = 8;

	Emulator* _emu = nullptr;
	bool _started = false;
	bool _muted = false;

	AudioOutBuffer _buffers[BufferCount] = {};
	bool _bufferInFlight[BufferCount] = {};
	uint32_t _activeBuffer = 0;

	vector<int16_t> _staging;
	SimpleLock _lock;

	bool EnsureStarted();
	void ReclaimReleased();
	void Pump();
	void FlushLocked();
	int FindFreeBuffer();

public:
	explicit SwitchAudio(Emulator* emu);
	~SwitchAudio() override;

	void PlayBuffer(int16_t* soundBuffer, uint32_t sampleCount, uint32_t sampleRate, bool isStereo) override;
	void Stop() override;
	void Pause() override;
	void ProcessEndOfFrame() override;

	string GetAvailableDevices() override;
	void SetAudioDevice(string deviceName) override;

	AudioStatistics GetStatistics() override;

private:
	uint32_t _sampleRate = 48000;
	bool _isStereo = true;
	double _averageLatency = 0;
	uint32_t _underrunCount = 0;
};