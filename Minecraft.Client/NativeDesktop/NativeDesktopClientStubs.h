#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include <lce_abi/lce_abi.h>

#include "../Common/NativeRendererTypes.h"
#include "GameConfig/Minecraft.spa.h"
#include "Sentient/SentientTelemetryCommon.h"
#include "TelemetryEnum.h"
#include "strings.h"

#ifndef DYNAMIC_CONFIG_DEFAULT_TRIAL_TIME
#define DYNAMIC_CONFIG_DEFAULT_TRIAL_TIME 2400
#endif

#ifndef XONLINE_S_LOGON_CONNECTION_ESTABLISHED
#define XONLINE_S_LOGON_CONNECTION_ESTABLISHED 0
#endif

class MinecraftDynamicConfigurations
{
public:
    static void Tick() {}
    static DWORD GetTrialTime()
    {
        return DYNAMIC_CONFIG_DEFAULT_TRIAL_TIME;
    }
};

#ifndef VK_SPACE
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_DELETE 0x2E
#define VK_HOME 0x24
#define VK_END 0x23
#define VK_SPACE 0x20
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F11 0x7A
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_ADD 0x6B
#define VK_SUBTRACT 0x6D
#endif

class KeyboardMouseInput
{
public:
    static const int MAX_KEYS = 256;
    static const int MOUSE_LEFT = 0;
    static const int MOUSE_RIGHT = 1;
    static const int MOUSE_MIDDLE = 2;
    static const int MAX_MOUSE_BUTTONS = 3;

    static const int KEY_FORWARD = 'W';
    static const int KEY_BACKWARD = 'S';
    static const int KEY_LEFT = 'A';
    static const int KEY_RIGHT = 'D';
    static const int KEY_JUMP = VK_SPACE;
    static const int KEY_SNEAK = VK_LSHIFT;
    static const int KEY_SPRINT = VK_CONTROL;
    static const int KEY_INVENTORY = 'E';
    static const int KEY_DROP = 'Q';
    static const int KEY_CRAFTING = 'C';
    static const int KEY_CRAFTING_ALT = 'R';
    static const int KEY_CHAT = 'T';
    static const int KEY_CONFIRM = VK_RETURN;
    static const int KEY_CANCEL = VK_ESCAPE;
    static const int KEY_PAUSE = VK_ESCAPE;
    static const int KEY_TOGGLE_HUD = VK_F1;
    static const int KEY_DEBUG_INFO = VK_F3;
    static const int KEY_DEBUG_MENU = VK_F4;
    static const int KEY_THIRD_PERSON = VK_F5;
    static const int KEY_DEBUG_CONSOLE = VK_F6;
    static const int KEY_HOST_SETTINGS = VK_TAB;
    static const int KEY_FULLSCREEN = VK_F11;
    static const int KEY_SCREENSHOT = VK_F2;

    void Init() { ClearAllState(); }
    void Tick() { EndFrame(); }
    void ClearAllState()
    {
        std::fill(std::begin(m_keyDown), std::end(m_keyDown), false);
        std::fill(std::begin(m_keyPressed), std::end(m_keyPressed), false);
        std::fill(std::begin(m_keyReleased), std::end(m_keyReleased), false);
        std::fill(
            std::begin(m_mouseButtonDown),
            std::end(m_mouseButtonDown),
            false);
        std::fill(
            std::begin(m_mouseButtonPressed),
            std::end(m_mouseButtonPressed),
            false);
        std::fill(
            std::begin(m_mouseButtonReleased),
            std::end(m_mouseButtonReleased),
            false);
        m_mouseX = 0;
        m_mouseY = 0;
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
        m_mouseWheel = 0;
        m_mouseWheelConsumed = false;
        m_pressedKey = 0;
        m_hasAnyInput = false;
        m_kbmActive = false;
        m_charBuffer.clear();
    }
    void EndFrame()
    {
        std::fill(std::begin(m_keyPressed), std::end(m_keyPressed), false);
        std::fill(std::begin(m_keyReleased), std::end(m_keyReleased), false);
        std::fill(
            std::begin(m_mouseButtonPressed),
            std::end(m_mouseButtonPressed),
            false);
        std::fill(
            std::begin(m_mouseButtonReleased),
            std::end(m_mouseButtonReleased),
            false);
        m_pressedKey = 0;
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
        m_mouseWheel = 0;
        m_mouseWheelConsumed = false;
    }

    void OnKeyDown(int vkCode)
    {
        int key = NormalizeKey(vkCode);
        if (key < 0)
        {
            return;
        }
        if (!m_keyDown[key])
        {
            m_keyPressed[key] = true;
            m_pressedKey = key;
        }
        m_keyDown[key] = true;
        MarkKeyboardMouseActive();
    }
    void OnKeyUp(int vkCode)
    {
        int key = NormalizeKey(vkCode);
        if (key < 0)
        {
            return;
        }
        if (m_keyDown[key])
        {
            m_keyReleased[key] = true;
        }
        m_keyDown[key] = false;
        MarkKeyboardMouseActive();
    }
    void OnMouseButtonDown(int button)
    {
        if (!IsValidMouseButton(button))
        {
            return;
        }
        if (!m_mouseButtonDown[button])
        {
            m_mouseButtonPressed[button] = true;
        }
        m_mouseButtonDown[button] = true;
        MarkKeyboardMouseActive();
    }
    void OnMouseButtonUp(int button)
    {
        if (!IsValidMouseButton(button))
        {
            return;
        }
        if (m_mouseButtonDown[button])
        {
            m_mouseButtonReleased[button] = true;
        }
        m_mouseButtonDown[button] = false;
        MarkKeyboardMouseActive();
    }
    void OnMouseMove(int x, int y)
    {
        m_mouseDeltaX += x - m_mouseX;
        m_mouseDeltaY += y - m_mouseY;
        m_mouseX = x;
        m_mouseY = y;
        MarkKeyboardMouseActive();
    }
    void OnMouseWheel(int delta)
    {
        if (delta == 0)
        {
            return;
        }
        m_mouseWheel += delta;
        m_mouseWheelConsumed = false;
        MarkKeyboardMouseActive();
    }
    void OnRawMouseDelta(int dx, int dy)
    {
        m_mouseDeltaX += dx;
        m_mouseDeltaY += dy;
        if (dx != 0 || dy != 0)
        {
            MarkKeyboardMouseActive();
        }
    }

    bool IsKeyDown(int vkCode) const
    {
        return ReadKeyState(m_keyDown, vkCode);
    }
    bool IsKeyPressed(int vkCode) const
    {
        return ReadKeyState(m_keyPressed, vkCode);
    }
    bool IsKeyReleased(int vkCode) const
    {
        return ReadKeyState(m_keyReleased, vkCode);
    }
    int GetPressedKey() const { return m_pressedKey; }

    bool IsMouseButtonDown(int button) const
    {
        return IsValidMouseButton(button) && m_mouseButtonDown[button];
    }
    bool IsMouseButtonPressed(int button) const
    {
        return IsValidMouseButton(button) && m_mouseButtonPressed[button];
    }
    bool IsMouseButtonReleased(int button) const
    {
        return IsValidMouseButton(button) && m_mouseButtonReleased[button];
    }

    int GetMouseX() const { return m_mouseX; }
    int GetMouseY() const { return m_mouseY; }
    int GetMouseDeltaX() const { return m_mouseDeltaX; }
    int GetMouseDeltaY() const { return m_mouseDeltaY; }

    int GetMouseWheel()
    {
        int wheel = m_mouseWheel;
        ConsumeMouseWheel();
        return wheel;
    }
    int PeekMouseWheel() const { return m_mouseWheel; }
    void ConsumeMouseWheel()
    {
        m_mouseWheel = 0;
        m_mouseWheelConsumed = true;
    }
    bool WasMouseWheelConsumed() const { return m_mouseWheelConsumed; }
    void ConsumeMouseDelta(float& dx, float& dy)
    {
        dx = static_cast<float>(m_mouseDeltaX);
        dy = static_cast<float>(m_mouseDeltaY);
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
    }

    void SetMouseGrabbed(bool grabbed) { m_mouseGrabbed = grabbed; }
    bool IsMouseGrabbed() const { return m_mouseGrabbed; }
    void SetCursorHiddenForUI(bool hidden) { m_cursorHiddenForUI = hidden; }
    bool IsCursorHiddenForUI() const { return m_cursorHiddenForUI; }
    void SetWindowFocused(bool focused) { m_windowFocused = focused; }
    bool IsWindowFocused() const { return m_windowFocused; }
    bool HasAnyInput() const { return m_hasAnyInput; }
    void SetKBMActive(bool active) { m_kbmActive = active; }
    bool IsKBMActive() const { return m_kbmActive; }
    void SetScreenCursorHidden(bool hidden) { m_screenWantsCursorHidden = hidden; }
    bool IsScreenCursorHidden() const { return m_screenWantsCursorHidden; }
    void OnChar(wchar_t c)
    {
        if (c == 0)
        {
            return;
        }
        if (m_charBuffer.size() >= kMaxCharBuffer)
        {
            m_charBuffer.erase(m_charBuffer.begin());
        }
        m_charBuffer.push_back(c);
        MarkKeyboardMouseActive();
    }
    bool ConsumeChar(wchar_t& outChar)
    {
        if (m_charBuffer.empty())
        {
            outChar = 0;
            return false;
        }
        outChar = m_charBuffer.front();
        m_charBuffer.erase(m_charBuffer.begin());
        return true;
    }
    void ClearCharBuffer() { m_charBuffer.clear(); }
    float GetMoveX() const
    {
        float move = 0.0f;
        if (IsKeyDown(KEY_RIGHT))
        {
            move += 1.0f;
        }
        if (IsKeyDown(KEY_LEFT))
        {
            move -= 1.0f;
        }
        return move;
    }
    float GetMoveY() const
    {
        float move = 0.0f;
        if (IsKeyDown(KEY_FORWARD))
        {
            move += 1.0f;
        }
        if (IsKeyDown(KEY_BACKWARD))
        {
            move -= 1.0f;
        }
        return move;
    }
    float GetLookX(float sensitivity) const
    {
        return static_cast<float>(m_mouseDeltaX) * sensitivity;
    }
    float GetLookY(float sensitivity) const
    {
        return static_cast<float>(m_mouseDeltaY) * sensitivity;
    }

private:
    static constexpr size_t kMaxCharBuffer = 64;

    static int NormalizeKey(int vkCode)
    {
        if (vkCode >= 'a' && vkCode <= 'z')
        {
            vkCode = vkCode - 'a' + 'A';
        }
        if (vkCode < 0 || vkCode >= MAX_KEYS)
        {
            return -1;
        }
        return vkCode;
    }
    static bool ReadKeyState(const bool (&states)[MAX_KEYS], int vkCode)
    {
        int key = NormalizeKey(vkCode);
        if (key < 0)
        {
            return false;
        }
        if (key == VK_SHIFT)
        {
            return states[VK_SHIFT] || states[VK_LSHIFT] || states[VK_RSHIFT];
        }
        if (key == VK_CONTROL)
        {
            return
                states[VK_CONTROL] ||
                states[VK_LCONTROL] ||
                states[VK_RCONTROL];
        }
        if (key == VK_MENU)
        {
            return states[VK_MENU] || states[VK_LMENU] || states[VK_RMENU];
        }
        return states[key];
    }
    static bool IsValidMouseButton(int button)
    {
        return button >= 0 && button < MAX_MOUSE_BUTTONS;
    }
    void MarkKeyboardMouseActive()
    {
        m_hasAnyInput = true;
        m_kbmActive = true;
    }

    bool m_keyDown[MAX_KEYS] = {};
    bool m_keyPressed[MAX_KEYS] = {};
    bool m_keyReleased[MAX_KEYS] = {};
    bool m_mouseButtonDown[MAX_MOUSE_BUTTONS] = {};
    bool m_mouseButtonPressed[MAX_MOUSE_BUTTONS] = {};
    bool m_mouseButtonReleased[MAX_MOUSE_BUTTONS] = {};
    std::vector<wchar_t> m_charBuffer;
    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_mouseDeltaX = 0;
    int m_mouseDeltaY = 0;
    int m_mouseWheel = 0;
    int m_pressedKey = 0;
    bool m_mouseWheelConsumed = false;
    bool m_hasAnyInput = false;
    bool m_mouseGrabbed = false;
    bool m_cursorHiddenForUI = false;
    bool m_windowFocused = true;
    bool m_kbmActive = false;
    bool m_screenWantsCursorHidden = false;
};

inline KeyboardMouseInput g_KBMInput;

inline NativeRendererSize NativeDesktopGetClientAreaSize()
{
    extern int g_iScreenWidth;
    extern int g_iScreenHeight;

    NativeRendererSize size;
    size.width = g_iScreenWidth > 0 ? g_iScreenWidth : 1280;
    size.height = g_iScreenHeight > 0 ? g_iScreenHeight : 720;
    return size;
}

#ifndef RADLINK
#define RADLINK
#endif

#ifndef RADEXPLINK
#define RADEXPLINK
#endif

#ifndef RADEXPFUNC
#define RADEXPFUNC
#endif

#ifndef MINECRAFT_NATIVE_DESKTOP_BOOLEAN_DEFINED
#define MINECRAFT_NATIVE_DESKTOP_BOOLEAN_DEFINED
typedef unsigned char boolean;
#endif

