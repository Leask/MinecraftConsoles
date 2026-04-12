#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include <lce_win32/lce_win32.h>

#include "Common/BuildVer.h"
#include "../../Minecraft.Client/Common/App_enums.h"
#include "../../Minecraft.Client/Common/Console_Debug_enum.h"
#include "../../Minecraft.Client/Common/Console_Awards_enum.h"
#include "../../Minecraft.Client/SkinBox.h"
#include "../../Minecraft.Client/Windows64/Sentient/TelemetryEnum.h"
#include "../../Minecraft.Client/Windows64Media/strings.h"

#define MULTITHREAD_ENABLE

#ifndef MAX_CAPENAME_SIZE
#define MAX_CAPENAME_SIZE 32
#endif

#ifndef MINECRAFT_NATIVE_BYTE_DEFINED
typedef unsigned char mc_byte;
#define MINECRAFT_NATIVE_BYTE_DEFINED
#endif
#ifndef byte
#define byte mc_byte
#endif

const int XUSER_INDEX_ANY = 255;
const int XUSER_INDEX_FOCUS = 254;
const int XUSER_MAX_COUNT = 4;
const int MINECRAFT_NET_MAX_PLAYERS = 256;

#ifndef MINECRAFT_NET_VERSION
#define MINECRAFT_NET_VERSION VER_NETWORK
#endif

typedef ULONGLONG PlayerUID;
typedef ULONGLONG SessionID;
typedef PlayerUID GameSessionUID;
typedef PlayerUID* PPlayerUID;
typedef unsigned char boolean;

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#endif

#ifndef LCE_WIN32_OUTPUT_DEBUG_STRING_DEFINED
#define LCE_WIN32_OUTPUT_DEBUG_STRING_DEFINED
inline void OutputDebugStringA(LPCSTR value)
{
    (void)value;
}

inline void OutputDebugStringW(LPCWSTR value)
{
    (void)value;
}
#endif

class INVITE_INFO;

typedef struct _XUIOBJ* HXUIOBJ;
typedef struct _XUICLASS* HXUICLASS;
typedef struct _XUIBRUSH* HXUIBRUSH;
typedef struct _XUIDC* HXUIDC;

#ifndef FILE_BEGIN
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#endif

#ifndef PAGE_READWRITE
#define PAGE_READWRITE 0x04
#endif

#ifndef MEM_COMMIT
#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_DECOMMIT 0x00004000
#define MEM_RELEASE 0x00008000
#define MEM_LARGE_PAGES 0x20000000
#endif

#ifndef GAME_RULE_SAVENAME
#define GAME_RULE_SAVENAME L"requiredGameRules.grf"
#endif

#ifndef MAKE_SKIN_BITMASK
#define MAKE_SKIN_BITMASK(dlcSkin, skinId) \
    (((dlcSkin) ? 0x80000000u : 0u) | ((skinId) & 0x7fffffffu))
#endif

#ifndef GET_DLC_SKIN_ID_FROM_BITMASK
#define GET_DLC_SKIN_ID_FROM_BITMASK(uiBitmask) \
    (static_cast<DWORD>(uiBitmask) & 0x7fffffffu)
#define GET_UGC_SKIN_ID_FROM_BITMASK(uiBitmask) \
    (static_cast<DWORD>(uiBitmask) & 0x7fffffe0u)
#define GET_DEFAULT_SKIN_ID_FROM_BITMASK(uiBitmask) \
    (static_cast<DWORD>(uiBitmask) & 0x0000001fu)
#define GET_IS_DLC_SKIN_FROM_BITMASK(uiBitmask) \
    ((static_cast<DWORD>(uiBitmask) & 0x80000000u) != 0)
#endif

#ifndef INVALID_XUID
const int INVALID_XUID = 0;
#endif

#ifndef sprintf_s
#define sprintf_s(buffer, size, format, ...) \
    std::snprintf((buffer), (size), (format), __VA_ARGS__)
#endif

#ifndef LCE_WIN32_SWPRINTF_S_DEFINED
#define LCE_WIN32_SWPRINTF_S_DEFINED
template <typename... Args>
inline int swprintf_s(wchar_t* buffer, size_t size, const wchar_t* format,
                      Args... args)
{
    return std::swprintf(buffer, size, format, args...);
}

