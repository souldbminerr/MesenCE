#define MemoryType LibnxMemoryType
#define DebugEventType LibnxDebugEventType
#include <switch.h>
#undef MemoryType
#undef DebugEventType

#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>

#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/KeyManager.h"
#include "Shared/MessageManager.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/VirtualFile.h"

#include "SwitchRenderer.h"
#include "SwitchAudio.h"
#include "SwitchInput.h"
#include "SwitchLog.h"

static std::string ToLower(std::string s)
{
	for(char& c : s) {
		c = (char)tolower((unsigned char)c);
	}
	return s;
}

// Maps a -s system name to a ConsoleType. Returns ConsoleType count sentinel
// (auto) when the name is unknown; ROM auto-detection still applies.
static bool ParseSystem(const std::string& name, ConsoleType& out)
{
	std::string s = ToLower(name);
	if(s == "nes" || s == "famicom" || s == "fds" || s == "nsf") {
		out = ConsoleType::Nes;
	} else if(s == "snes" || s == "sfc" || s == "superfamicom" || s == "supernes") {
		out = ConsoleType::Snes;
	} else if(s == "gb" || s == "gbc" || s == "sgb" || s == "gameboy" || s == "gameboycolor") {
		out = ConsoleType::Gameboy;
	} else if(s == "gba" || s == "gameboyadvance") {
		out = ConsoleType::Gba;
	} else if(s == "pce" || s == "pcengine" || s == "tg16" || s == "turbografx" || s == "sgx" || s == "supergrafx") {
		out = ConsoleType::PcEngine;
	} else if(s == "sms" || s == "mastersystem" || s == "gg" || s == "gamegear" || s == "sg" || s == "coleco") {
		out = ConsoleType::Sms;
	} else if(s == "ws" || s == "wsc" || s == "wonder" || s == "wonderswan") {
		out = ConsoleType::Ws;
	} else {
		return false;
	}
	return true;
}

static const char* ConsoleTypeName(ConsoleType type)
{
	switch(type) {
		case ConsoleType::Snes: return "SNES";
		case ConsoleType::Gameboy: return "Game Boy";
		case ConsoleType::Nes: return "NES";
		case ConsoleType::PcEngine: return "PC Engine";
		case ConsoleType::Sms: return "Master System";
		case ConsoleType::Gba: return "Game Boy Advance";
		case ConsoleType::Ws: return "WonderSwan";
		default: return "Unknown";
	}
}

static void ConfigureControllers(EmuSettings* settings)
{
	// Player 1 on a standard controller for every core. Directions accept
	// both the D-Pad and either analog stick (see SwitchInput).
	KeyMapping base = {};
	base.Up = SwUp;
	base.Down = SwDown;
	base.Left = SwLeft;
	base.Right = SwRight;
	base.A = SwA;
	base.B = SwB;
	base.X = SwX;
	base.Y = SwY;
	base.L = SwL;
	base.R = SwR;
	base.Start = SwPlus;
	base.Select = SwMinus;

	{
		NesConfig cfg = settings->GetNesConfig();
		cfg.Port1.Type = ControllerType::NesController;
		cfg.Port1.Keys.Mapping1 = base;
		settings->SetNesConfig(cfg);
	}
	{
		SnesConfig cfg = settings->GetSnesConfig();
		cfg.Port1.Type = ControllerType::SnesController;
		cfg.Port1.Keys.Mapping1 = base;
		settings->SetSnesConfig(cfg);
	}
	{
		GameboyConfig cfg = settings->GetGameboyConfig();
		cfg.Controller.Type = ControllerType::GameboyController;
		cfg.Controller.Keys.Mapping1 = base;
		settings->SetGameboyConfig(cfg);
	}
	{
		GbaConfig cfg = settings->GetGbaConfig();
		cfg.Controller.Type = ControllerType::GbaController;
		cfg.Controller.Keys.Mapping1 = base;
		settings->SetGbaConfig(cfg);
	}
	{
		PcEngineConfig cfg = settings->GetPcEngineConfig();
		cfg.Port1.Type = ControllerType::PceController;
		cfg.Port1.Keys.Mapping1 = base;
		settings->SetPcEngineConfig(cfg);
	}
	{
		SmsConfig cfg = settings->GetSmsConfig();
		cfg.Port1.Type = ControllerType::SmsController;
		cfg.Port1.Keys.Mapping1 = base;
		settings->SetSmsConfig(cfg);
	}
	{
		WsConfig cfg = settings->GetWsConfig();
		cfg.ControllerHorizontal.Type = ControllerType::WsController;
		cfg.ControllerHorizontal.Keys.Mapping1 = base;
		settings->SetWsConfig(cfg);
	}
}