typedef int8_t S8;
typedef uint8_t U8;
typedef int16_t S16;
typedef uint16_t U16;
typedef int32_t S32;
typedef uint32_t U32;
typedef int64_t S64;
typedef uint64_t U64;
typedef float F32;
typedef double F64;
typedef intptr_t SINTa;
typedef uintptr_t UINTa;
typedef S32 rrbool;
typedef S32 RRBOOL;

typedef wchar_t IggyUTF16;
typedef UINTa IggyName;
typedef void* IggyValueRef;
typedef UINTa IggyTempRef;
typedef S32 IggyLibrary;
typedef void* IggyFocusHandle;
typedef void* HIGGYEXP;
typedef void* HIGGYPERFMON;

struct Iggy;
struct GDrawFunctions;
struct GDrawTexture;

struct IggyValuePath
{
    Iggy* f;
    IggyValuePath* parent;
    IggyName name;
    IggyValueRef ref;
    S32 index;
    S32 type;
    std::string nativeName;
    rrbool nativeVisible = true;
};

struct IggyStringUTF16
{
    IggyUTF16* string;
    S32 length;
};

struct IggyStringUTF8
{
    char* string;
    S32 length;
};

enum IggyResult
{
    IGGY_RESULT_SUCCESS = 0,
    IGGY_RESULT_Warning_None = 0,
    IGGY_RESULT_Warning_Misc = 100,
    IGGY_RESULT_Warning_GDraw = 101,
    IGGY_RESULT_Warning_ProgramFlow = 102,
    IGGY_RESULT_Warning_Actionscript = 103,
    IGGY_RESULT_Warning_Graphics = 104,
    IGGY_RESULT_Warning_Font = 105,
    IGGY_RESULT_Warning_Timeline = 106,
    IGGY_RESULT_Warning_Library = 107,
    IGGY_RESULT_Warning_ValuePath = 108,
    IGGY_RESULT_Warning_Audio = 109,
    IGGY_RESULT_Warning_CannotSustainFrameRate = 201,
    IGGY_RESULT_Warning_ThrewException = 202,
    IGGY_RESULT_Error_Threshhold = 400,
    IGGY_RESULT_Error_Misc = 400,
    IGGY_RESULT_Error_GDraw = 401,
    IGGY_RESULT_Error_ProgramFlow = 402,
    IGGY_RESULT_Error_Actionscript = 403,
    IGGY_RESULT_Error_Graphics = 404,
    IGGY_RESULT_Error_Font = 405,
    IGGY_RESULT_Error_Create = 406,
    IGGY_RESULT_Error_Library = 407,
    IGGY_RESULT_Error_ValuePath = 408,
    IGGY_RESULT_Error_Audio = 409,
    IGGY_RESULT_Error_Internal = 499,
    IGGY_RESULT_Error_InvalidIggy = 501,
    IGGY_RESULT_Error_InvalidArgument = 502,
    IGGY_RESULT_Error_InvalidEntity = 503,
    IGGY_RESULT_Error_UndefinedEntity = 504,
    IGGY_RESULT_Error_OutOfMemory = 1001
};

enum IggyDatatype
{
    IGGY_DATATYPE__invalid_request,
    IGGY_DATATYPE_undefined,
    IGGY_DATATYPE_null,
    IGGY_DATATYPE_boolean,
    IGGY_DATATYPE_number,
    IGGY_DATATYPE_string_UTF8,
    IGGY_DATATYPE_string_UTF16,
    IGGY_DATATYPE_fastname,
    IGGY_DATATYPE_valuepath,
    IGGY_DATATYPE_valueref
};

struct IggyDataValue
{
    S32 type = IGGY_DATATYPE_null;
    IggyTempRef temp_ref = 0;
    union
    {
        IggyStringUTF16 string16;
        IggyStringUTF8 string8;
        F64 number;
        rrbool boolval;
        IggyName fastname;
        void* userdata;
        IggyValuePath* valuepath;
        IggyValueRef valueref;
    };
};

struct IggyExternalFunctionCallUTF16
{
    IggyStringUTF16 function_name;
    S32 num_arguments;
    S32 padding;
    IggyDataValue arguments[1];
};

typedef void* RADLINK Iggy_AllocateFunction(
    void* allocCallbackUserData,
    size_t sizeRequested,
    size_t* sizeReturned);
typedef void RADLINK Iggy_DeallocateFunction(
    void* allocCallbackUserData,
    void* ptr);
typedef rrbool RADLINK Iggy_AS3ExternalFunctionUTF16(
    void* userCallbackData,
    Iggy* player,
    IggyExternalFunctionCallUTF16* call);
typedef void RADLINK Iggy_TraceFunctionUTF8(
    void* userCallbackData,
    Iggy* player,
    char const* utf8String,
    S32 lengthInBytes);
typedef void RADLINK Iggy_WarningFunction(
    void* userCallbackData,
    Iggy* player,
    IggyResult errorCode,
    char const* errorMessage);

struct IggyAllocator
{
    void* user_callback_data = nullptr;
    Iggy_AllocateFunction* mem_alloc = nullptr;
    Iggy_DeallocateFunction* mem_free = nullptr;
};

struct IggyPlayerGCSizes
{
    S32 total_storage_in_bytes = 0;
    S32 stack_size_in_bytes = 0;
    S32 young_heap_size_in_bytes = 0;
    S32 old_heap_size_in_bytes = 0;
    S32 remembered_set_size_in_bytes = 0;
    S32 greylist_size_in_bytes = 0;
    S32 rootstack_size_in_bytes = 0;
    S32 padding = 0;
};

struct IggyPlayerConfig
{
    IggyAllocator allocator;
    IggyPlayerGCSizes gc;
    char* filename = nullptr;
    char* user_name = nullptr;
    rrbool load_in_place = 0;
    rrbool did_load_in_place = 0;
};

struct IggyProperties
{
    S32 movie_width_in_pixels = 1280;
    S32 movie_height_in_pixels = 720;
    F32 movie_frame_rate_current_in_fps = 60.0f;
    F32 movie_frame_rate_from_file_in_fps = 60.0f;
    S32 frames_passed = 0;
    S32 swf_major_version_number = 9;
    F64 time_passed_in_seconds = 0.0;
    F64 seconds_since_last_tick = 0.0;
    F64 seconds_per_drawn_frame = 0.0;
};

struct Iggy
{
    IggyProperties properties;
    IggyValuePath root = {};
    void* userdata = nullptr;
    bool ready_to_tick = true;
};

struct IggyFontMetrics
{
    F32 ascent = 0.0f;
    F32 descent = 0.0f;
    F32 line_gap = 0.0f;
    F32 average_glyph_width_for_tab_stops = 0.0f;
    F32 largest_glyph_bbox_y1 = 0.0f;
};

struct IggyGlyphMetrics
{
    F32 x0 = 0.0f;
    F32 y0 = 0.0f;
    F32 x1 = 0.0f;
    F32 y1 = 0.0f;
    F32 advance = 0.0f;
};

struct IggyBitmapCharacter
{
    U8* pixels_one_per_byte = nullptr;
    S32 width_in_pixels = 0;
    S32 height_in_pixels = 0;
    S32 stride_in_bytes = 0;
    S32 oversample = 0;
    rrbool point_sample = 0;
    S32 top_left_x = 0;
    S32 top_left_y = 0;
    F32 pixel_scale_correct = 1.0f;
    F32 pixel_scale_min = 0.0f;
    F32 pixel_scale_max = 1.0f;
    void* user_context_for_free = nullptr;
};

typedef IggyFontMetrics* RADLINK IggyFontGetFontMetrics(
    void* userContext,
    IggyFontMetrics* metrics);
typedef S32 RADLINK IggyFontGetCodepointGlyph(
    void* userContext,
    U32 codepoint);
typedef IggyGlyphMetrics* RADLINK IggyFontGetGlyphMetrics(
    void* userContext,
    S32 glyph,
    IggyGlyphMetrics* metrics);
typedef rrbool RADLINK IggyFontIsGlyphEmpty(void* userContext, S32 glyph);
typedef F32 RADLINK IggyFontGetKerningForGlyphPair(
    void* userContext,
    S32 firstGlyph,
    S32 secondGlyph);
typedef rrbool RADLINK IggyBitmapFontCanProvideBitmap(
    void* userContext,
    S32 glyph,
    F32 pixelScale);
typedef rrbool RADLINK IggyBitmapFontGetGlyphBitmap(
    void* userContext,
    S32 glyph,
    F32 pixelScale,
    IggyBitmapCharacter* bitmap);
typedef void RADLINK IggyBitmapFontFreeGlyphBitmap(
    void* userContext,
    S32 glyph,
    F32 pixelScale,
    IggyBitmapCharacter* bitmap);

struct IggyBitmapFontProvider
{
    IggyFontGetFontMetrics* get_font_metrics = nullptr;
    IggyFontGetCodepointGlyph* get_glyph_for_codepoint = nullptr;
    IggyFontGetGlyphMetrics* get_glyph_metrics = nullptr;
    IggyFontIsGlyphEmpty* is_empty = nullptr;
    IggyFontGetKerningForGlyphPair* get_kerning = nullptr;
    IggyBitmapFontCanProvideBitmap* can_bitmap = nullptr;
    IggyBitmapFontGetGlyphBitmap* get_bitmap = nullptr;
    IggyBitmapFontFreeGlyphBitmap* free_bitmap = nullptr;
    S32 num_glyphs = 0;
    void* userdata = nullptr;
};

struct IggyCustomDrawCallbackRegion
{
    IggyUTF16* name = nullptr;
    F32 x0 = 0.0f;
    F32 y0 = 0.0f;
    F32 x1 = 0.0f;
    F32 y1 = 0.0f;
    F32 rgba_mul[4] = {};
    F32 rgba_add[4] = {};
    S32 scissor_x0 = 0;
    S32 scissor_y0 = 0;
    S32 scissor_x1 = 0;
    S32 scissor_y1 = 0;
    U8 scissor_enable = 0;
    U8 stencil_func_mask = 0;
    U8 stencil_func_ref = 0;
    U8 stencil_write_mask = 0;
    void* o2w = nullptr;
};

struct IggyEvent
{
    S32 type = 0;
    U32 flags = 0;
    S32 x = 0;
    S32 y = 0;
    S32 keycode = 0;
    S32 keyloc = 0;
};

struct IggyEventResult
{
    U32 new_flags = 0;
    S32 focus_change = 0;
    S32 focus_direction = 0;
};

struct IggyFocusableObject
{
    IggyFocusHandle object = nullptr;
    F32 x0 = 0.0f;
    F32 y0 = 0.0f;
    F32 x1 = 0.0f;
    F32 y1 = 0.0f;
};

struct IggyMemoryUseInfo
{
    char* subcategory = nullptr;
    S32 subcategory_stringlen = 0;
    S32 static_allocation_count = 0;
    S32 static_allocation_bytes = 0;
    S32 dynamic_allocation_count = 0;
    S32 dynamic_allocation_bytes = 0;
};

enum IggyKeyevent
{
    IGGY_KEYEVENT_Up = 9,
    IGGY_KEYEVENT_Down = 10
};

enum IggyKeyloc
{
    IGGY_KEYLOC_Standard = 0,
    IGGY_KEYLOC_Left = 1,
    IGGY_KEYLOC_Right = 2,
    IGGY_KEYLOC_Numpad = 3
};

enum IggyKeycode
{
    IGGY_KEYCODE_ENTER = 13,
    IGGY_KEYCODE_ESCAPE = 27,
    IGGY_KEYCODE_PAGE_UP = 33,
    IGGY_KEYCODE_PAGE_DOWN = 34,
    IGGY_KEYCODE_LEFT = 37,
    IGGY_KEYCODE_UP = 38,
    IGGY_KEYCODE_RIGHT = 39,
    IGGY_KEYCODE_DOWN = 40,
    IGGY_KEYCODE_F1 = 112,
    IGGY_KEYCODE_F2 = 113,
    IGGY_KEYCODE_F3 = 114,
    IGGY_KEYCODE_F4 = 115,
    IGGY_KEYCODE_F5 = 116,
    IGGY_KEYCODE_F6 = 117,
    IGGY_KEYCODE_F11 = 122,
    IGGY_KEYCODE_F12 = 123
};

const IggyLibrary IGGY_INVALID_LIBRARY = -1;
const S32 IGGY_GLYPH_INVALID = -1;
const U32 IGGY_FONTFLAG_none = 0;
const U32 IGGY_FONTFLAG_all = ~0U;
const S32 IGGY_TTC_INDEX_none = 0;
const IggyFocusHandle IGGY_FOCUS_NULL = nullptr;
const S32 IGGYEXP_MIN_STORAGE = 1024;