template <size_t N, typename... Args>
inline int swprintf_s(wchar_t (&buffer)[N], const wchar_t* format,
                      Args... args)
{
    return std::swprintf(buffer, N, format, args...);
}
#endif

#ifndef MASK_REGENERATION
#define MASK_REGENERATION 0x2001
#define MASK_SPEED 0x2002
#define MASK_FIRE_RESISTANCE 0x2003
#define MASK_POISON 0x2004
#define MASK_INSTANTHEALTH 0x2005
#define MASK_NIGHTVISION 0x2006
#define MASK_INVISIBILITY 0x200E
#define MASK_WEAKNESS 0x2008
#define MASK_STRENGTH 0x2009
#define MASK_SLOWNESS 0x200A
#define MASK_INSTANTDAMAGE 0x200C
#define MASK_TYPE_AWKWARD 0x0010
#define MASK_SPLASH 0x4000
#define MASK_BIT13 0x2000
#define MASK_LEVEL2 0x0020
#define MASK_EXTENDED 0x0040
#define MASK_LEVEL2EXTENDED 0x0060
#define MACRO_POTION_IS_REGENERATION(aux) \
    (((aux) & 0x200F) == MASK_REGENERATION)
#define MACRO_POTION_IS_SPEED(aux) \
    (((aux) & 0x200F) == MASK_SPEED)
#define MACRO_POTION_IS_FIRE_RESISTANCE(aux) \
    (((aux) & 0x200F) == MASK_FIRE_RESISTANCE)
#define MACRO_POTION_IS_INSTANTHEALTH(aux) \
    (((aux) & 0x200F) == MASK_INSTANTHEALTH)
#define MACRO_POTION_IS_NIGHTVISION(aux) \
    (((aux) & 0x200F) == MASK_NIGHTVISION)
#define MACRO_POTION_IS_INVISIBILITY(aux) \
    (((aux) & 0x200F) == MASK_INVISIBILITY)
#define MACRO_POTION_IS_WEAKNESS(aux) \
    (((aux) & 0x200F) == MASK_WEAKNESS)
#define MACRO_POTION_IS_STRENGTH(aux) \
    (((aux) & 0x200F) == MASK_STRENGTH)
#define MACRO_POTION_IS_SLOWNESS(aux) \
    (((aux) & 0x200F) == MASK_SLOWNESS)
#define MACRO_POTION_IS_POISON(aux) \
    (((aux) & 0x200F) == MASK_POISON)
#define MACRO_POTION_IS_INSTANTDAMAGE(aux) \
    (((aux) & 0x200F) == MASK_INSTANTDAMAGE)
#define MACRO_POTION_IS_SPLASH(aux) \
    (((aux) & MASK_SPLASH) == MASK_SPLASH)
#define MACRO_POTION_IS_BOTTLE(aux) (((aux) & MASK_SPLASH) == 0)
#define MACRO_POTION_IS_AKWARD(aux) \
    (((aux) & MASK_TYPE_AWKWARD) == MASK_TYPE_AWKWARD)
#define MACRO_POTION_IS_REGULAR(aux) \
    (((aux) & MASK_LEVEL2EXTENDED) == 0)
#define MACRO_POTION_IS_LEVEL2(aux) \
    (((aux) & MASK_LEVEL2) == MASK_LEVEL2)
#define MACRO_POTION_IS_EXTENDED(aux) \
    (((aux) & MASK_EXTENDED) == MASK_EXTENDED)
#define MACRO_POTION_IS_LEVEL2EXTENDED(aux) \
    (((aux) & MASK_LEVEL2EXTENDED) == MASK_LEVEL2EXTENDED)
#define MACRO_MAKEPOTION_AUXVAL(potion_type, potion_strength, potion_effect) \
    ((potion_type) | (potion_strength) | (potion_effect))
#define NORMALISE_POTION_AUXVAL(aux) \
    ((aux) & (MASK_BIT13 | MASK_SPLASH | 0xFF))
#endif

inline bool IsEqualXUID(PlayerUID a, PlayerUID b)
{
    return a == b;
}

