#include "SwitchLog.h"

#include <switch.h>
#include <cstdarg>
#include <cstdio>

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

	printf("%s", buffer);
	svcOutputDebugString(buffer, (u64)length);
}