inline void IggyInit(IggyAllocator* allocator) { (void)allocator; }
inline void IggyInstallPerfmon(void* perfmonContext)
{
    (void)perfmonContext;
}
inline void* IggyPerfmonCreate(void* allocator, S32 flags)
{
    (void)allocator;
    (void)flags;
    return nullptr;
}
inline void IggyUseExplorer(Iggy* swf, void* context)
{
    (void)swf;
    (void)context;
}
inline void* IggyExpCreate(void* allocator, S32 flags)
{
    (void)allocator;
    (void)flags;
    return nullptr;
}
inline Iggy* IggyPlayerCreateFromMemory(
    void const* data,
    U32 dataSizeInBytes,
    IggyPlayerConfig* config)
{
    (void)data;
    (void)dataSizeInBytes;
    (void)config;
    Iggy* player = new Iggy();
    player->root.f = player;
    return player;
}
inline int& NativeDesktopIggyLibraryCreateCount()
{
    static int createCount = 0;
    return createCount;
}
inline int NativeDesktopGetIggyLibraryCreateCount()
{
    return NativeDesktopIggyLibraryCreateCount();
}
inline IggyLibrary IggyLibraryCreateFromMemoryUTF16(
    IggyUTF16 const* url,
    void const* data,
    U32 dataSizeInBytes,
    IggyPlayerConfig* config)
{
    (void)url;
    (void)data;
    (void)dataSizeInBytes;
    (void)config;
    int& createCount = NativeDesktopIggyLibraryCreateCount();
    createCount += 1;
    return createCount;
}
inline void IggyPlayerDestroy(Iggy* player) { delete player; }
inline void IggyLibraryDestroy(IggyLibrary library) { (void)library; }
inline void IggySetWarningCallback(Iggy_WarningFunction* func, void* data)
{
    (void)func;
    (void)data;
}
inline void IggySetTraceCallbackUTF8(Iggy_TraceFunctionUTF8* func, void* data)
{
    (void)func;
    (void)data;
}
inline void IggySetCustomDrawCallback(
    void (*func)(void*, Iggy*, IggyCustomDrawCallbackRegion*),
    void* data)
{
    (void)func;
    (void)data;
}
inline void IggySetTextureSubstitutionCallbacks(
    GDrawTexture* (*createFunc)(void*, IggyUTF16*, S32*, S32*, void**),
    void (*destroyFunc)(void*, void*, GDrawTexture*),
    void* data)
{
    (void)createFunc;
    (void)destroyFunc;
    (void)data;
}
inline void gdraw_D3D11_setViewport_4J() {}
inline IggyProperties* IggyPlayerProperties(Iggy* player)
{
    static IggyProperties properties;
    if (player == nullptr)
    {
        return &properties;
    }
    return &player->properties;
}
inline void* IggyPlayerGetUserdata(Iggy* player)
{
    return player == nullptr ? nullptr : player->userdata;
}
inline void IggyPlayerSetUserdata(Iggy* player, void* userdata)
{
    if (player != nullptr)
    {
        player->userdata = userdata;
    }
}
inline void IggyPlayerInitializeAndTickRS(Iggy* player)
{
    if (player != nullptr)
    {
        ++player->properties.frames_passed;
        player->ready_to_tick = false;
    }
}
inline rrbool IggyPlayerReadyToTick(Iggy* player)
{
    return player != nullptr && player->ready_to_tick;
}
inline void IggyPlayerTickRS(Iggy* player)
{
    if (player != nullptr)
    {
        ++player->properties.frames_passed;
        player->ready_to_tick = false;
    }
}
inline void IggyPlayerSetDisplaySize(Iggy* player, S32 w, S32 h)
{
    if (player != nullptr)
    {
        player->properties.movie_width_in_pixels = w;
        player->properties.movie_height_in_pixels = h;
    }
}
inline void IggyPlayerDraw(Iggy* player) { (void)player; }
inline void IggyPlayerDrawTile(Iggy* player, S32 x0, S32 y0, S32 x1, S32 y1,
                               S32 padding)
{
    (void)player;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)padding;
}
inline void IggyPlayerDrawTilesStart(Iggy* player) { (void)player; }
inline void IggyPlayerDrawTilesEnd(Iggy* player) { (void)player; }
inline void IggyFlushInstalledFonts() {}
inline void IggySetFontCachingCalculationBuffer(
    S32 maxChars,
    void* tempBuffer,
    S32 tempBufferSize)
{
    (void)maxChars;
    (void)tempBuffer;
    (void)tempBufferSize;
}
inline void IggyFontInstallTruetypeUTF8(
    const void* storage,
    S32 ttcIndex,
    const char* fontname,
    S32 nameLen,
    U32 flags)
{
    (void)storage;
    (void)ttcIndex;
    (void)fontname;
    (void)nameLen;
    (void)flags;
}
inline void IggyFontInstallTruetypeFallbackCodepointUTF8(
    const char* fontname,
    S32 len,
    U32 flags,
    S32 fallbackCodepoint)
{
    (void)fontname;
    (void)len;
    (void)flags;
    (void)fallbackCodepoint;
}
inline void IggyFontInstallBitmapUTF8(
    const IggyBitmapFontProvider* font,
    const char* fontname,
    S32 nameLen,
    U32 flags)
{
    (void)font;
    (void)fontname;
    (void)nameLen;
    (void)flags;
}
inline void IggyFontRemoveUTF8(const char* fontname, S32 nameLen, U32 flags)
{
    (void)fontname;
    (void)nameLen;
    (void)flags;
}
inline void IggyFontSetIndirectUTF8(
    const char* requestName,
    S32 requestLen,
    U32 requestFlags,
    const char* resultName,
    S32 resultLen,
    U32 resultFlags)
{
    (void)requestName;
    (void)requestLen;
    (void)requestFlags;
    (void)resultName;
    (void)resultLen;
    (void)resultFlags;
}
inline void IggySetAS3ExternalFunctionCallbackUTF16(
    Iggy_AS3ExternalFunctionUTF16* func,
    void* data)
{
    (void)func;
    (void)data;
}
inline IggyName IggyPlayerCreateFastName(
    Iggy* player,
    IggyUTF16 const* name,
    S32 len)
{
    (void)player;
    if (name == nullptr)
    {
        return 0;
    }

    IggyName hash = 1469598103934665603ULL;
    S32 count = len;
    if (count < 0)
    {
        count = 0;
        while (name[count] != 0)
        {
            ++count;
        }
    }

    for (S32 i = 0; i < count; ++i)
    {
        hash ^= static_cast<IggyName>(name[i]);
        hash *= 1099511628211ULL;
    }

    return hash == 0 ? 1 : hash;
}

inline IggyName NativeDesktopHashIggyNameUtf8(char const* name)
{
    if (name == nullptr)
    {
        return 0;
    }

    IggyName hash = 1469598103934665603ULL;
    while (*name != '\0')
    {
        hash ^= static_cast<IggyName>(
            static_cast<unsigned char>(*name));
        hash *= 1099511628211ULL;
        ++name;
    }

    return hash == 0 ? 1 : hash;
}

inline bool NativeDesktopIggyMetricMatches(IggyName name, char const* metric)
{
    return name == NativeDesktopHashIggyNameUtf8(metric);
}

inline void NativeDesktopGetSyntheticIggyControlRect(
    std::string const& name,
    F64* x,
    F64* y,
    F64* width,
    F64* height)
{
    *x = 0.0;
    *y = 0.0;
    *width = 0.0;
    *height = 0.0;

    if (name == "MainPanel" || name == "BackgroundPanel")
    {
        *width = 1280.0;
        *height = 720.0;
        return;
    }

    if (name == "CraftingHSlots")
    {
        *x = 384.0;
        *y = 230.0;
        *width = 512.0;
        *height = 48.0;
        return;
    }

    if (name == "CraftingOutput")
    {
        *x = 884.0;
        *y = 340.0;
        *width = 48.0;
        *height = 48.0;
        return;
    }

    if (name == "IngredientsLayout")
    {
        *x = 690.0;
        *y = 318.0;
        *width = 160.0;
        *height = 160.0;
        return;
    }

    if (name == "inventoryList")
    {
        *x = 390.0;
        *y = 450.0;
        *width = 500.0;
        *height = 150.0;
        return;
    }

    if (name == "hotbarList")
    {
        *x = 390.0;
        *y = 610.0;
        *width = 500.0;
        *height = 48.0;
        return;
    }

    if (name == "cursor")
    {
        *width = 48.0;
        *height = 48.0;
        return;
    }
}

inline IggyResult IggyPlayerCallMethodRS(
    Iggy* player,
    IggyDataValue* result,
    IggyValuePath* target,
    IggyName method,
    S32 numArgs,
    IggyDataValue* args)
{
    (void)player;
    (void)target;
    (void)method;
    (void)numArgs;
    (void)args;
    if (result != nullptr)
    {
        result->type = IGGY_DATATYPE_null;
        result->number = 0.0;
    }
    return IGGY_RESULT_SUCCESS;
}
inline IggyValuePath* IggyPlayerRootPath(Iggy* player)
{
    static IggyValuePath root = {};
    if (player != nullptr)
    {
        player->root.f = player;
        return &player->root;
    }
    root.f = player;
    return &root;
}
inline rrbool IggyValuePathMakeNameRef(
    IggyValuePath* result,
    IggyValuePath* parent,
    char const* text)
{
    if (result != nullptr)
    {
        result->f = parent == nullptr ? nullptr : parent->f;
        result->parent = parent;
        result->name = NativeDesktopHashIggyNameUtf8(text);
        result->nativeName = text == nullptr ? "" : text;
        result->nativeVisible = true;
    }
    return true;
}
inline IggyResult IggyValueGetF64RS(
    IggyValuePath* var,
    IggyName subName,
    char const* subNameUtf8,
    F64* result)
{
    (void)subNameUtf8;
    if (result != nullptr)
    {
        *result = 0.0;
        if (var != nullptr)
        {
            F64 x;
            F64 y;
            F64 width;
            F64 height;
            NativeDesktopGetSyntheticIggyControlRect(
                var->nativeName,
                &x,
                &y,
                &width,
                &height);

            if (NativeDesktopIggyMetricMatches(subName, "x"))
            {
                *result = x;
            }
            else if (NativeDesktopIggyMetricMatches(subName, "y"))
            {
                *result = y;
            }
            else if (NativeDesktopIggyMetricMatches(subName, "width"))
            {
                *result = width;
            }
            else if (NativeDesktopIggyMetricMatches(subName, "height"))
            {
                *result = height;
            }
        }
    }
    return IGGY_RESULT_SUCCESS;
}
inline IggyResult IggyValueGetBooleanRS(
    IggyValuePath* var,
    IggyName subName,
    char const* subNameUtf8,
    rrbool* result)
{
    (void)subNameUtf8;
    if (result != nullptr)
    {
        if (var != nullptr && NativeDesktopIggyMetricMatches(subName, "visible"))
        {
            *result = var->nativeVisible;
        }
        else
        {
            *result = false;
        }
    }
    return IGGY_RESULT_SUCCESS;
}
inline rrbool IggyValueSetBooleanRS(
    IggyValuePath* var,
    IggyName subName,
    char const* subNameUtf8,
    rrbool value)
{
    (void)subNameUtf8;
    if (var != nullptr && NativeDesktopIggyMetricMatches(subName, "visible"))
    {
        var->nativeVisible = value;
    }
    return true;
}
inline void IggyMakeEventKey(
    IggyEvent* event,
    IggyKeyevent eventType,
    IggyKeycode keycode,
    IggyKeyloc keyloc)
{
    if (event != nullptr)
    {
        event->type = eventType;
        event->keycode = keycode;
        event->keyloc = keyloc;
    }
}
inline void IggyMakeEventMouseMove(IggyEvent* event, S32 x, S32 y)
{
    if (event != nullptr)
    {
        event->x = x;
        event->y = y;
    }
}
inline void IggyMakeEventFocusLost(IggyEvent* event)
{
    if (event != nullptr)
    {
        event->type = 17;
    }
}
inline void IggyMakeEventFocusGained(IggyEvent* event, S32 focusDirection)
{
    if (event != nullptr)
    {
        event->type = 0;
        event->keyloc = focusDirection;
    }
}
inline rrbool IggyPlayerDispatchEventRS(
    Iggy* player,
    IggyEvent* event,
    IggyEventResult* result)
{
    (void)player;
    (void)event;
    if (result != nullptr)
    {
        *result = {};
    }
    return true;
}
inline rrbool IggyPlayerGetFocusableObjects(
    Iggy* player,
    IggyFocusHandle* currentFocus,
    IggyFocusableObject* objects,
    S32 maxObjects,
    S32* numObjects)
{
    (void)player;
    (void)objects;
    (void)maxObjects;
    if (currentFocus != nullptr)
    {
        *currentFocus = nullptr;
    }
    if (numObjects != nullptr)
    {
        *numObjects = 0;
    }
    return true;
}
inline void IggyPlayerSetFocusRS(
    Iggy* player,
    IggyFocusHandle object,
    int focusKeyChar)
{
    (void)player;
    (void)object;
    (void)focusKeyChar;
}
inline rrbool IggyDebugGetMemoryUseInfo(
    Iggy* player,
    IggyLibrary library,
    char const* categoryString,
    S32 categoryStringLen,
    S32 iteration,
    IggyMemoryUseInfo* data)
{
    (void)player;
    (void)library;
    (void)categoryString;
    (void)categoryStringLen;
    (void)iteration;
    if (data != nullptr)
    {
        *data = {};
    }
    return false;
}
inline void IggyPerfmonPadFromXInputStatePointer(void* pad, void* state)
{
    (void)pad;
    (void)state;
}
inline void IggyPerfmonTickAndDraw(void* perfmon, void* pad)
{
    (void)perfmon;
    (void)pad;
}

