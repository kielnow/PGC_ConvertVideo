#pragma once

namespace zen
{

	class Debug
	{
	public:
		static void trace(const c16* format, ...);

		static void traceLine(const c16* format, ...);

		static void debugBreak();
	};

}