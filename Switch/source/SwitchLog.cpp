#include "SwitchLog.h"

#include <switch.h>
#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>

static bool s_logToFile = false;
static bool s_logChecked = false;
static FILE* s_logFile = nullptr;

static void EnsureLogEnabled()
{
	if(s_logChecked) return;
	s_logChecked = true;

	// Create directory if needed
	mkdir("sdmc:/emu", 0777);
	mkdir("sdmc:/emu/cores", 0777);
	mkdir("sdmc:/emu/cores/mesen", 0777);

	FILE* flag = fopen("sdmc:/emu/cores/mesen/log.flag", "r");
	if(flag) {
		fclose(flag);
		s_logToFile = true;
		s_logFile = fopen("sdmc:/emu/cores/mesen/log.txt", "a");
	}
}

/* Debug string will only appear in emulator or over UART with a debug-mode kernel */
void SwitchLog(const char* format, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if(length < 0) {
		return;
	}

	if(length >= (int)sizeof(buffer)) {
		length = sizeof(buffer) - 1;
	}

	EnsureLogEnabled();

	printf("%s", buffer);
	svcOutputDebugString(buffer, (u64)length);

	if(s_logToFile && s_logFile) {
		fputs(buffer, s_logFile);
		fflush(s_logFile);
	}
}