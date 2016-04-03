#pragma once

#include <cstdint>

namespace zen
{

	using namespace std;

	typedef int8_t		s8;
	typedef int16_t		s16;
	typedef int32_t		s32;
	typedef int64_t		s64;
	typedef uint8_t		u8;
	typedef uint16_t	u16;
	typedef uint32_t	u32;
	typedef uint64_t	u64;
	typedef float		f32;
	typedef double		f64;
	typedef char		c8;
	typedef wchar_t		c16;
	typedef bool		b8;
	typedef intptr_t	intptr;
	typedef size_t		uintptr;

	template <class T> void SafeRelease(T **ppT)
	{
		if (*ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}

}

#define ZEN_ASSERT(cond, message)\
do {\
	OutputDebugString(message);\
	assert(cond);\
} while(0)

#define SAFE_DELETE(p)			if (p) { delete p; p = nullptr; }
#define SAFE_DELETE_ARRAY(p)	if (p) { delete [] p; p = nullptr; }

#define SAFE_RELEASE(p)			zen::SafeRelease(&p)

#define CHECK_HRESULT(hr)		ZEN_ASSERT(SUCCEEDED(hr), L"Fatal error.")
