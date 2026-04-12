// stdafx.h: native desktop precompiled header for macOS/Linux builds.

#pragma once

#include <algorithm>
#include <assert.h>
#include <cfloat>
#include <cstdint>
#include <deque>
#include <exception>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <math.h>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <lce_abi/lce_abi.h>

#ifndef MINECRAFT_WORLD_SKIP_NATIVE_DESKTOP_STUBS
#include "NativeDesktop/NativeDesktopStubs.h"
#endif

#ifndef MINECRAFT_NATIVE_BYTE_DEFINED
typedef unsigned char mc_byte;
#define MINECRAFT_NATIVE_BYTE_DEFINED
#endif
#ifndef byte
#define byte mc_byte
#endif

#include "Definitions.h"
#include "Class.h"
#include "Exceptions.h"
#include "Mth.h"
#include "StringHelpers.h"
#include "ArrayWithLength.h"
#include "Random.h"
#include "TilePos.h"
#include "ChunkPos.h"
#include "compression.h"
#include "PerformanceTimer.h"

#ifdef _FINAL_BUILD
#define printf BREAKTHECOMPILE
#define wprintf BREAKTHECOMPILE
#undef OutputDebugString
#define OutputDebugString BREAKTHECOMPILE
#define OutputDebugStringA BREAKTHECOMPILE
#define OutputDebugStringW BREAKTHECOMPILE
#endif


void MemSect(int sect);