template <typename T>
class XLockFreeStack
{
public:
    XLockFreeStack()
    {
        InitializeCriticalSectionAndSpinCount(&m_criticalSection, 5120);
    }

    ~XLockFreeStack()
    {
        DeleteCriticalSection(&m_criticalSection);
    }

    void Initialize()
    {
    }

    void Push(T* data)
    {
        EnterCriticalSection(&m_criticalSection);
        m_stack.push_back(data);
        LeaveCriticalSection(&m_criticalSection);
    }

    T* Pop()
    {
        EnterCriticalSection(&m_criticalSection);
        if (m_stack.empty())
        {
            LeaveCriticalSection(&m_criticalSection);
            return nullptr;
        }

        T* value = m_stack.back();
        m_stack.pop_back();
        LeaveCriticalSection(&m_criticalSection);
        return value;
    }

private:
    std::vector<T*> m_stack;
    CRITICAL_SECTION m_criticalSection;
};

inline void XMemCpy(void* destination, const void* source, size_t size)
{
    std::memcpy(destination, source, size);
}

inline void XMemSet(void* destination, int value, size_t size)
{
    std::memset(destination, value, size);
}

inline void XMemSet128(void* destination, int value, size_t size)
{
    std::memset(destination, value, size);
}

inline void* XPhysicalAlloc(
    SIZE_T size,
    ULONG_PTR highestAcceptableAddress,
    ULONG_PTR alignment,
    DWORD protect)
{
    (void)highestAcceptableAddress;
    (void)alignment;
    (void)protect;
    return std::malloc(size);
}

inline void XPhysicalFree(void* allocation)
{
    std::free(allocation);
}

inline void XSetThreadProcessor(HANDLE thread, int processor)
{
    SetThreadAffinityMask(thread, static_cast<DWORD_PTR>(1) << processor);
}

class DLCSkinFile
{
public:
    DWORD getAdditionalBoxesCount() const { return 0; }
    std::vector<SKIN_BOX*>* getAdditionalBoxes() const { return nullptr; }
    DWORD getSkinID() const { return 0; }
    unsigned int getAnimOverrideBitmask() const { return 0; }
};
class DataOutputStream;
class AABB;
class Level;
class LevelChunk;
class LocalPlayer;
class ModelPart;
class Player;

class LevelGenerationOptions
{
public:
    bool checkIntersects(
        int x0,
        int y0,
        int z0,
        int x1,
        int y1,
        int z1) const
    {
        (void)x0;
        (void)y0;
        (void)z0;
        (void)x1;
        (void)y1;
        (void)z1;
        return false;
    }

    bool isFeatureChunk(int x, int z, int feature, int* orientation = nullptr) const
    {
        (void)x;
        (void)z;
        (void)feature;
        if (orientation != nullptr)
        {
            *orientation = 0;
        }
        return false;
    }

    bool isFromDLC() const { return false; }
    bool requiresBaseSave() const { return false; }
    PBYTE getBaseSaveData(DWORD& size) const
    {
        size = 0;
        return nullptr;
    }
    void deleteBaseSaveData() {}

    void getBiomeOverride(int biomeId, BYTE& tile, BYTE& topTile) const
    {
        (void)biomeId;
        (void)tile;
        (void)topTile;
    }
};

class ColourTable
{
public:
    int getColor(eMinecraftColour color) const
    {
        (void)color;
        return 0xffffff;
    }
};

class ProgressRenderer
{
public:
    enum eProgressStringType
    {
        eProgressStringType_ID,
        eProgressStringType_String,
    };

    int getCurrentPercent() const { return m_lastPercent; }
    int getCurrentTitle() const { return m_title; }
    int getCurrentStatus() const { return m_status; }
    std::wstring& getProgressString() { return m_text; }
    eProgressStringType getType() const { return m_type; }

    void progressStart(int title)
    {
        m_title = title;
        m_noAbort = false;
    }

    void progressStartNoAbort(int title)
    {
        m_title = title;
        m_noAbort = true;
    }

    void _progressStart(int title)
    {
        progressStart(title);
    }

    void progressStage(unsigned int stringId)
    {
        m_status = static_cast<int>(stringId);
        m_type = eProgressStringType_ID;
    }

