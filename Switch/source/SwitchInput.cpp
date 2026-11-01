#include "SwitchInput.h"

/* Minimal implementation of Input */
SwitchInput::SwitchInput()
{
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	padInitializeDefault(&_pad);
	RefreshState();
}

bool SwitchInput::IsDirectionHeld(SwitchPadKey key)
{
	const int dz = StickDeadzone;
	switch(key) {
		case SwUp: return (_buttons & HidNpadButton_Up) || (_leftStick.y > dz) || (_rightStick.y > dz);
		case SwDown: return (_buttons & HidNpadButton_Down) || (_leftStick.y < -dz) || (_rightStick.y < -dz);
		case SwLeft: return (_buttons & HidNpadButton_Left) || (_leftStick.x < -dz) || (_rightStick.x < -dz);
		case SwRight: return (_buttons & HidNpadButton_Right) || (_leftStick.x > dz) || (_rightStick.x > dz);
		default: return false;
	}
}

void SwitchInput::RefreshState()
{
	padUpdate(&_pad);
	_buttons = padGetButtons(&_pad);
	_leftStick = padGetStickPos(&_pad, 0);
	_rightStick = padGetStickPos(&_pad, 1);
}

void SwitchInput::UpdateDevices()
{
	/* TODO: add support for showing controller popup when you connect more controllers */
}

bool SwitchInput::IsMouseButtonPressed(MouseButton button)
{
	(void)button;
	return false;
}

bool SwitchInput::IsKeyPressed(uint16_t keyCode)
{
	if(_disabled || keyCode == SwNone) {
		return false;
	}
	switch((SwitchPadKey)keyCode) {
		case SwA: return (_buttons & HidNpadButton_A) != 0;
		case SwB: return (_buttons & HidNpadButton_B) != 0;
		case SwX: return (_buttons & HidNpadButton_X) != 0;
		case SwY: return (_buttons & HidNpadButton_Y) != 0;
		case SwL: return (_buttons & HidNpadButton_L) != 0;
		case SwR: return (_buttons & HidNpadButton_R) != 0;
		case SwZL: return (_buttons & HidNpadButton_ZL) != 0;
		case SwZR: return (_buttons & HidNpadButton_ZR) != 0;
		case SwPlus: return (_buttons & HidNpadButton_Plus) != 0;
		case SwMinus: return (_buttons & HidNpadButton_Minus) != 0;
		case SwStickL: return (_buttons & HidNpadButton_StickL) != 0;
		case SwStickR: return (_buttons & HidNpadButton_StickR) != 0;
		case SwUp:
		case SwDown:
		case SwLeft:
		case SwRight: return IsDirectionHeld((SwitchPadKey)keyCode);
		default: return false;
	}
}

vector<uint16_t> SwitchInput::GetPressedKeys()
{
	vector<uint16_t> keys;
	for(uint16_t code = SwA; code <= SwStickR; code++) {
		if(IsKeyPressed(code)) {
			keys.push_back(code);
		}
	}
	return keys;
}

string SwitchInput::GetKeyName(uint16_t keyCode)
{
	switch((SwitchPadKey)keyCode) {
		case SwA: return "A";
		case SwB: return "B";
		case SwX: return "X";
		case SwY: return "Y";
		case SwL: return "L";
		case SwR: return "R";
		case SwZL: return "ZL";
		case SwZR: return "ZR";
		case SwPlus: return "Plus";
		case SwMinus: return "Minus";
		case SwUp: return "Up";
		case SwDown: return "Down";
		case SwLeft: return "Left";
		case SwRight: return "Right";
		case SwStickL: return "StickL";
		case SwStickR: return "StickR";
		default: return "";
	}
}

uint16_t SwitchInput::GetKeyCode(string keyName)
{
	if(keyName == "A") return SwA;
	if(keyName == "B") return SwB;
	if(keyName == "X") return SwX;
	if(keyName == "Y") return SwY;
	if(keyName == "L") return SwL;
	if(keyName == "R") return SwR;
	if(keyName == "ZL") return SwZL;
	if(keyName == "ZR") return SwZR;
	if(keyName == "Plus") return SwPlus;
	if(keyName == "Minus") return SwMinus;
	if(keyName == "Up") return SwUp;
	if(keyName == "Down") return SwDown;
	if(keyName == "Left") return SwLeft;
	if(keyName == "Right") return SwRight;
	if(keyName == "StickL") return SwStickL;
	if(keyName == "StickR") return SwStickR;
	return SwNone;
}

bool SwitchInput::SetKeyState(uint16_t scanCode, bool state)
{
	(void)scanCode;
	(void)state;
	return false;
}

void SwitchInput::ResetKeyState()
{
	_buttons = 0;
	_leftStick = {};
	_rightStick = {};
}

void SwitchInput::SetDisabled(bool disabled)
{
	_disabled = disabled;
}

u64 SwitchInput::GetButtonsDown()
{
	return padGetButtonsDown(&_pad);
}