#ifndef LCE_XSOCIAL_TYPES_DEFINED
#define LCE_XSOCIAL_TYPES_DEFINED
class XSOCIAL_IMAGEPOSTPARAMS
{
};

class XSOCIAL_LINKPOSTPARAMS
{
};
#endif

#ifndef MAX_DISPLAYNAME_LENGTH
#define MAX_DISPLAYNAME_LENGTH 128
#endif

#ifndef MAX_DETAILS_LENGTH
#define MAX_DETAILS_LENGTH 128
#endif

#ifndef MAX_SAVEFILENAME_LENGTH
#define MAX_SAVEFILENAME_LENGTH 32
#endif

#ifndef USER_INDEX_ANY
#define USER_INDEX_ANY 0x000000FF
#endif

#ifndef CURRENT_DLC_VERSION_NUM
#define CURRENT_DLC_VERSION_NUM 3
#endif

#ifndef MAX_CREDIT_STRINGS
#define MAX_CREDIT_STRINGS (XBOXONE_CREDITS_COUNT + MILES_AND_IGGY_CREDITS_COUNT)
#endif

class ImageFileBuffer
{
public:
    enum EImageType
    {
        e_typePNG,
        e_typeJPG
    };

    int GetType() const { return m_type; }
    void* GetBufferPointer() { return m_pBuffer; }
    int GetBufferSize() const { return m_bufferSize; }
    void Release()
    {
        std::free(m_pBuffer);
        m_pBuffer = nullptr;
        m_bufferSize = 0;
    }
    bool Allocated() const { return m_pBuffer != nullptr; }

    EImageType m_type = e_typePNG;
    void* m_pBuffer = nullptr;
    int m_bufferSize = 0;
};

typedef struct
{
    int Width;
    int Height;
} D3DXIMAGE_INFO;

typedef struct _XSOCIAL_PREVIEWIMAGE
{
    BYTE* pBytes;
    DWORD Pitch;
    DWORD Width;
    DWORD Height;
} XSOCIAL_PREVIEWIMAGE, *PXSOCIAL_PREVIEWIMAGE;

typedef void ID3D11Texture2D;
typedef void ID3D11ShaderResourceView;
typedef void ID3D11RenderTargetView;
typedef void ID3D11DepthStencilView;
typedef void ID3D11VertexShader;
typedef void ID3D11PixelShader;
typedef void ID3D11Buffer;
typedef void ID3D11SamplerState;
typedef void ID3D11RasterizerState;
typedef void ID3D11DepthStencilState;
typedef void ID3D11BlendState;
typedef void ID3D11Device;
typedef void ID3D11DeviceContext;
typedef void ID3D11Resource;

using D3D11_VIEWPORT = NativeRendererViewport;

struct STRING_VERIFY_RESPONSE
{
    WORD wNumStrings;
    HRESULT* pStringResult;
    BOOL bFailed;
};

using D3D11_RECT = NativeRendererRect;

const int MAP_STYLE_0 = 0;
const int MAP_STYLE_1 = 1;
const int MAP_STYLE_2 = 2;
const int MAP_STYLE_3 = 3;

const unsigned int _360_JOY_BUTTON_A = 0x00000001;
const unsigned int _360_JOY_BUTTON_B = 0x00000002;
const unsigned int _360_JOY_BUTTON_X = 0x00000004;
const unsigned int _360_JOY_BUTTON_Y = 0x00000008;
const unsigned int _360_JOY_BUTTON_START = 0x00000010;
const unsigned int _360_JOY_BUTTON_BACK = 0x00000020;
const unsigned int _360_JOY_BUTTON_RB = 0x00000040;
const unsigned int _360_JOY_BUTTON_LB = 0x00000080;
const unsigned int _360_JOY_BUTTON_RTHUMB = 0x00000100;
const unsigned int _360_JOY_BUTTON_LTHUMB = 0x00000200;
const unsigned int _360_JOY_BUTTON_DPAD_UP = 0x00000400;
const unsigned int _360_JOY_BUTTON_DPAD_DOWN = 0x00000800;
const unsigned int _360_JOY_BUTTON_DPAD_LEFT = 0x00001000;
const unsigned int _360_JOY_BUTTON_DPAD_RIGHT = 0x00002000;
const unsigned int _360_JOY_BUTTON_LSTICK_RIGHT = 0x00004000;
const unsigned int _360_JOY_BUTTON_LSTICK_LEFT = 0x00008000;
const unsigned int _360_JOY_BUTTON_RSTICK_DOWN = 0x00010000;
const unsigned int _360_JOY_BUTTON_RSTICK_UP = 0x00020000;
const unsigned int _360_JOY_BUTTON_RSTICK_RIGHT = 0x00040000;
const unsigned int _360_JOY_BUTTON_RSTICK_LEFT = 0x00080000;
const unsigned int _360_JOY_BUTTON_LSTICK_DOWN = 0x00100000;
const unsigned int _360_JOY_BUTTON_LSTICK_UP = 0x00200000;
const unsigned int _360_JOY_BUTTON_RT = 0x00400000;
const unsigned int _360_JOY_BUTTON_LT = 0x00800000;

const int AXIS_MAP_LX = 0;
const int AXIS_MAP_LY = 1;
const int AXIS_MAP_RX = 2;
const int AXIS_MAP_RY = 3;
const int TRIGGER_MAP_0 = 0;
const int TRIGGER_MAP_1 = 1;

struct ScePadTouchData
{
};

class C_4JInput
{
public:
    enum EKeyboardResult
    {
        EKeyboard_Pending,
        EKeyboard_Cancelled,
        EKeyboard_ResultAccept,
        EKeyboard_ResultDecline
    };

    enum EKeyboardMode
    {
        EKeyboardMode_Default,
        EKeyboardMode_Numeric,
        EKeyboardMode_Password,
        EKeyboardMode_Alphabet,
        EKeyboardMode_Full,
        EKeyboardMode_Alphabet_Extended,
        EKeyboardMode_IP_Address,
        EKeyboardMode_Phone
    };

    void Initialise(int inputStates, unsigned char maps, unsigned char actions,
                    unsigned char menuActions)
    {
        (void)inputStates;
        (void)maps;
        (void)actions;
        (void)menuActions;
    }
    void Tick() {}
    void Resume() {}
    void SetDeadzoneAndMovementRange(unsigned int deadzone,
                                     unsigned int movementRangeMax)
    {
        (void)deadzone;
        (void)movementRangeMax;
    }
    void SetDeadzoneAndMovementRange(unsigned int deadzone,
                                     unsigned int digitalDeadzone,
                                     unsigned int movementRangeMax)
    {
        (void)digitalDeadzone;
        SetDeadzoneAndMovementRange(deadzone, movementRangeMax);
    }
    void SetGameJoypadMaps(unsigned char map, unsigned char action,
                           unsigned int value)
    {
        (void)map;
        (void)action;
        (void)value;
    }
    unsigned int GetGameJoypadMaps(unsigned char map, unsigned char action)
    {
        (void)map;
        (void)action;
        return 0;
    }
    void SetJoypadMapVal(int pad, unsigned char map)
    {
        (void)pad;
        m_map = map;
    }
    unsigned char GetJoypadMapVal(int pad)
    {
        (void)pad;
        return m_map;
    }
    void SetJoypadSensitivity(int pad, float sensitivity)
    {
        (void)pad;
        (void)sensitivity;
    }
    unsigned int GetValue(int pad, unsigned char action, bool repeat = false)
    {
        (void)pad;
        (void)action;
        (void)repeat;
        return 0;
    }
    bool ButtonPressed(int pad, unsigned char action = 255)
    {
        (void)pad;
        (void)action;
        return false;
    }
    bool ButtonReleased(int pad, unsigned char action)
    {
        (void)pad;
        (void)action;
        return false;
    }
    bool ButtonDown(int pad, unsigned char action = 255)
    {
        (void)pad;
        (void)action;
        return false;
    }
    void SetJoypadStickAxisMap(int pad, unsigned int from, unsigned int to)
    {
        (void)pad;
        (void)from;
        (void)to;
    }
    void SetJoypadStickTriggerMap(int pad, unsigned int from, unsigned int to)
    {
        (void)pad;
        (void)from;
        (void)to;
    }
    void SetKeyRepeatRate(float repeatDelaySecs, float repeatRateSecs)
    {
        (void)repeatDelaySecs;
        (void)repeatRateSecs;
    }
    void SetDebugSequence(const char* sequence, int (*func)(LPVOID),
                          LPVOID param)
    {
        (void)sequence;
        (void)func;
        (void)param;
    }
    FLOAT GetIdleSeconds(int pad)
    {
        (void)pad;
        return 0.0f;
    }
    bool IsPadConnected(int pad)
    {
        (void)pad;
        return true;
    }
    bool IsPadLocked(int pad)
    {
        (void)pad;
        return false;
    }
    void SetCircleCrossSwapped(bool swapped) { m_circleCrossSwapped = swapped; }
    bool IsCircleCrossSwapped() { return m_circleCrossSwapped; }
    float GetJoypadStick_LX(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return 0.0f;
    }
    float GetJoypadStick_LY(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return 0.0f;
    }
    float GetJoypadStick_RX(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return 0.0f;
    }
    float GetJoypadStick_RY(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return 0.0f;
    }
    unsigned char GetJoypadLTrigger(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return 0;
    }
    unsigned char GetJoypadRTrigger(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return 0;
    }
    ScePadTouchData* GetTouchPadData(int pad, bool checkMenuDisplay = true)
    {
        (void)pad;
        (void)checkMenuDisplay;
        return nullptr;
    }
    void SetMenuDisplayed(int pad, bool value)
    {
        (void)pad;
        (void)value;
    }
    EKeyboardResult RequestKeyboard(LPCWSTR title, LPCWSTR text, DWORD pad,
                                    UINT maxChars,
                                    int (*func)(LPVOID, const bool),
                                    LPVOID param, EKeyboardMode mode)
    {
        (void)title;
        (void)text;
        (void)pad;
        (void)maxChars;
        (void)mode;
        if (func != nullptr)
        {
            func(param, true);
        }
        return EKeyboard_ResultAccept;
    }
    int RequestMessageBox(...) { return 0; }
    void DestroyKeyboard() {}
    void GetText(uint16_t* value)
    {
        if (value != nullptr)
        {
            *value = 0;
        }
    }
    void SetLocalMultiplayer(bool value) { m_localMultiplayer = value; }
    bool IsLocalMultiplayerAvailable() { return true; }
    bool UsingRemoteVita() { return false; }
    bool IsVitaTV() { return false; }
    void MapTouchInput(bool value) { (void)value; }
    int GetConnectedGamepadCount() { return 1; }
    int GetUserForGamepad(int gamepad)
    {
        (void)gamepad;
        return 0;
    }
    bool InputDetected(DWORD userIndex, WCHAR* input = nullptr)
    {
        (void)userIndex;
        (void)input;
        return false;
    }
    void SetEnabledGtcButtons(bool enabled) { (void)enabled; }
    void SetLostInputControlCallback(void (*func)(LPVOID), LPVOID param)
    {
        (void)func;
        (void)param;
    }
    bool VerifyStrings(WCHAR** strings, int stringCount,
                       int (*func)(LPVOID, STRING_VERIFY_RESPONSE*),
                       LPVOID param)
    {
        (void)strings;
        STRING_VERIFY_RESPONSE response = {};
        response.wNumStrings = static_cast<WORD>(stringCount);
        response.pStringResult = nullptr;
        response.bFailed = FALSE;
        if (func != nullptr)
        {
            func(param, &response);
        }
        return true;
    }
    void CancelQueuedVerifyStrings(
        int (*func)(LPVOID, STRING_VERIFY_RESPONSE*),
        LPVOID param)
    {
        (void)func;
        (void)param;
    }
    void CancelAllVerifyInProgress() {}

private:
    unsigned char m_map = MAP_STYLE_0;
    bool m_circleCrossSwapped = false;
    bool m_localMultiplayer = false;
};

inline C_4JInput InputManager;

class C4JRender
{
public:
    enum eVertexType
    {
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1,
        VERTEX_TYPE_COMPRESSED,
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT,
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN,
        VERTEX_TYPE_PS3_TS2_CS1,
        VERTEX_TYPE_COUNT
    };

    enum ePixelShaderType
    {
        PIXEL_SHADER_TYPE_STANDARD,
        PIXEL_SHADER_TYPE_PROJECTION,
        PIXEL_SHADER_TYPE_FORCELOD,
        PIXEL_SHADER_COUNT
    };

    enum eViewportType
    {
        VIEWPORT_TYPE_FULLSCREEN,
        VIEWPORT_TYPE_SPLIT_TOP,
        VIEWPORT_TYPE_SPLIT_BOTTOM,
        VIEWPORT_TYPE_SPLIT_LEFT,
        VIEWPORT_TYPE_SPLIT_RIGHT,
        VIEWPORT_TYPE_QUADRANT_TOP_LEFT,
        VIEWPORT_TYPE_QUADRANT_TOP_RIGHT,
        VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT,
        VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT
    };

    enum ePrimitiveType
    {
        PRIMITIVE_TYPE_TRIANGLE_LIST,
        PRIMITIVE_TYPE_TRIANGLE_STRIP,
        PRIMITIVE_TYPE_TRIANGLE_FAN,
        PRIMITIVE_TYPE_QUAD_LIST,
        PRIMITIVE_TYPE_LINE_LIST,
        PRIMITIVE_TYPE_LINE_STRIP,
        PRIMITIVE_TYPE_COUNT
    };