    void progressStage(std::wstring& text)
    {
        m_text = text;
        m_type = eProgressStringType_String;
    }

    void progressStagePercentage(int percent)
    {
        m_lastPercent = percent;
    }

private:
    int m_lastPercent = 0;
    int m_status = 0;
    int m_title = 0;
    bool m_noAbort = false;
    std::wstring m_text;
    eProgressStringType m_type = eProgressStringType_ID;
};

class DestroyedTileManager
{
public:
    void addAABBs(Level* level, AABB* box, std::vector<AABB*>* boxes)
    {
        (void)level;
        (void)box;
        (void)boxes;
    }
};

class LevelRenderer
{
public:
    DestroyedTileManager* destroyedTileManager = &m_destroyedTileManager;

private:
    DestroyedTileManager m_destroyedTileManager;
};

class MultiPlayerLevel
{
public:
    LevelChunk* getChunkAt(int x, int z)
    {
        (void)x;
        (void)z;
        return nullptr;
    }
};

class Minecraft
{
public:
    Minecraft()
        : localplayers(XUSER_MAX_COUNT)
    {
    }

    static Minecraft* GetInstance()
    {
        static Minecraft minecraft;
        return &minecraft;
    }

    bool isTutorial() const { return false; }
    ColourTable* getColourTable() { return &m_colourTable; }
    MultiPlayerLevel* getLevel(int dimensionId)
    {
        (void)dimensionId;
        return nullptr;
    }
    unsigned int getCurrentTexturePackId() const { return 0; }

    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Player>> localplayers;
    ProgressRenderer* progressRenderer = &m_progressRenderer;
    LevelRenderer* levelRenderer = &m_levelRenderer;

private:
    ColourTable m_colourTable;
    ProgressRenderer m_progressRenderer;
    LevelRenderer m_levelRenderer;
};

struct MOJANG_DATA
{
    eXUID eXuid;
    WCHAR wchCape[MAX_CAPENAME_SIZE];
    WCHAR wchSkin[MAX_CAPENAME_SIZE];
};

struct STRING_VERIFY_RESPONSE
{
    BOOL bFailed;
};

class ServerLevel
{
public:
    static void staticCtor() {}
};

class GameRenderer
{
public:
    template <typename T>
    static void AddForDelete(T* value)
    {
        delete value;
    }

    static void FinishedReassigning() {}
};

enum NativeDesktopTextureName
{
    TN_MOB_CHAR = 0,
    TN_MOB_CHAR1,
    TN_MOB_CHAR2,
    TN_MOB_CHAR3,
    TN_MOB_CHAR4,
    TN_MOB_CHAR5,
    TN_MOB_CHAR6,
    TN_MOB_CHAR7,
    TN_MOB_OZELOT,
    TN_MOB_CAT_BLACK,
    TN_MOB_CAT_RED,
    TN_MOB_CAT_SIAMESE,
    TN_MOB_HORSE_BLACK,
    TN_MOB_HORSE_BROWN,
    TN_MOB_HORSE_CHESTNUT,
    TN_MOB_HORSE_CREAMY,
    TN_MOB_HORSE_DARKBROWN,
    TN_MOB_HORSE_GRAY,
    TN_MOB_HORSE_MARKINGS_BLACKDOTS,
    TN_MOB_HORSE_MARKINGS_WHITE,
    TN_MOB_HORSE_MARKINGS_WHITEDOTS,
    TN_MOB_HORSE_MARKINGS_WHITEFIELD,
    TN_MOB_HORSE_WHITE,
    TN_MOB_HORSE_ARMOR_DIAMOND,
    TN_MOB_HORSE_ARMOR_GOLD,
    TN_MOB_HORSE_ARMOR_IRON
};

class HumanoidModel
{
public:
    enum animbits
    {
        eAnim_ArmsDown = 0,
        eAnim_ArmsOutFront,
        eAnim_NoLegAnim,
        eAnim_HasIdle,
        eAnim_ForceAnim,
        eAnim_SingleLegs,
        eAnim_SingleArms,
        eAnim_StatueOfLiberty,
        eAnim_DontRenderArmour,
        eAnim_NoBobbing,
        eAnim_DisableRenderHead,
        eAnim_DisableRenderArm0,
        eAnim_DisableRenderArm1,
        eAnim_DisableRenderTorso,
        eAnim_DisableRenderLeg0,
        eAnim_DisableRenderLeg1,
        eAnim_DisableRenderHair,
        eAnim_SmallModel
    };