int main(int argc, char* argv[])
{
	std::string systemArg;
	std::string romArg;

	/* Parse program parameters */
	for(int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if((arg == "-s" || arg == "--system") && i + 1 < argc) {
			systemArg = argv[++i];
		} else if((arg == "-r" || arg == "--rom") && i + 1 < argc) {
			romArg = argv[++i];
		} else if(arg[0] != '-' && romArg.empty()) {
			romArg = arg;
		}
	}

	romfsInit();

	/* Set the system to emulate*/
	ConsoleType expectedConsole = ConsoleType::Nes;
	bool systemValid = true;
	if(!systemArg.empty()) {
		systemValid = ParseSystem(systemArg, expectedConsole);
		if(!systemValid) {
			SwitchLog("Unknown system '%s'\n", systemArg.c_str());
		}
	}

	/* Load the ROM*/
	std::string romPath = romArg.empty() ? "romfs:/badapple.nes" : romArg;
	SwitchLog("Loading ROM: %s\n", romPath.c_str());

	/* Set the home folder to proper location for libnx */
	FolderUtilities::SetHomeFolder("sdmc:/emu/cores/mesen");

	/* Create a emulator instance*/
	unique_ptr<Emulator> emu(new Emulator());
	emu->Initialize();
	KeyManager::SetSettings(emu->GetSettings());

	/* Set up audio */
	AudioConfig audio = emu->GetSettings()->GetAudioConfig();
	audio.EnableAudio = true;
	audio.SampleRate = 48000;
	emu->GetSettings()->SetAudioConfig(audio);

	ConfigureControllers(emu->GetSettings());

	unique_ptr<SwitchInput> input(new SwitchInput());
	KeyManager::RegisterKeyManager(input.get());

	/* Create the Switch renderer and audio*/
	SwitchRenderer* renderer = new SwitchRenderer(emu.get());
	if(!renderer->IsInitialized()) {
		SwitchLog("Failed to initialize deko3d renderer.\n");
		delete renderer;
		romfsExit();
		return 1;
	}
	SwitchAudio* audioDevice = new SwitchAudio(emu.get());

	/* Set callbacks to the core */
	emu->SetAudioVideoInitCallback(
	    [audioDevice]() -> IAudioDevice* { return audioDevice; },
	    [renderer]() -> IRenderingDevice* { return renderer; });

	/* Try to load the ROM*/
	if(!emu->LoadRom(VirtualFile(romPath), VirtualFile())) {
		SwitchLog("Failed to load ROM: %s\n", romPath.c_str());
		emu->Stop(true);
		emu->Release();
		romfsExit();
		return 1;
	}

	/* Handle invalid systems*/
	if(systemValid && !systemArg.empty() && emu->GetConsoleType() != expectedConsole) {
		SwitchLog("Note: -s %s given, but ROM detected as %s.\n",
		    systemArg.c_str(), ConsoleTypeName(emu->GetConsoleType()));
	}
	SwitchLog("Running in %s mode\n", ConsoleTypeName(emu->GetConsoleType()));

	emu->Run();

	// emu->Stop(true);
	// emu->Release();
	// emu.reset();
	// input.reset();

	// romfsExit();
	// return 0;
}