    enum eTextureFormat
    {
        TEXTURE_FORMAT_RxGyBzAw,
        TEXTURE_FORMAT_RxGyBzAw5551,
        TEXTURE_FORMAT_R0G0B0Ax,
        TEXTURE_FORMAT_R1G1B1Ax,
        TEXTURE_FORMAT_RxGxBxAx,
        MAX_TEXTURE_FORMATS
    };

    void Tick() {}
    void UpdateGamma(unsigned short usGamma) { (void)usGamma; }
    void MatrixMode(int type) { (void)type; }
    void MatrixSetIdentity() {}
    void MatrixTranslate(float x, float y, float z) { (void)x; (void)y; (void)z; }
    void MatrixRotate(float angle, float x, float y, float z)
    {
        (void)angle;
        (void)x;
        (void)y;
        (void)z;
    }
    void MatrixScale(float x, float y, float z) { (void)x; (void)y; (void)z; }
    void MatrixPerspective(float fovy, float aspect, float zNear, float zFar)
    {
        (void)fovy;
        (void)aspect;
        (void)zNear;
        (void)zFar;
    }
    void MatrixOrthogonal(float left, float right, float bottom, float top,
                          float zNear, float zFar)
    {
        (void)left;
        (void)right;
        (void)bottom;
        (void)top;
        (void)zNear;
        (void)zFar;
    }
    void MatrixPop() {}
    void MatrixPush() {}
    void MatrixMult(float* mat) { (void)mat; }
    const float* MatrixGet(int type)
    {
        (void)type;
        static float identity[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        return identity;
    }
    void Set_matrixDirty() {}
    void Initialise() {}
    void InitialiseContext() {}
    void StartFrame(bool actualFrameStart = true) { (void)actualFrameStart; }
    void DoScreenGrabOnNextPresent() {}
    void Present() {}
    void Clear(int flags) { (void)flags; }
    void Clear(int flags, D3D11_RECT* rect)
    {
        (void)flags;
        (void)rect;
    }
    void SetClearColour(const float colourRGBA[4]) { (void)colourRGBA; }
    bool IsWidescreen() const { return true; }
    bool IsHiDef() const { return true; }
    void InternalScreenCapture() {}
    void CaptureThumbnail(ImageFileBuffer* pngOut)
    {
        if (pngOut != nullptr)
        {
            pngOut->Release();
        }
    }
    void CaptureThumbnail(ImageFileBuffer* pngOut, ImageFileBuffer* saveGamePngOut)
    {
        CaptureThumbnail(pngOut);
        CaptureThumbnail(saveGamePngOut);
    }
    void CaptureScreen(ImageFileBuffer* jpgOut, XSOCIAL_PREVIEWIMAGE* previewOut)
    {
        CaptureThumbnail(jpgOut);
        (void)previewOut;
    }
    void BeginConditionalSurvey(int identifier) { (void)identifier; }
    void EndConditionalSurvey() {}
    void BeginConditionalRendering(int identifier) { (void)identifier; }
    void EndConditionalRendering() {}
    void DrawVertices(ePrimitiveType primitiveType, int count, void* dataIn,
                      eVertexType vertexType, ePixelShaderType pixelShaderType)
    {
        (void)primitiveType;
        (void)count;
        (void)dataIn;
        (void)vertexType;
        (void)pixelShaderType;
    }
    void DrawVerticesCutOut(ePrimitiveType primitiveType, int count, void* dataIn,
                            eVertexType vertexType, ePixelShaderType pixelShaderType)
    {
        DrawVertices(primitiveType, count, dataIn, vertexType, pixelShaderType);
    }
    void CBuffLockStaticCreations() {}
    int CBuffCreate(int count)
    {
        (void)count;
        return 0;
    }
    void CBuffDelete(int first, int count) { (void)first; (void)count; }
    void CBuffStart(int index, bool full = false) { (void)index; (void)full; }
    void CBuffClear(int index) { (void)index; }
    int CBuffSize(int index)
    {
        (void)index;
        return 0;
    }
    void CBuffEnd() {}
    bool CBuffCall(int index, bool full = true)
    {
        (void)index;
        (void)full;
        return true;
    }
    void CBuffTick() {}
    void CBuffDeferredModeStart() {}
    void CBuffDeferredModeEnd() {}
    int TextureCreate() { return ++m_nextTextureId; }
    void TextureFree(int idx) { (void)idx; }
    void TextureBind(int idx) { (void)idx; }
    void TextureBindVertex(int idx) { (void)idx; }
    void TextureSetTextureLevels(int levels) { m_textureLevels = levels; }
    int TextureGetTextureLevels() const { return m_textureLevels; }
    void TextureData(int width, int height, void* data, int level,
                     eTextureFormat format = TEXTURE_FORMAT_RxGyBzAw)
    {
        (void)width;
        (void)height;
        (void)data;
        (void)level;
        (void)format;
    }
    void TextureDataUpdate(int xoffset, int yoffset, int width, int height,
                           void* data, int level)
    {
        (void)xoffset;
        (void)yoffset;
        (void)width;
        (void)height;
        (void)data;
        (void)level;
    }
    void TextureSetParam(int param, int value) { (void)param; (void)value; }
    void TextureDynamicUpdateStart() {}
    void TextureDynamicUpdateEnd() {}

private:
    static DWORD ReadBigEndian32(const BYTE* bytes)
    {
        return (static_cast<DWORD>(bytes[0]) << 24) |
            (static_cast<DWORD>(bytes[1]) << 16) |
            (static_cast<DWORD>(bytes[2]) << 8) |
            static_cast<DWORD>(bytes[3]);
    }

    static bool TryReadPngInfo(
        const BYTE* data,
        DWORD size,
        D3DXIMAGE_INFO* info)
    {
        static const BYTE kPngSignature[8] =
            { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
        if (data == nullptr || size < 24 ||
            std::memcmp(data, kPngSignature, sizeof(kPngSignature)) != 0)
        {
            return false;
        }

        if (info != nullptr)
        {
            info->Width = static_cast<int>(ReadBigEndian32(data + 16));
            info->Height = static_cast<int>(ReadBigEndian32(data + 20));
        }
        return true;
    }

    static bool TryReadPngInfoFromFile(
        const char* filename,
        D3DXIMAGE_INFO* info)
    {
        if (filename == nullptr)
        {
            return false;
        }

        FILE* file = std::fopen(filename, "rb");
        if (file == nullptr)
        {
            return false;
        }

        BYTE header[24] = {};
        const size_t bytesRead = std::fread(header, 1, sizeof(header), file);
        std::fclose(file);
        return TryReadPngInfo(header, static_cast<DWORD>(bytesRead), info);
    }

    static void AllocateFallbackTexture(D3DXIMAGE_INFO* info, int** ppDataOut)
    {
        int width = 1;
        int height = 1;
        if (info != nullptr && info->Width > 0 && info->Height > 0)
        {
            width = info->Width;
            height = info->Height;
        }

        if (ppDataOut != nullptr)
        {
            int* pixels = new int[static_cast<size_t>(width) * height];
            for (int i = 0; i < width * height; ++i)
            {
                pixels[i] = 0xFFFFFFFF;
            }
            *ppDataOut = pixels;
        }
    }

public:
    HRESULT LoadTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo,
                            int** ppDataOut)
    {
        if (pSrcInfo != nullptr)
        {
            pSrcInfo->Width = 0;
            pSrcInfo->Height = 0;
        }
        if (ppDataOut != nullptr)
        {
            *ppDataOut = nullptr;
        }
        if (!TryReadPngInfoFromFile(szFilename, pSrcInfo))
        {
            return E_FAIL;
        }
        AllocateFallbackTexture(pSrcInfo, ppDataOut);
        return ERROR_SUCCESS;
    }
    HRESULT LoadTextureData(BYTE* pbData, DWORD dwBytes,
                            D3DXIMAGE_INFO* pSrcInfo, int** ppDataOut)
    {
        if (pSrcInfo != nullptr)
        {
            pSrcInfo->Width = 0;
            pSrcInfo->Height = 0;
        }
        if (ppDataOut != nullptr)
        {
            *ppDataOut = nullptr;
        }
        if (!TryReadPngInfo(pbData, dwBytes, pSrcInfo))
        {
            return E_FAIL;
        }
        AllocateFallbackTexture(pSrcInfo, ppDataOut);
        return ERROR_SUCCESS;
    }
    HRESULT SaveTextureData(const char* szFilename, D3DXIMAGE_INFO* pSrcInfo,
                            int* ppDataOut)
    {
        (void)szFilename;
        (void)pSrcInfo;
        (void)ppDataOut;
        return E_FAIL;
    }
    HRESULT SaveTextureDataToMemory(void* pOutput, int outputCapacity,
                                    int* outputLength, int width, int height,
                                    int* ppDataIn)
    {
        (void)pOutput;
        (void)outputCapacity;
        (void)width;
        (void)height;
        (void)ppDataIn;
        if (outputLength != nullptr)
        {
            *outputLength = 0;
        }
        return E_FAIL;
    }
    void TextureGetStats() {}
    void* TextureGetTexture(int idx)
    {
        (void)idx;
        return nullptr;
    }
    void StateSetColour(float r, float g, float b, float a)
    {
        (void)r;
        (void)g;
        (void)b;
        (void)a;
    }
    void StateSetDepthMask(bool enable) { (void)enable; }
    void StateSetBlendEnable(bool enable) { (void)enable; }
    void StateSetBlendFunc(int src, int dst) { (void)src; (void)dst; }
    void StateSetBlendFactor(unsigned int colour) { (void)colour; }
    void StateSetAlphaFunc(int func, float param) { (void)func; (void)param; }
    void StateSetDepthFunc(int func) { (void)func; }
    void StateSetFaceCull(bool enable) { (void)enable; }
    void StateSetFaceCullCW(bool enable) { (void)enable; }
    void StateSetLineWidth(float width) { (void)width; }
    void StateSetWriteEnable(bool red, bool green, bool blue, bool alpha)
    {
        (void)red;
        (void)green;
        (void)blue;
        (void)alpha;
    }
    void StateSetDepthTestEnable(bool enable) { (void)enable; }
    void StateSetAlphaTestEnable(bool enable) { (void)enable; }
    void StateSetDepthSlopeAndBias(float slope, float bias)
    {
        (void)slope;
        (void)bias;
    }
    void StateSetFogEnable(bool enable) { (void)enable; }
    void StateSetFogMode(int mode) { (void)mode; }
    void StateSetFogNearDistance(float dist) { (void)dist; }
    void StateSetFogFarDistance(float dist) { (void)dist; }
    void StateSetFogDensity(float density) { (void)density; }
    void StateSetFogColour(float red, float green, float blue)
    {
        (void)red;
        (void)green;
        (void)blue;
    }
    void StateSetLightingEnable(bool enable) { (void)enable; }
    void StateSetVertexTextureUV(float u, float v) { (void)u; (void)v; }
    void StateSetLightColour(int light, float red, float green, float blue)
    {
        (void)light;
        (void)red;
        (void)green;
        (void)blue;
    }
    void StateSetLightAmbientColour(float red, float green, float blue)
    {
        (void)red;
        (void)green;
        (void)blue;
    }
    void StateSetLightDirection(int light, float x, float y, float z)
    {
        (void)light;
        (void)x;
        (void)y;
        (void)z;
    }
    void StateSetLightEnable(int light, bool enable)
    {
        (void)light;
        (void)enable;
    }
    void StateSetViewport(eViewportType viewportType) { (void)viewportType; }
    void StateSetEnableViewportClipPlanes(bool enable) { (void)enable; }
    void StateSetTexGenCol(int col, float x, float y, float z, float w,
                           bool eyeSpace)
    {
        (void)col;
        (void)x;
        (void)y;
        (void)z;
        (void)w;
        (void)eyeSpace;
    }
    void StateSetStencil(int function, uint8_t stencilRef,
                         uint8_t stencilFuncMask, uint8_t stencilWriteMask)
    {
        (void)function;
        (void)stencilRef;
        (void)stencilFuncMask;
        (void)stencilWriteMask;
    }
    void StateSetForceLOD(int lod) { (void)lod; }
    void BeginEvent(LPCWSTR eventName) { (void)eventName; }
    void EndEvent() {}
    void Suspend() {}
    bool Suspended() const { return false; }
    void Resume() {}

private:
    int m_nextTextureId = 0;
    int m_textureLevels = 1;
};

inline C4JRender RenderManager;

const int GL_MODELVIEW_MATRIX = 0;
const int GL_PROJECTION_MATRIX = 1;
const int GL_MODELVIEW = 0;
const int GL_PROJECTION = 1;
const int GL_TEXTURE = 2;
const int GL_S = 0;
const int GL_T = 1;
const int GL_R = 2;
const int GL_Q = 3;
const int GL_TEXTURE_GEN_S = 10;
const int GL_TEXTURE_GEN_T = 11;
const int GL_TEXTURE_GEN_Q = 12;
const int GL_TEXTURE_GEN_R = 13;
const int GL_TEXTURE_GEN_MODE = 0;
const int GL_OBJECT_LINEAR = 0;
const int GL_EYE_LINEAR = 1;
const int GL_OBJECT_PLANE = 0;
const int GL_EYE_PLANE = 1;
const int GL_TEXTURE_2D = 1;
const int GL_BLEND = 2;
const int GL_CULL_FACE = 3;
const int GL_ALPHA_TEST = 4;
const int GL_DEPTH_TEST = 5;
const int GL_FOG = 6;
const int GL_LIGHTING = 7;
const int GL_LIGHT0 = 8;
const int GL_LIGHT1 = 9;
const int CLEAR_DEPTH_FLAG = 1;
const int CLEAR_COLOUR_FLAG = 2;
const int GL_DEPTH_BUFFER_BIT = CLEAR_DEPTH_FLAG;
const int GL_COLOR_BUFFER_BIT = CLEAR_COLOUR_FLAG;
const int GL_SRC_ALPHA = 1;
const int GL_ONE_MINUS_SRC_ALPHA = 2;
const int GL_ONE = 3;
const int GL_ZERO = 4;
const int GL_DST_ALPHA = 5;
const int GL_SRC_COLOR = 6;
const int GL_DST_COLOR = 7;
const int GL_ONE_MINUS_DST_COLOR = 8;
const int GL_ONE_MINUS_SRC_COLOR = 9;
const int GL_CONSTANT_ALPHA = 10;
const int GL_ONE_MINUS_CONSTANT_ALPHA = 11;
const int GL_GREATER = 1;
const int GL_EQUAL = 2;
const int GL_LEQUAL = 3;
const int GL_GEQUAL = 4;
const int GL_ALWAYS = 5;
const int GL_TEXTURE_MIN_FILTER = 1;
const int GL_TEXTURE_MAG_FILTER = 2;
const int GL_TEXTURE_WRAP_S = 3;
const int GL_TEXTURE_WRAP_T = 4;
const int GL_NEAREST = 0;
const int GL_LINEAR = 1;
const int GL_EXP = 2;
const int GL_NEAREST_MIPMAP_LINEAR = 0;
const int GL_CLAMP = 0;
const int GL_REPEAT = 1;
const int GL_FOG_START = 1;
const int GL_FOG_END = 2;
const int GL_FOG_MODE = 3;
const int GL_FOG_DENSITY = 4;
const int GL_FOG_COLOR = 5;
const int GL_POSITION = 1;
const int GL_AMBIENT = 2;
const int GL_DIFFUSE = 3;
const int GL_SPECULAR = 4;
const int GL_LIGHT_MODEL_AMBIENT = 1;
const int GL_LINES = C4JRender::PRIMITIVE_TYPE_LINE_LIST;
const int GL_LINE_STRIP = C4JRender::PRIMITIVE_TYPE_LINE_STRIP;
const int GL_QUADS = C4JRender::PRIMITIVE_TYPE_QUAD_LIST;
const int GL_TRIANGLE_FAN = C4JRender::PRIMITIVE_TYPE_TRIANGLE_FAN;
const int GL_TRIANGLE_STRIP = C4JRender::PRIMITIVE_TYPE_TRIANGLE_STRIP;

typedef struct
{
    char UTF8SaveFilename[MAX_SAVEFILENAME_LENGTH];
    char UTF8SaveTitle[MAX_DISPLAYNAME_LENGTH];
    wchar_t UTF16SaveFilename[MAX_SAVEFILENAME_LENGTH];
    wchar_t UTF16SaveTitle[MAX_DISPLAYNAME_LENGTH];
    std::time_t modifiedTime;
    PBYTE thumbnailData;
    unsigned int thumbnailSize;
    int blocksUsed;
} SAVE_INFO, *PSAVE_INFO;

typedef struct
{
    int totalBlocksUsed;
    int newSaveBlocksUsed;
    int iSaveC;
    PSAVE_INFO SaveInfoA;
    PSAVE_INFO pCurrentSaveInfo;
} SAVE_DETAILS, *PSAVE_DETAILS;

typedef std::vector<PXMARKETPLACE_CONTENTOFFER_INFO> OfferDataArray;
typedef std::vector<PXCONTENT_DATA> XContentDataArray;
typedef XMARKETPLACE_CONTENTOFFER_INFO MARKETPLACE_CONTENTOFFER_INFO;
typedef PXMARKETPLACE_CONTENTOFFER_INFO PMARKETPLACE_CONTENTOFFER_INFO;

enum eAwardType
{
    eAwardType_Achievement = 0,
    eAwardType_GamerPic,
    eAwardType_Theme,
    eAwardType_AvatarItem
};

enum eUpsellType
{
    eUpsellType_Custom = 0,
    eUpsellType_Achievement,
    eUpsellType_GamerPic,
    eUpsellType_Theme,
    eUpsellType_AvatarItem
};

enum eUpsellResponse
{
    eUpsellResponse_Declined,
    eUpsellResponse_Accepted_NoPurchase,
    eUpsellResponse_Accepted_Purchase,
    eUpsellResponse_UserNotSignedInPSN
};

class C_4JProfile
{
public:
    struct PROFILESETTINGS
    {
        int iYAxisInversion;
        int iControllerSensitivity;
        int iVibration;
        bool bSwapSticks;
    };

