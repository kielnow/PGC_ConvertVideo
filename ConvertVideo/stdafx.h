#pragma once

// Standard Library
#include <tchar.h>
#include <cstdlib>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>

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

#include "HeaderWriter.h"
