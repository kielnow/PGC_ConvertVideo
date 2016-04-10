#pragma once

// Standard Library
#include <cstdlib>
#include <cassert>
#include <memory>
#include <string>
#include <tchar.h>

#include <vector>

// Windows
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <CommCtrl.h>

// Shell
#include <shellapi.h>

// COM
#include <objbase.h>
#include <wrl/client.h>

// zen
#include "std.h"
#include "Debug.h"
#include "Singleton.h"
#include "Graphics2D.h"
#include "GraphicsWIC.h"
#include "Window.h"
#include "App.h"