    void Initialise(DWORD titleId, DWORD offerId, unsigned short version,
                    UINT profileValues, UINT profileSettings, DWORD* settings,
                    int gameDefinedDataSize,
                    unsigned int* changedBitmask)
    {
        (void)titleId;
        (void)offerId;
        (void)version;
        (void)profileValues;
        (void)profileSettings;
        (void)settings;
        if (gameDefinedDataSize <= 0)
        {
            gameDefinedDataSize =
                kFallbackGameDefinedDataBytesPerUser * XUSER_MAX_COUNT;
        }

        m_gameDefinedData.assign(
            static_cast<std::size_t>(gameDefinedDataSize),
            0);
        m_gameDefinedDataBytesPerUser = gameDefinedDataSize / XUSER_MAX_COUNT;

        if (changedBitmask != nullptr)
        {
            *changedBitmask = 0;
        }
    }
    void SetTrialTextStringTable(void* stringTable, int accept, int reject)
    {
        (void)stringTable;
        (void)accept;
        (void)reject;
    }
    void SetTrialAwardText(eAwardType type, int title, int text)
    {
        (void)type;
        (void)title;
        (void)text;
    }
    int GetLockedProfile() const { return -1; }
    void SetLockedProfile(int profile) { (void)profile; }
    bool IsSignedIn(int quadrant) const
    {
        (void)quadrant;
        return true;
    }
    bool IsSignedInLive(int profile) const
    {
        (void)profile;
        return true;
    }
    bool IsGuest(int quadrant) const
    {
        (void)quadrant;
        return false;
    }
    UINT RequestSignInUI(bool fromInvite, bool localGame, bool noGuestsAllowed,
                         bool multiplayerSignIn, bool addUser,
                         int (*func)(LPVOID, const bool, const int),
                         LPVOID lpParam, int quadrant = XUSER_INDEX_ANY)
    {
        (void)fromInvite;
        (void)localGame;
        (void)noGuestsAllowed;
        (void)multiplayerSignIn;
        (void)addUser;
        if (func != nullptr)
        {
            func(lpParam, true, quadrant == XUSER_INDEX_ANY ? 0 : quadrant);
        }
        return 0;
    }
    UINT DisplayOfflineProfile(int (*func)(LPVOID, const bool, const int),
                               LPVOID lpParam,
                               int quadrant = XUSER_INDEX_ANY)
    {
        return RequestSignInUI(false, true, false, false, false, func, lpParam,
                               quadrant);
    }
    UINT RequestConvertOfflineToGuestUI(
        int (*func)(LPVOID, const bool, const int),
        LPVOID lpParam,
        int quadrant = XUSER_INDEX_ANY)
    {
        return DisplayOfflineProfile(func, lpParam, quadrant);
    }
    void SetPrimaryPlayerChanged(bool value) { (void)value; }
    bool QuerySigninStatus() const { return true; }
    void GetXUID(int pad, PlayerUID* xuid, bool online)
    {
        (void)pad;
        (void)online;
        if (xuid != nullptr)
        {
            *xuid = 0;
        }
    }
    BOOL AreXUIDSEqual(PlayerUID xuid1, PlayerUID xuid2)
    {
        return xuid1 == xuid2;
    }
    BOOL XUIDIsGuest(PlayerUID xuid)
    {
        (void)xuid;
        return FALSE;
    }
    bool AllowedToPlayMultiplayer(int profile) const
    {
        (void)profile;
        return true;
    }
    bool GetChatAndContentRestrictions(int pad, bool* chatRestricted,
                                       bool* contentRestricted, int* age)
    {
        (void)pad;
        if (chatRestricted != nullptr)
        {
            *chatRestricted = false;
        }
        if (contentRestricted != nullptr)
        {
            *contentRestricted = false;
        }
        if (age != nullptr)
        {
            *age = 99;
        }
        return true;
    }
    void StartTrialGame() {}
    void AllowedPlayerCreatedContent(int pad, bool thisQuadrantOnly,
                                     BOOL* allAllowed, BOOL* friendsAllowed)
    {
        (void)pad;
        (void)thisQuadrantOnly;
        if (allAllowed != nullptr)
        {
            *allAllowed = TRUE;
        }
        if (friendsAllowed != nullptr)
        {
            *friendsAllowed = TRUE;
        }
    }
    BOOL CanViewPlayerCreatedContent(int pad, bool thisQuadrantOnly,
                                     PPlayerUID xuids, DWORD xuidCount)
    {
        (void)pad;
        (void)thisQuadrantOnly;
        (void)xuids;
        (void)xuidCount;
        return TRUE;
    }
    void ShowProfileCard(int pad, PlayerUID targetUid)
    {
        (void)pad;
        (void)targetUid;
    }
    bool GetProfileAvatar(int pad, int (*func)(LPVOID, PBYTE, DWORD),
                          LPVOID lpParam)
    {
        (void)pad;
        if (func != nullptr)
        {
            func(lpParam, nullptr, 0);
        }
        return false;
    }
    void CancelProfileAvatarRequest() {}
    int GetPrimaryPad() const { return 0; }
    void SetPrimaryPad(int pad) { (void)pad; }
    char* GetGamertag(int pad)
    {
        (void)pad;
        return m_gamertag;
    }
    std::wstring GetDisplayName(int pad)
    {
        (void)pad;
        return L"Player";
    }
    bool IsFullVersion() const { return true; }
    void SetSignInChangeCallback(void (*func)(LPVOID, bool, unsigned int),
                                 LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
    }
    void SetNotificationsCallback(void (*func)(LPVOID, DWORD, unsigned int),
                                  LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
    }
    bool RegionIsNorthAmerica() const { return true; }
    bool LocaleIsUSorCanada() const { return true; }
    HRESULT GetLiveConnectionStatus() const { return S_OK; }
    bool IsSystemUIDisplayed() const { return false; }
    void SetProfileReadErrorCallback(void (*func)(LPVOID), LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
    }
    int SetDefaultOptionsCallback(
        int (*func)(LPVOID, PROFILESETTINGS*, const int),
        LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
        return 0;
    }
    int SetOldProfileVersionCallback(
        int (*func)(LPVOID, unsigned char*, const unsigned short, const int),
        LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
        return 0;
    }
    PROFILESETTINGS* GetDashboardProfileSettings(int pad)
    {
        (void)pad;
        return &m_settings;
    }
    void WriteToProfile(int quadrant, bool changed = false,
                        bool overrideLimit = false)
    {
        (void)quadrant;
        (void)changed;
        (void)overrideLimit;
    }
    void ForceQueuedProfileWrites(int pad = XUSER_INDEX_ANY) { (void)pad; }
    void* GetGameDefinedProfileData(int quadrant)
    {
        if (quadrant < 0 || quadrant >= XUSER_MAX_COUNT)
        {
            return nullptr;
        }

        EnsureGameDefinedData();
        const std::size_t offset = static_cast<std::size_t>(
            quadrant * m_gameDefinedDataBytesPerUser);
        if (offset >= m_gameDefinedData.size())
        {
            return nullptr;
        }
        return m_gameDefinedData.data() + offset;
    }
    void ResetProfileProcessState() {}
    void Tick() {}
    void RegisterAward(int awardNumber, int gamerconfigId, eAwardType type,
                       bool leaderboardAffected = false,
                       void* stringTable = nullptr, int title = -1,
                       int text = -1, int accept = -1,
                       char* themeName = nullptr, unsigned int themeSize = 0)
    {
        (void)awardNumber;
        (void)gamerconfigId;
        (void)type;
        (void)leaderboardAffected;
        (void)stringTable;
        (void)title;
        (void)text;
        (void)accept;
        (void)themeName;
        (void)themeSize;
    }
    int GetAwardId(int awardNumber)
    {
        return awardNumber;
    }
    eAwardType GetAwardType(int awardNumber)
    {
        (void)awardNumber;
        return eAwardType_Achievement;
    }
    bool CanBeAwarded(int quadrant, int awardNumber)
    {
        (void)quadrant;
        (void)awardNumber;
        return false;
    }
    void Award(int quadrant, int awardNumber, bool force = false)
    {
        (void)quadrant;
        (void)awardNumber;
        (void)force;
    }
    bool IsAwardsFlagSet(int quadrant, int award)
    {
        (void)quadrant;
        (void)award;
        return false;
    }
    void RichPresenceInit(int presenceCount, int contextCount)
    {
        (void)presenceCount;
        (void)contextCount;
    }
    void RegisterRichPresenceContext(int contextId) { (void)contextId; }
    void SetRichPresenceContextValue(int pad, int contextId, int value)
    {
        (void)pad;
        (void)contextId;
        (void)value;
    }
    void SetCurrentGameActivity(int pad, int presence, bool setOthersToIdle = false)
    {
        (void)pad;
        (void)presence;
        (void)setOthersToIdle;
    }
    void DisplayFullVersionPurchase(bool required, int quadrant,
                                    int upsellParam = -1)
    {
        (void)required;
        (void)quadrant;
        (void)upsellParam;
    }
    void SetUpsellCallback(
        void (*func)(LPVOID, eUpsellType, eUpsellResponse, int),
        LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
    }
    void SetDebugFullOverride(bool value) { (void)value; }

private:
    void EnsureGameDefinedData()
    {
        if (!m_gameDefinedData.empty())
        {
            return;
        }

        m_gameDefinedDataBytesPerUser = kFallbackGameDefinedDataBytesPerUser;
        m_gameDefinedData.assign(
            static_cast<std::size_t>(
                kFallbackGameDefinedDataBytesPerUser * XUSER_MAX_COUNT),
            0);
    }