    static const unsigned int m_staticBitmaskIgnorePlayerCustomAnimSetting =
        (1u << eAnim_ForceAnim) |
        (1u << eAnim_DisableRenderArm0) |
        (1u << eAnim_DisableRenderArm1) |
        (1u << eAnim_DisableRenderTorso) |
        (1u << eAnim_DisableRenderLeg0) |
        (1u << eAnim_DisableRenderLeg1) |
        (1u << eAnim_DisableRenderHair);
};

class ConsoleGameRules
{
public:
    enum EGameRuleType
    {
        eGameRuleType_Invalid = -1,
        eGameRuleType_Root = 0,
        eGameRuleType_LevelGenerationOptions,
        eGameRuleType_ApplySchematic,
        eGameRuleType_GenerateStructure,
        eGameRuleType_GenerateBox,
        eGameRuleType_PlaceBlock,
        eGameRuleType_PlaceContainer,
        eGameRuleType_PlaceSpawner,
        eGameRuleType_BiomeOverride,
        eGameRuleType_StartFeature,
        eGameRuleType_AddItem,
        eGameRuleType_AddEnchantment,
        eGameRuleType_LevelRules,
        eGameRuleType_NamedArea,
        eGameRuleType_UseTileRule,
        eGameRuleType_CollectItemRule,
        eGameRuleType_CompleteAllRule,
        eGameRuleType_UpdatePlayerRule,
        eGameRuleType_Count
    };

    enum EGameRuleAttr
    {
        eGameRuleAttr_Invalid = -1,
        eGameRuleAttr_Count
    };

    static void write(DataOutputStream* dos, EGameRuleType type);
    static void write(DataOutputStream* dos, EGameRuleAttr attr);
};

class NativeDesktopDLCManagerStub
{
public:
    DLCSkinFile* getSkinFile(const std::wstring& skinPath)
    {
        (void)skinPath;
        return nullptr;
    }
};

class NativeDesktopAppStub
{
public:
    void DebugPrintf(const char* format, ...)
    {
        if (format == nullptr)
        {
            return;
        }

        va_list args;
        va_start(args, format);
        std::vfprintf(stderr, format, args);
        va_end(args);
    }

    bool DebugSettingsOn() const { return false; }
    bool DebugArtToolsOn() const { return false; }
    bool DefaultCapeExists() const { return false; }
    bool GetMobsDontAttackEnabled() const { return false; }
    bool GetMobsDontTickEnabled() const { return false; }
    bool GetResetNether() const { return false; }
    bool GetWriteSavesToFolderEnabled() const { return false; }
    bool IsFileInMemoryTextures(const std::wstring& path) const
    {
        (void)path;
        return false;
    }
    bool isXuidNotch(PlayerUID xuid) const
    {
        (void)xuid;
        return false;
    }
    bool GetTerrainFeaturePosition(_eTerrainFeatureType type, int* x, int* z)
    {
        (void)type;
        (void)x;
        (void)z;
        return false;
    }

    int GetGameHostOption(eGameHostOption option) const
    {
        switch (option)
        {
        case eGameHostOption_TNT:
        case eGameHostOption_FireSpreads:
        case eGameHostOption_MobGriefing:
        case eGameHostOption_DoMobSpawning:
        case eGameHostOption_DoMobLoot:
        case eGameHostOption_DoTileDrops:
        case eGameHostOption_NaturalRegeneration:
        case eGameHostOption_DoDaylightCycle:
            return 1;
        default:
            return 0;
        }
    }
    int GetGameSettings(eGameSetting setting) const
    {
        (void)setting;
        return 0;
    }
    unsigned int GetGameSettingsDebugMask(int pad = 0) const
    {
        (void)pad;
        return 0;
    }
    unsigned int GetGameNewWorldSize() const { return 0; }
    unsigned int GetGameNewHellScale() const { return 0; }
    unsigned int GetGameNewWorldSizeUseMoat() const { return 0; }
    unsigned int GetHTMLColor(int color) const
    {
        (void)color;
        return 0xffffff;
    }
    unsigned int GetAnimOverrideBitmask(DWORD skinId) const
    {
        (void)skinId;
        return 0;
    }
    unsigned int CreateImageTextData(
        PBYTE textMetadata,
        int64_t seed,
        bool hasSeed,
        unsigned int hostOptions,
        unsigned int texturePackId)
    {
        (void)textMetadata;
        (void)seed;
        (void)hasSeed;
        (void)hostOptions;
        (void)texturePackId;
        return 0;
    }
    int64_t getAppTime() const { return 0; }

