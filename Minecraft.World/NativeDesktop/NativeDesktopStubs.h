#pragma once

#include <functional>
#include <string>
#include <vector>

#include <lce_win32/lce_win32.h>

#include "../../Minecraft.Client/SkinBox.h"

#define MULTITHREAD_ENABLE

typedef unsigned char mc_byte;
#define byte mc_byte

const int XUSER_INDEX_ANY = 255;
const int XUSER_INDEX_FOCUS = 254;
const int XUSER_MAX_COUNT = 4;
const int MINECRAFT_NET_MAX_PLAYERS = 256;

typedef ULONGLONG PlayerUID;
typedef ULONGLONG SessionID;
typedef PlayerUID GameSessionUID;
typedef PlayerUID* PPlayerUID;

class INVITE_INFO;

typedef struct _XUIOBJ* HXUIOBJ;
typedef struct _XUICLASS* HXUICLASS;
typedef struct _XUIBRUSH* HXUIBRUSH;
typedef struct _XUIDC* HXUIDC;

bool IsEqualXUID(PlayerUID a, PlayerUID b);