    static constexpr int kFallbackGameDefinedDataBytesPerUser = 972;

    std::vector<unsigned char> m_gameDefinedData;
    int m_gameDefinedDataBytesPerUser = kFallbackGameDefinedDataBytesPerUser;
    PROFILESETTINGS m_settings = {};
    char m_gamertag[XUSER_NAME_SIZE] = "Player";
};

inline C_4JProfile ProfileManager;

typedef struct
{
    char chDLCKeyname[17];
    int eDLCType;
    char chDLCPicname[17];
    int iFirstSkin;
    int iConfig;
} SONYDLC;

class C4JStorage
{
public:
    struct PROFILESETTINGS
    {
        int iYAxisInversion;
        int iControllerSensitivity;
        int iVibration;
        bool bSwapSticks;
    };

    typedef struct
    {
        unsigned int uiFileSize;
        DWORD dwType;
        DWORD dwWchCount;
        WCHAR wchFile[1];
    } DLC_FILE_DETAILS, *PDLC_FILE_DETAILS;

    typedef struct
    {
        DWORD dwType;
        DWORD dwWchCount;
        WCHAR wchData[1];
    } DLC_FILE_PARAM, *PDLC_FILE_PARAM;

    typedef struct
    {
        DWORD dwVersion;
        DWORD dwNewOffers;
        DWORD dwTotalOffers;
        DWORD dwInstalledTotalOffers;
        BYTE bPadding[1024 - sizeof(DWORD) * 4];
    } DLC_TMS_DETAILS;

    typedef struct
    {
        DWORD dwSize;
        PBYTE pbData;
    } TMSPP_FILEDATA, *PTMSPP_FILEDATA;

    typedef struct
    {
        CHAR szFilename[256];
        int iFileSize;
        int eFileTypeVal;
    } TMSPP_FILE_DETAILS, *PTMSPP_FILE_DETAILS;

    typedef struct
    {
        int iCount;
        PTMSPP_FILE_DETAILS FileDetailsA;
    } TMSPP_FILE_LIST, *PTMSPP_FILE_LIST;

    enum eTMS_FILETYPEVAL
    {
        TMS_FILETYPE_BINARY = 0,
        TMS_FILETYPE_CONFIG = 1,
        TMS_FILETYPE_JSON = 2,
        TMS_FILETYPE_MAX
    };

    enum eTMS_UGCTYPE
    {
        TMS_UGCTYPE_NONE,
        TMS_UGCTYPE_IMAGE,
        TMS_UGCTYPE_MAX
    };

    enum eGlobalStorage
    {
        eGlobalStorage_Title = 0,
        eGlobalStorage_TitleUser,
        eGlobalStorage_Max
    };

    enum EMessageResult
    {
        EMessage_Undefined = 0,
        EMessage_Busy,
        EMessage_Pending,
        EMessage_Cancelled,
        EMessage_ResultAccept,
        EMessage_ResultDecline,
        EMessage_ResultThirdOption,
        EMessage_ResultFourthOption
    };

    enum ESaveGameState
    {
        ESaveGame_Idle = 0,
        ESaveGame_Save,
        ESaveGame_SaveCompleteSuccess,
        ESaveGame_SaveCompleteFail,
        ESaveGame_SaveIncomplete,
        ESaveGame_SaveIncomplete_WaitingOnResponse,
        ESaveGame_SaveSubfiles,
        ESaveGame_SaveSubfilesCompleteSuccess,
        ESaveGame_SaveSubfilesCompleteFail,
        ESaveGame_SaveSubfilesIncomplete,
        ESaveGame_SaveSubfilesIncomplete_WaitingOnResponse,
        ESaveGame_Load,
        ESaveGame_LoadCompleteSuccess,
        ESaveGame_LoadCompleteFail,
        ESaveGame_Delete,
        ESaveGame_DeleteSuccess,
        ESaveGame_DeleteFail,
        ESaveGame_Rename,
        ESaveGame_RenameSuccess,
        ESaveGame_RenameFail,
        ESaveGame_Copy,
        ESaveGame_CopyCompleteSuccess,
        ESaveGame_CopyCompleteFail,
        ESaveGame_CopyCompleteFailQuota,
        ESaveGame_CopyCompleteFailLocalStorage,
        ESaveGame_GetSaveThumbnail
    };

    typedef ESaveGameState EDeleteGameStatus;
    static const EDeleteGameStatus EDeleteGame_Idle = ESaveGame_Idle;
    static const EDeleteGameStatus EDeleteGame_InProgress = ESaveGame_Delete;

    enum EOptionsState
    {
        EOptions_Idle = 0,
        EOptions_Save,
        EOptions_Load,
        EOptions_Delete,
        EOptions_NoSpace,
        EOptions_Corrupt
    };

    enum EDLCStatus
    {
        EDLC_Error = 0,
        EDLC_Idle,
        EDLC_NoOffers,
        EDLC_AlreadyEnumeratedAllOffers,
        EDLC_NoInstalledDLC,
        EDLC_Pending,
        EDLC_LoadInProgress,
        EDLC_Loaded,
        EDLC_ChangedDevice
    };

    enum ESavingMessage
    {
        ESavingMessage_None = 0,
        ESavingMessage_Short,
        ESavingMessage_Long
    };

    enum ESaveIncompleteType
    {
        ESaveIncomplete_None,
        ESaveIncomplete_OutOfQuota,
        ESaveIncomplete_OutOfLocalStorage,
        ESaveIncomplete_Unknown
    };

    enum ETMSStatus
    {
        ETMSStatus_Idle = 0,
        ETMSStatus_Fail,
        ETMSStatus_ReadInProgress,
        ETMSStatus_ReadFileListInProgress,
        ETMSStatus_WriteInProgress,
        ETMSStatus_Fail_ReadInProgress,
        ETMSStatus_Fail_ReadFileListInProgress,
        ETMSStatus_Fail_ReadDetailsNotRetrieved,
        ETMSStatus_Fail_WriteInProgress,
        ETMSStatus_DeleteInProgress,
        ETMSStatus_Pending
    };

    enum eTMS_FileType
    {
        eTMS_FileType_Normal = 0,
        eTMS_FileType_Graphic
    };

    enum ESGIStatus
    {
        ESGIStatus_Error = 0,
        ESGIStatus_Idle,
        ESGIStatus_ReadInProgress,
        ESGIStatus_NoSaves
    };

    enum eOptionsCallback
    {
        eOptions_Callback_Idle,
        eOptions_Callback_Write,
        eOptions_Callback_Write_Fail_NoSpace,
        eOptions_Callback_Write_Fail,
        eOptions_Callback_Read,
        eOptions_Callback_Read_Fail,
        eOptions_Callback_Read_FileNotFound,
        eOptions_Callback_Read_Corrupt,
        eOptions_Callback_Read_CorruptDeletePending,
        eOptions_Callback_Read_CorruptDeleted
    };

    enum eSaveTransferState
    {
        eSaveTransfer_Idle,
        eSaveTransfer_Busy,
        eSaveTransfer_FileSizeRetrieved,
        eSaveTransfer_GettingFileData,
        eSaveTransfer_FileDataRetrieved,
        eSaveTransfer_Saving,
        eSaveTransfer_Converting
    };

    typedef struct
    {
        DWORD dwSize;
        PBYTE pbData;
    } SAVETRANSFER_FILE_DETAILS;