    const WCHAR* GetString(unsigned int stringId) const
    {
        (void)stringId;
        return L"";
    }
    std::wstring getSkinPathFromId(DWORD skinId) const
    {
        (void)skinId;
        return std::wstring();
    }
    LevelGenerationOptions* getLevelGenerationOptions() const
    {
        return nullptr;
    }
    MOJANG_DATA* GetMojangDataForXuid(PlayerUID xuid)
    {
        (void)xuid;
        return nullptr;
    }
    std::vector<ModelPart*>* GetAdditionalModelParts(DWORD skinId)
    {
        (void)skinId;
        return nullptr;
    }
    std::vector<ModelPart*>* SetAdditionalSkinBoxes(
        DWORD skinId,
        std::vector<SKIN_BOX*>* skinBoxes)
    {
        (void)skinId;
        (void)skinBoxes;
        return nullptr;
    }

    void AddTerrainFeaturePosition(_eTerrainFeatureType type, int x, int z)
    {
        (void)type;
        (void)x;
        (void)z;
    }
    void FatalLoadError() {}
    void GetSaveThumbnail(PBYTE* data, DWORD* size)
    {
        if (data != nullptr)
        {
            *data = nullptr;
        }
        if (size != nullptr)
        {
            *size = 0;
        }
    }
    void GetSaveThumbnail(
        PBYTE* thumbnail,
        DWORD* thumbnailSize,
        PBYTE* saveImage,
        DWORD* saveImageSize)
    {
        GetSaveThumbnail(thumbnail, thumbnailSize);
        if (saveImage != nullptr)
        {
            *saveImage = nullptr;
        }
        if (saveImageSize != nullptr)
        {
            *saveImageSize = 0;
        }
    }
    void processSchematics(LevelChunk* chunk)
    {
        (void)chunk;
    }
    void SetAdditionalSkinBoxes(DWORD skinId, SKIN_BOX* boxes, DWORD count)
    {
        (void)skinId;
        (void)boxes;
        (void)count;
    }
    void SetAnimOverrideBitmask(DWORD skinId, unsigned int bitmask)
    {
        (void)skinId;
        (void)bitmask;
    }
    void SetGameHostOption(eGameHostOption option, int value)
    {
        (void)option;
        (void)value;
    }
    void SetUniqueMapName(char* uniqueMapName)
    {
        (void)uniqueMapName;
    }
    void SetXuiServerAction(int pad, eXuiServerAction action)
    {
        (void)pad;
        (void)action;
    }

    NativeDesktopDLCManagerStub m_dlcManager;
    std::vector<std::wstring> vSkinNames;
};

class NativeDesktopProfileManagerStub
{
public:
    int GetPrimaryPad() const { return 0; }
    bool GetXUID(int pad, PlayerUID* xuid, bool online) const
    {
        (void)pad;
        (void)online;
        if (xuid != nullptr)
        {
            *xuid = 0;
        }
        return true;
    }
};

class NativeDesktopStorageManagerStub
{
public:
    bool GetSaveDisabled() const { return false; }
    std::string GetMountedPath(const char* path) const
    {
        (void)path;
        return std::string();
    }
    std::wstring GetMountedPath(const WCHAR* path) const
    {
        (void)path;
        return std::wstring();
    }
};

inline NativeDesktopAppStub app;
inline NativeDesktopProfileManagerStub ProfileManager;
inline NativeDesktopStorageManagerStub StorageManager;
