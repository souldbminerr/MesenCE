#pragma once

#define MemoryType LibnxMemoryType
#define DebugEventType LibnxDebugEventType
#include <switch.h>
#undef MemoryType
#undef DebugEventType

#include "Shared/Interfaces/IKeyManager.h"

enum SwitchPadKey : uint16_t
{
	SwNone = 0,
	SwA = 1,
	SwB = 2,
	SwX = 3,
	SwY = 4,
	SwL = 5,
	SwR = 6,
	SwZL = 7,
	SwZR = 8,
	SwPlus = 9,
	SwMinus = 10,
	SwUp = 11,
	SwDown = 12,
	SwLeft = 13,
	SwRight = 14,
	SwStickL = 15,
	SwStickR = 16,
};

class SwitchInput : public IKeyManager
{
private:
	PadState _pad = {};
	u64 _buttons = 0;
	HidAnalogStickState _leftStick = {};
	HidAnalogStickState _rightStick = {};
	bool _disabled = false;

	static constexpr int StickDeadzone = 8000;

	bool IsDirectionHeld(SwitchPadKey key);

public:
	SwitchInput();
	~SwitchInput() override = default;

	void RefreshState() override;
	void UpdateDevices() override;
	bool IsMouseButtonPressed(MouseButton button) override;
	bool IsKeyPressed(uint16_t keyCode) override;
	vector<uint16_t> GetPressedKeys() override;
	string GetKeyName(uint16_t keyCode) override;
	uint16_t GetKeyCode(string keyName) override;

	bool SetKeyState(uint16_t scanCode, bool state) override;
	void ResetKeyState() override;
	void SetDisabled(bool disabled) override;

	u64 GetButtonsDown();
};
