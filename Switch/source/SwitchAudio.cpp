#include "SwitchAudio.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/Audio/SoundMixer.h"
#include <cstdlib>
#include <cstring>
#include "SwitchLog.h"

SwitchAudio::SwitchAudio(Emulator* emu) : _emu(emu)
{
	for(uint32_t i = 0; i < BufferCount; i++) {
		void* mem = aligned_alloc(0x1000, SamplesPerBuffer * 2 * sizeof(int16_t));
		if(mem) {
			memset(mem, 0, SamplesPerBuffer * 2 * sizeof(int16_t));
			_buffers[i].buffer = mem;
			_buffers[i].buffer_size = SamplesPerBuffer * 2 * sizeof(int16_t);
			_buffers[i].data_size = 0;
			_buffers[i].data_offset = 0;
		}
		_bufferInFlight[i] = false;
	}

	_staging.reserve(SamplesPerBuffer * BufferCount * 2);

	if(R_SUCCEEDED(audoutInitialize())) {
		if(R_SUCCEEDED(audoutStartAudioOut())) {
			_started = true;
		}
	}

	_emu->GetSoundMixer()->RegisterAudioDevice(this);
}

SwitchAudio::~SwitchAudio()
{
	auto lock = _lock.AcquireSafe();
	FlushLocked();

	if(_started) {
		audoutStopAudioOut();
	}

	audoutExit();
	for(uint32_t i = 0; i < BufferCount; i++) {
		free(_buffers[i].buffer);
		_buffers[i].buffer = nullptr;
	}
}

bool SwitchAudio::EnsureStarted()
{
	if(!_started) {
		if(R_SUCCEEDED(audoutStartAudioOut())) {
			_started = true;
		}
	}
	return _started;
}

void SwitchAudio::ReclaimReleased()
{
	AudioOutBuffer* released = nullptr;
	u32 releasedCount = 0;
	audoutWaitPlayFinish(&released, &releasedCount, 0);
	for(u32 i = 0; i < releasedCount; i++) {
		for(uint32_t b = 0; b < BufferCount; b++) {
			if(&_buffers[b] == &released[i]) {
				_bufferInFlight[b] = false;
				break;
			}
		}
	}
}

int SwitchAudio::FindFreeBuffer()
{
	for(uint32_t i = 0; i < BufferCount; i++) {
		uint32_t b = (_activeBuffer + i) % BufferCount;
		if(!_bufferInFlight[b] && _buffers[b].buffer != nullptr) {
			return (int)b;
		}
	}
	return -1;
}

void SwitchAudio::Pump()
{
	if(!EnsureStarted()) {
		return;
	}
	
	ReclaimReleased();

	while(_staging.size() >= SamplesPerBuffer * 2) {
		int free = FindFreeBuffer();
		if(free < 0) {
			break;
		}
		int16_t* dst = (int16_t*)_buffers[free].buffer;
		memcpy(dst, _staging.data(), SamplesPerBuffer * 2 * sizeof(int16_t));
		_staging.erase(_staging.begin(), _staging.begin() + SamplesPerBuffer * 2);
		_buffers[free].data_size = SamplesPerBuffer * 2 * sizeof(int16_t);
		_buffers[free].data_offset = 0;
		if(R_SUCCEEDED(audoutAppendAudioOutBuffer(&_buffers[free]))) {
			_bufferInFlight[free] = true;
			_activeBuffer = (free + 1) % BufferCount;
		} else {
			break;
		}
	}
}

void SwitchAudio::FlushLocked()
{
	_staging.clear();
	u32 count = 0;
	if(_started) {
		bool flushed = false;
		audoutFlushAudioOutBuffers(&flushed);
		AudioOutBuffer* released = nullptr;
		audoutGetReleasedAudioOutBuffer(&released, &count);
	}
	for(uint32_t i = 0; i < BufferCount; i++) {
		_bufferInFlight[i] = false;
	}
}

void SwitchAudio::PlayBuffer(int16_t* soundBuffer, uint32_t sampleCount, uint32_t sampleRate, bool isStereo)
{
	(void)0;
	if(soundBuffer == nullptr || sampleCount == 0) {
		return;
	}
	auto lock = _lock.AcquireSafe();

	_sampleRate = sampleRate;
	_isStereo = isStereo;

	if(isStereo) {
		_staging.insert(_staging.end(), soundBuffer, soundBuffer + sampleCount * 2);
	} else {
		size_t base = _staging.size();
		_staging.resize(base + sampleCount * 2);
		for(uint32_t i = 0; i < sampleCount; i++) {
			_staging[base + i * 2] = soundBuffer[i];
			_staging[base + i * 2 + 1] = soundBuffer[i];
		}
	}

	const size_t maxStaged = 48000 / 4 * 2;
	if(_staging.size() > maxStaged) {
		_staging.erase(_staging.begin(), _staging.begin() + (_staging.size() - maxStaged));
		_underrunCount++;
	}

	Pump();
}

void SwitchAudio::Stop()
{
	auto lock = _lock.AcquireSafe();
	FlushLocked();
	_averageLatency = 0;
}

void SwitchAudio::Pause()
{
	auto lock = _lock.AcquireSafe();
	FlushLocked();
	if(_started) {
		audoutStopAudioOut();
		_started = false;
	}
}

void SwitchAudio::ProcessEndOfFrame()
{
	auto lock = _lock.AcquireSafe();
	Pump();
}

string SwitchAudio::GetAvailableDevices()
{
	return "audout";
}

void SwitchAudio::SetAudioDevice(string deviceName)
{
	(void)deviceName;
}

AudioStatistics SwitchAudio::GetStatistics()
{
	AudioStatistics stats;
	stats.AverageLatency = _averageLatency;
	stats.BufferUnderrunEventCount = _underrunCount;
	stats.BufferSize = SamplesPerBuffer * BufferCount * 2 * (uint32_t)sizeof(int16_t);
	return stats;
}