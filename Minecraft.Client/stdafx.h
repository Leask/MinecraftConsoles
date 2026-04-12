// stdafx.h: native desktop precompiled header for macOS/Linux builds.

#pragma once

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "(" __STR1__(__LINE__) ") : 4J Warning Msg: "

#include <algorithm>
#include <assert.h>
#include <cfloat>
#include <climits>
#include <cstdint>
#include <deque>
#include <exception>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <lce_abi/lce_abi.h>

#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define MINECRAFT_WORLD_SKIP_NATIVE_DESKTOP_STUBS

#include "../Minecraft.World/x64headers/extraX64.h"

#ifndef MINECRAFT_NATIVE_BYTE_DEFINED
typedef unsigned char mc_byte;
#define MINECRAFT_NATIVE_BYTE_DEFINED
#endif

#ifndef byte
#define byte mc_byte
#endif

#include "../Minecraft.World/Definitions.h"
#include "../Minecraft.World/Class.h"
#include "../Minecraft.World/ArrayWithLength.h"
#include "../Minecraft.World/SharedConstants.h"
#include "../Minecraft.World/Random.h"
#include "../Minecraft.World/compression.h"
#include "../Minecraft.World/PerformanceTimer.h"

#include "NativeDesktop/NativeDesktopClientStubs.h"

#include "Textures.h"
#include "Font.h"
#include "ClientConstants.h"
#include "Gui.h"
#include "Screen.h"
#include "ScreenSizeCalculator.h"
#include "Minecraft.h"
#include "MemoryTracker.h"
#include "stubs.h"
#include "BufferedImage.h"

#include "Common/Network/GameNetworkManager.h"

#include "Common/App_Defines.h"
#include "Common/UI/UIEnums.h"
#include "Common/UI/UIStructs.h"
#include "Common/App_enums.h"
#include "Common/Tutorial/TutorialEnum.h"
#include "Common/App_structs.h"

#include "Common/Consoles_App.h"
#include "NativeDesktop/NativeDesktopClientApp.h"
#include "Common/Minecraft_Macros.h"
#include "Common/Potion_Macros.h"
#include "Common/BuildVer.h"

#include "Common/Audio/SoundEngine.h"
#include "NativeDesktop/SocialManager.h"
#include "NativeDesktop/NativeDesktop_UIController.h"

#include "Common/ConsoleGameMode.h"
#include "Common/Console_Debug_enum.h"
#include "Common/Console_Awards_enum.h"
#include "Common/Tutorial/TutorialMode.h"
#include "Common/Tutorial/Tutorial.h"
#include "Common/Tutorial/FullTutorialMode.h"
#include "Common/Trial/TrialMode.h"
#include "Common/GameRules/ConsoleGameRules.h"
#include "Common/GameRules/ConsoleSchematicFile.h"
#include "Common/Colours/ColourTable.h"
#include "Common/DLC/DLCSkinFile.h"
#include "Common/DLC/DLCManager.h"
#include "Common/DLC/DLCPack.h"
#include "Common/Telemetry/TelemetryManager.h"

#include "extraX64client.h"

#ifdef _FINAL_BUILD
#define printf BREAKTHECOMPILE
#define wprintf BREAKTHECOMPILE
#undef OutputDebugString
#define OutputDebugString BREAKTHECOMPILE
#define OutputDebugStringA BREAKTHECOMPILE
#define OutputDebugStringW BREAKTHECOMPILE
#endif

void MemSect(int sect);
