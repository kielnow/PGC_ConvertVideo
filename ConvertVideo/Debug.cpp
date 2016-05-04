#include "stdafx.h"
#include "Debug.h"

using namespace zen;

void Debug::trace(const c16* format, ...)
{
	c16 buf[2048] = {};
	va_list ap;
	va_start(ap, format);
	vswprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	OutputDebugString(buf);
}

void Debug::traceLine(const c16* format, ...)
{
	c16 buf[2048] = {};
	va_list ap;
	va_start(ap, format);
	vswprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	wcscat_s(buf, L"\r\n");
	OutputDebugString(buf);
}

void Debug::debugBreak()
{
	DebugBreak();
}