    void Tick() {}
    void Init(unsigned int uiSaveVersion, LPCWSTR pwchDefaultSaveName,
              char* pszSavePackName, int iMinimumSaveSize,
              int (*func)(LPVOID, const ESavingMessage, int), LPVOID lpParam,
              LPCSTR szGroupID)
    {
        (void)uiSaveVersion;
        (void)pwchDefaultSaveName;
        (void)pszSavePackName;
        (void)iMinimumSaveSize;
        (void)func;
        (void)lpParam;
        (void)szGroupID;
    }
    void SetGameSaveFolderTitle(WCHAR* value) { (void)value; }
    void SetSaveCacheFolderTitle(WCHAR* value) { (void)value; }
    void SetOptionsFolderTitle(WCHAR* value) { (void)value; }
    void SetCorruptSaveName(WCHAR* value) { (void)value; }
    void SetGameSaveFolderPrefix(char* value) { (void)value; }
    void SetMaxSaves(int value) { (void)value; }
    void SetDefaultImages(PBYTE optionsImage, DWORD optionsImageBytes,
                          PBYTE saveImage, DWORD saveImageBytes,
                          PBYTE saveThumbnail, DWORD saveThumbnailBytes)
    {
        (void)optionsImage;
        (void)optionsImageBytes;
        (void)saveImage;
        (void)saveImageBytes;
        (void)saveThumbnail;
        (void)saveThumbnailBytes;
    }
    void SetIncompleteSaveCallback(
        void (*func)(LPVOID, const ESaveIncompleteType, int), LPVOID param)
    {
        (void)func;
        (void)param;
    }
    void SetSaveDisabled(bool disabled) { m_saveDisabled = disabled; }
    bool GetSaveDisabled() const { return m_saveDisabled; }
    void ResetSaveData()
    {
        m_saveData.clear();
        m_saveState = ESaveGame_Idle;
    }
    ESaveGameState DoesSaveExist(bool* exists)
    {
        if (exists != nullptr)
        {
            *exists = false;
        }
        return ESaveGame_Idle;
    }
    bool EnoughSpaceForAMinSaveGame() const { return true; }
    ESaveGameState GetSaveState() const { return m_saveState; }
    ESaveGameState GetSavesInfo(int iPad,
                                int (*func)(LPVOID, SAVE_DETAILS*, const bool),
                                LPVOID lpParam, char* pszSavePackName)
    {
        (void)iPad;
        (void)pszSavePackName;
        if (func != nullptr)
        {
            func(lpParam, &m_saveDetails, true);
        }
        return ESaveGame_Idle;
    }
    PSAVE_DETAILS ReturnSavesInfo() { return &m_saveDetails; }
    void ClearSavesInfo() {}
    ESaveGameState LoadSaveDataThumbnail(
        PSAVE_INFO pSaveInfo,
        int (*func)(LPVOID, PBYTE, DWORD),
        LPVOID lpParam,
        bool force = false)
    {
        (void)pSaveInfo;
        (void)force;
        if (func != nullptr)
        {
            func(lpParam, nullptr, 0);
        }
        return ESaveGame_GetSaveThumbnail;
    }
    ESaveGameState LoadSaveData(
        PSAVE_INFO pSaveInfo,
        int (*func)(LPVOID, const bool, const bool),
        LPVOID lpParam,
        bool ignoreCRC = false)
    {
        (void)pSaveInfo;
        (void)ignoreCRC;
        if (func != nullptr)
        {
            func(lpParam, true, false);
        }
        return ESaveGame_LoadCompleteSuccess;
    }
    unsigned int GetSaveSize() const
    {
        return static_cast<unsigned int>(m_saveData.size());
    }
    void GetSaveData(void* data, unsigned int* bytes)
    {
        if (bytes != nullptr)
        {
            *bytes = static_cast<unsigned int>(m_saveData.size());
        }
        if (data != nullptr && !m_saveData.empty())
        {
            std::memcpy(data, m_saveData.data(), m_saveData.size());
        }
    }
    bool GetSaveUniqueNumber(INT* value)
    {
        if (value != nullptr)
        {
            *value = 0;
        }
        return true;
    }
    bool GetSaveUniqueFilename(char* name)
    {
        if (name != nullptr)
        {
            strcpy_s(name, MAX_SAVEFILENAME_LENGTH, "native");
        }
        return true;
    }
    void SetSaveUniqueFilename(char* name) { (void)name; }
    bool GetSaveUniqueFileDir(char* name) { return GetSaveUniqueFilename(name); }
    bool SaveGameDirMountExisting(char* mountPoint)
    {
        (void)mountPoint;
        return false;
    }
    bool SaveGameDirUnMount(const char* mountPoint)
    {
        (void)mountPoint;
        return true;
    }
    unsigned int GetSubfileCount() const { return 0; }
    void ResetSubfiles() {}
    void GetSubfileDetails(int idx, unsigned int* subfileId,
                           unsigned char** data, unsigned int* sizeOut)
    {
        (void)idx;
        if (subfileId != nullptr)
        {
            *subfileId = 0;
        }
        if (data != nullptr)
        {
            *data = nullptr;
        }
        if (sizeOut != nullptr)
        {
            *sizeOut = 0;
        }
    }
    void UpdateSubfile(int idx, unsigned char* data, unsigned int size)
    {
        (void)idx;
        (void)data;
        (void)size;
    }
    int AddSubfile(unsigned int subfileId)
    {
        (void)subfileId;
        return 0;
    }
    ESaveGameState SaveSubfiles(int (*func)(LPVOID, const bool),
                                LPVOID lpParam)
    {
        if (func != nullptr)
        {
            func(lpParam, true);
        }
        return ESaveGame_SaveSubfilesCompleteSuccess;
    }
    void SetSaveTitle(const wchar_t* value) { (void)value; }
    void SetSaveTitleExtraFileSuffix(const wchar_t* value) { (void)value; }
    PVOID AllocateSaveData(unsigned int bytes)
    {
        m_saveData.assign(bytes, 0);
        return m_saveData.data();
    }
    void SetSaveDataSize(unsigned int bytes) { m_saveData.resize(bytes); }
    void GetDefaultSaveImage(PBYTE* image, DWORD* bytes)
    {
        setNullBuffer(image, bytes);
    }
    void GetDefaultSaveThumbnail(PBYTE* thumbnail, DWORD* bytes)
    {
        setNullBuffer(thumbnail, bytes);
    }
    void SetSaveImages(PBYTE thumbnail, DWORD thumbnailBytes, PBYTE image,
                       DWORD imageBytes, PBYTE textData, DWORD textDataBytes)
    {
        (void)thumbnail;
        (void)thumbnailBytes;
        (void)image;
        (void)imageBytes;
        (void)textData;
        (void)textDataBytes;
    }
    ESaveGameState SaveSaveData(int (*func)(LPVOID, const bool), LPVOID lpParam)
    {
        if (func != nullptr)
        {
            func(lpParam, true);
        }
        return ESaveGame_SaveCompleteSuccess;
    }
    void ContinueIncompleteOperation() {}
    void CancelIncompleteOperation() {}
    ESaveGameState DeleteSaveData(PSAVE_INFO pSaveInfo,
                                  int (*func)(LPVOID, const bool),
                                  LPVOID lpParam)
    {
        (void)pSaveInfo;
        if (func != nullptr)
        {
            func(lpParam, true);
        }
        return ESaveGame_DeleteSuccess;
    }
    ESaveGameState CopySaveData(PSAVE_INFO pSaveInfo,
                                int (*func)(LPVOID, const bool,
                                            ESaveGameState),
                                bool (*progress)(LPVOID, const int),
                                LPVOID lpParam)
    {
        (void)pSaveInfo;
        if (progress != nullptr)
        {
            progress(lpParam, 100);
        }
        if (func != nullptr)
        {
            func(lpParam, true, ESaveGame_CopyCompleteSuccess);
        }
        return ESaveGame_CopyCompleteSuccess;
    }
    ESaveGameState RenameSaveData(int renameIndex, uint16_t* newName,
                                  int (*func)(LPVOID, const bool),
                                  LPVOID lpParam)
    {
        (void)renameIndex;
        (void)newName;
        if (func != nullptr)
        {
            func(lpParam, true);
        }
        return ESaveGame_RenameSuccess;
    }
    void InitialiseProfileData(unsigned short version, UINT profileValues,
                               UINT profileSettings, DWORD* settings,
                               int gameDefinedDataSize,
                               unsigned int* changedBitmask)
    {
        (void)version;
        (void)profileValues;
        (void)profileSettings;
        (void)settings;
        (void)gameDefinedDataSize;
        (void)changedBitmask;
    }
    int SetDefaultOptionsCallback(
        int (*func)(LPVOID, PROFILESETTINGS*, const int), LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
        return 0;
    }
    void SetOptionsDataCallback(
        int (*func)(LPVOID, int, unsigned short, eOptionsCallback, int),
        LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
    }
    int SetOldProfileVersionCallback(
        int (*func)(LPVOID, unsigned char*, const unsigned short, const int),
        LPVOID lpParam)
    {
        (void)func;
        (void)lpParam;
        return 0;
    }
    PROFILESETTINGS* GetDashboardProfileSettings(int iPad)
    {
        (void)iPad;
        return &m_profileSettings;
    }
    void* GetGameDefinedProfileData(int quadrant)
    {
        (void)quadrant;
        return nullptr;
    }
    void ReadFromProfile(int quadrant, int readType = 0)
    {
        (void)quadrant;
        (void)readType;
    }
    void WriteToProfile(int quadrant, bool gameDefinedDataChanged = false,
                        bool override5MinuteLimitOnProfileWrites = false)
    {
        (void)quadrant;
        (void)gameDefinedDataChanged;
        (void)override5MinuteLimitOnProfileWrites;
    }
    void DeleteOptionsData(int iPad) { (void)iPad; }
    void ForceQueuedProfileWrites(int iPad = XUSER_INDEX_ANY) { (void)iPad; }
    void SetSaveDeviceSelected(unsigned int pad, bool selected)
    {
        (void)pad;
        (void)selected;
    }
    bool GetSaveDeviceSelected(unsigned int pad)
    {
        (void)pad;
        return true;
    }
    void ClearDLCOffers() {}
    XMARKETPLACE_CONTENTOFFER_INFO& GetOffer(DWORD offerIndex)
    {
        (void)offerIndex;
        return m_offer;
    }
    int GetOfferCount() const { return 0; }
    DWORD InstallOffer(int offerIdCount, ULONGLONG* offerIds,
                       int (*func)(LPVOID, int, int), LPVOID lpParam,
                       bool trial = false)
    {
        (void)offerIdCount;
        (void)offerIds;
        (void)trial;
        if (func != nullptr)
        {
            func(lpParam, 0, 0);
        }
        return 0;
    }
    DWORD InstallOffer(int offerIdCount, WCHAR* productId,
                       int (*func)(LPVOID, int, int), LPVOID lpParam,
                       bool trial = false)
    {
        (void)offerIdCount;
        (void)productId;
        (void)trial;
        if (func != nullptr)
        {
            func(lpParam, 0, 0);
        }
        return 0;
    }
    DWORD InstallOffer(int offerIdCount, WCHAR* productId,
                       int (*func)(LPVOID, int, int), LPVOID lpParam,
                       LPVOID reserved)
    {
        (void)reserved;
        return InstallOffer(offerIdCount, productId, func, lpParam, false);
    }
    ETMSStatus ReadTMSFile(int quadrant, eGlobalStorage storage,
                           eTMS_FileType fileType, WCHAR* filename,
                           BYTE** buffer, DWORD* bufferSize,
                           int (*func)(LPVOID, WCHAR*, int, bool, int),
                           LPVOID lpParam, int action)
    {
        (void)quadrant;
        (void)storage;
        (void)fileType;
        (void)filename;
        (void)action;
        if (buffer != nullptr)
        {
            *buffer = nullptr;
        }
        if (bufferSize != nullptr)
        {
            *bufferSize = 0;
        }
        if (func != nullptr)
        {
            func(lpParam, filename, 0, false, action);
        }
        return ETMSStatus_Idle;
    }
    bool WriteTMSFile(int quadrant, eGlobalStorage storage, WCHAR* filename,
                      BYTE* buffer, DWORD bufferSize)
    {
        (void)quadrant;
        (void)storage;
        (void)filename;
        (void)buffer;
        (void)bufferSize;
        return true;
    }
    bool DeleteTMSFile(int quadrant, eGlobalStorage storage, WCHAR* filename)
    {
        (void)quadrant;
        (void)storage;
        (void)filename;
        return true;
    }
    ETMSStatus TMSPP_ReadFile(
        int pad,
        eGlobalStorage storage,
        eTMS_FILETYPEVAL fileType,
        LPCSTR filename,
        int (*func)(LPVOID, int, int, PTMSPP_FILEDATA, LPCSTR) = nullptr,
        LPVOID lpParam = nullptr,
        int userData = 0)
    {
        (void)pad;
        (void)storage;
        (void)fileType;
        (void)filename;
        (void)userData;
        if (func != nullptr)
        {
            func(lpParam, 0, 0, nullptr, filename);
        }
        return ETMSStatus_Idle;
    }
    EDLCStatus GetDLCOffers(int iPad, int (*func)(LPVOID, int, DWORD, int),
                            LPVOID lpParam, DWORD offerTypes)
    {
        (void)iPad;
        (void)offerTypes;
        if (func != nullptr)
        {
            func(lpParam, 0, 0, 0);
        }
        return EDLC_Idle;
    }
    void SetDLCPackageRoot(char* root) { (void)root; }
    EDLCStatus GetInstalledDLC(int iPad, int (*func)(LPVOID, int, int),
                               LPVOID lpParam)
    {
        (void)iPad;
        if (func != nullptr)
        {
            func(lpParam, 0, 0);
        }
        return EDLC_NoInstalledDLC;
    }
    XCONTENT_DATA& GetDLC(DWORD dw)
    {
        (void)dw;
        return m_content;
    }
    DWORD GetAvailableDLCCount(int iPad)
    {
        (void)iPad;
        return 0;
    }
    DWORD MountInstalledDLC(int iPad, DWORD dlc,
                            int (*func)(LPVOID, int, DWORD, DWORD),
                            LPVOID lpParam, LPCSTR mountDrive = nullptr)
    {
        (void)iPad;
        (void)dlc;
        (void)mountDrive;
        if (func != nullptr)
        {
            func(lpParam, 0, dlc, 0);
        }
        return 0;
    }
    DWORD UnmountInstalledDLC(LPCSTR mountDrive = nullptr)
    {
        (void)mountDrive;
        return 0;
    }
    void GetMountedDLCFileList(const char* mountDrive,
                               std::vector<std::string>& fileList)
    {
        (void)mountDrive;
        fileList.clear();
    }
    std::string GetMountedPath(std::string mount)
    {
        (void)mount;
        return std::string();
    }
    std::string GetMountedPath(const char* mount)
    {
        (void)mount;
        return std::string();
    }
    std::wstring GetMountedPath(const WCHAR* mount)
    {
        (void)mount;
        return std::wstring();
    }
    void SetDLCProductCode(const char* productCode) { (void)productCode; }
    void SetProductUpgradeKey(const char* key) { (void)key; }
    bool CheckForTrialUpgradeKey(void (*func)(LPVOID, bool), LPVOID lpParam)
    {
        if (func != nullptr)
        {
            func(lpParam, false);
        }
        return false;
    }
    void SetDLCInfoMap(std::unordered_map<std::wstring, SONYDLC*>* map)
    {
        (void)map;
    }
    EMessageResult RequestMessageBox(
        UINT title,
        UINT text,
        UINT* optionA,
        UINT optionC,
        DWORD pad = XUSER_INDEX_ANY,
        int (*func)(LPVOID, int, const EMessageResult) = nullptr,
        LPVOID lpParam = nullptr,
        void* stringTable = nullptr,
        WCHAR* formatString = nullptr,
        DWORD focusButton = 0)
    {
        (void)title;
        (void)text;
        (void)optionA;
        (void)optionC;
        (void)pad;
        (void)stringTable;
        (void)formatString;
        (void)focusButton;
        if (func != nullptr)
        {
            func(lpParam, static_cast<int>(pad), EMessage_ResultAccept);
        }
        return EMessage_ResultAccept;
    }
    EMessageResult GetMessageBoxResult() const { return EMessage_ResultAccept; }
    eSaveTransferState SaveTransferClearState() { return eSaveTransfer_Idle; }
    eSaveTransferState SaveTransferGetDetails(
        int pad,
        eGlobalStorage storage,
        WCHAR* filename,
        int (*func)(LPVOID, SAVETRANSFER_FILE_DETAILS*),
        LPVOID lpParam)
    {
        (void)pad;
        (void)storage;
        (void)filename;
        if (func != nullptr)
        {
            func(lpParam, nullptr);
        }
        return eSaveTransfer_Idle;
    }
    eSaveTransferState SaveTransferGetData(
        int pad,
        eGlobalStorage storage,
        WCHAR* filename,
        int (*func)(LPVOID, SAVETRANSFER_FILE_DETAILS*),
        bool (*progress)(LPVOID, const int),
        LPVOID funcParam,
        LPVOID progressParam)
    {
        (void)pad;
        (void)storage;
        (void)filename;
        if (progress != nullptr)
        {
            progress(progressParam, 100);
        }
        if (func != nullptr)
        {
            func(funcParam, nullptr);
        }
        return eSaveTransfer_Idle;
    }

private:
    static void setNullBuffer(PBYTE* data, DWORD* bytes)
    {
        if (data != nullptr)
        {
            *data = nullptr;
        }
        if (bytes != nullptr)
        {
            *bytes = 0;
        }
    }

    bool m_saveDisabled = false;
    ESaveGameState m_saveState = ESaveGame_Idle;
    std::vector<BYTE> m_saveData;
    SAVE_DETAILS m_saveDetails = {};
    PROFILESETTINGS m_profileSettings = {};
    XCONTENT_DATA m_content = {};
    XMARKETPLACE_CONTENTOFFER_INFO m_offer = {};
};

inline C4JStorage StorageManager;
