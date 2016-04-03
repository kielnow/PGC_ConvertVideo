﻿#pragma once

#include <cstdint>
#include <cassert>

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

}

#define ZEN_ASSERT(cond, message)\
{\
	(void)message;\
	assert(cond);\
}