#pragma once

// Standard Library
#include <cstdlib>
#include <cassert>
#include <memory>
#include <string>
#include <tchar.h>

// Windows
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// COM
#include <objbase.h>

// Direct2D
#include <d2d1.h>
#include <d2d1helper.h>
#include <d2dbasetypes.h>
#include <D2DErr.h>

// zen
#include "std.h"
#include "App.h"
#include "Graphics2D.h"