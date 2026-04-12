#pragma once

#if defined(_WIN32)
#include <windows.h>
#else

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cctype>

typedef int BOOL;
typedef BOOL* PBOOL;
typedef BOOL* LPBOOL;
typedef char CHAR;
typedef CHAR* LPSTR;
typedef unsigned char BYTE;
typedef BYTE* PBYTE;
typedef unsigned short WORD;
typedef unsigned short USHORT;
typedef short SHORT;
typedef int32_t LONG;
typedef int64_t LONG64;
typedef int64_t LONGLONG;
#if !defined(_MSC_VER)
typedef int __int32;
typedef long long __int64;
#endif
typedef std::uint32_t DWORD;
typedef std::uint32_t ULONG;
typedef unsigned int UINT;
typedef int INT;
typedef unsigned long long ULONGLONG;
typedef std::size_t SIZE_T;
typedef std::uintptr_t ULONG_PTR;
typedef std::uintptr_t DWORD_PTR;
typedef void VOID;
typedef VOID* PVOID;
typedef float FLOAT;
typedef wchar_t WCHAR;
typedef void* HANDLE;
typedef HANDLE HINSTANCE;
typedef HINSTANCE HMODULE;
typedef HANDLE HWND;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef DWORD* LPDWORD;
typedef DWORD* PDWORD;
typedef LONG* PLONG;
typedef LONG64* PLONG64;
typedef LONG HRESULT;
typedef void* FARPROC;
typedef int errno_t;
typedef VOID* XMEMCOMPRESSION_CONTEXT;
typedef VOID* XMEMDECOMPRESSION_CONTEXT;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

#ifndef CP_ACP
#define CP_ACP 0
#endif

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif

#if !defined(_WIN32) && !defined(_WIN64)
typedef int SOCKET;
static constexpr SOCKET INVALID_SOCKET = -1;
#endif

struct FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

typedef FILETIME* LPFILETIME;

struct SYSTEMTIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
};

typedef SYSTEMTIME* PSYSTEMTIME;
typedef SYSTEMTIME* LPSYSTEMTIME;

union LARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    };
    LONGLONG QuadPart;
};

union ULARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        DWORD HighPart;
    };
    ULONGLONG QuadPart;
};

typedef ULARGE_INTEGER* PULARGE_INTEGER;

struct WIN32_FILE_ATTRIBUTE_DATA
{
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
};

struct WIN32_FIND_DATA
{
    DWORD dwFileAttributes;
    CHAR cFileName[MAX_PATH];
};

struct RECT
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

typedef RECT* LPRECT;

typedef struct _MEMORYSTATUS
{
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef CDECL
#define CDECL
#endif

#ifndef __debugbreak
#if defined(__clang__) || defined(__GNUC__)
#define __debugbreak() __builtin_trap()
#else
#define __debugbreak() ((void)0)
#endif
#endif

#ifndef CONST
#define CONST const
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif

#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif

#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

#ifndef RtlZeroMemory
#define RtlZeroMemory(Destination, Length) std::memset((Destination), 0, (Length))
#endif

#ifndef ZeroMemory
#define ZeroMemory RtlZeroMemory
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

inline BOOL GetClientRect(HWND hWnd, LPRECT rect)
{
    (void)hWnd;
    if (rect == nullptr)
    {
        return FALSE;
    }

    rect->left = 0;
    rect->top = 0;
    rect->right = 1280;
    rect->bottom = 720;
    return TRUE;
}

#ifndef _TRUNCATE
#define _TRUNCATE ((std::size_t)-1)
#endif

#ifndef _countof
#define _countof(array) (sizeof(array) / sizeof((array)[0]))
#endif

inline errno_t strcpy_s(char* destination, std::size_t size, const char* source)
{
    if (destination == nullptr || size == 0)
    {
        return 1;
    }

    if (source == nullptr)
    {
        destination[0] = '\0';
        return 1;
    }

    std::size_t sourceLength = std::strlen(source);
    if (sourceLength >= size)
    {
        std::memcpy(destination, source, size - 1);
        destination[size - 1] = '\0';
        return 1;
    }

    std::memcpy(destination, source, sourceLength + 1);
    return 0;
}

template <typename... Args>
inline int sprintf_s(char* buffer, std::size_t size, const char* format,
                     Args... args)
{
    return std::snprintf(buffer, size, format, args...);
}

template <std::size_t N, typename... Args>
inline int sprintf_s(char (&buffer)[N], const char* format, Args... args)
{
    return std::snprintf(buffer, N, format, args...);
}

template <std::size_t N>
inline errno_t strcpy_s(char (&destination)[N], const char* source)
{
    return strcpy_s(destination, N, source);
}

inline errno_t strcat_s(char* destination, std::size_t size,
                        const char* source)
{
    if (destination == nullptr || size == 0 || source == nullptr)
    {
        return 1;
    }

    const std::size_t destinationLength = std::strlen(destination);
    if (destinationLength >= size)
    {
        destination[0] = '\0';
        return 1;
    }

    const std::size_t remaining = size - destinationLength;
    if (std::strlen(source) + 1 > remaining)
    {
        destination[0] = '\0';
        return 1;
    }

    std::strcat(destination, source);
    return 0;
}

template <std::size_t N>
inline errno_t strcat_s(char (&destination)[N], const char* source)
{
    return strcat_s(destination, N, source);
}

inline unsigned long long _strtoui64(
    const char* nptr,
    char** endptr,
    int base)
{
    return std::strtoull(nptr, endptr, base);
}

#ifndef fprintf_s
#define fprintf_s std::fprintf
#endif

inline errno_t fopen_s(FILE** file, const char* filename, const char* mode)
{
    if (file == nullptr)
    {
        return 1;
    }
    *file = std::fopen(filename, mode);
    return *file == nullptr ? 1 : 0;
}

inline int _stricmp(const char* left, const char* right)
{
    if (left == nullptr || right == nullptr)
    {
        return left == right ? 0 : (left == nullptr ? -1 : 1);
    }

    while (*left != '\0' && *right != '\0')
    {
        const int lc = std::tolower(static_cast<unsigned char>(*left));
        const int rc = std::tolower(static_cast<unsigned char>(*right));
        if (lc != rc)
        {
            return lc - rc;
        }
        ++left;
        ++right;
    }
    return std::tolower(static_cast<unsigned char>(*left)) -
        std::tolower(static_cast<unsigned char>(*right));
}

DWORD GetFileAttributes(LPCSTR lpFileName);

inline DWORD GetFileAttributesA(LPCSTR lpFileName)
{
    return GetFileAttributes(lpFileName);
}

inline errno_t strncpy_s(
    char* destination,
    std::size_t size,
    const char* source,
    std::size_t count)
{
    if (destination == nullptr || size == 0)
    {
        return 1;
    }

    if (source == nullptr)
    {
        destination[0] = '\0';
        return 1;
    }

    std::size_t copyCount = count == _TRUNCATE ? size - 1 : count;
    if (copyCount > size - 1)
    {
        copyCount = size - 1;
    }

    std::strncpy(destination, source, copyCount);
    destination[copyCount] = '\0';
    return count != _TRUNCATE && std::strlen(source) > copyCount ? 1 : 0;
}

template <std::size_t N>
inline errno_t strncpy_s(
    char (&destination)[N],
    const char* source,
    std::size_t count)
{
    return strncpy_s(destination, N, source, count);
}

inline errno_t strncat_s(
    char* destination,
    std::size_t size,
    const char* source,
    std::size_t count)
{
    if (destination == nullptr || source == nullptr || size == 0)
    {
        return 1;
    }

    std::size_t used = std::strlen(destination);
    if (used >= size)
    {
        destination[size - 1] = '\0';
        return 1;
    }

    std::size_t remaining = size - used - 1;
    std::size_t copyCount = count == _TRUNCATE ? remaining : count;
    if (copyCount > remaining)
    {
        copyCount = remaining;
    }

    std::strncat(destination, source, copyCount);
    destination[size - 1] = '\0';
    return count != _TRUNCATE && std::strlen(source) > copyCount ? 1 : 0;
}

template <std::size_t N>
inline errno_t strncat_s(
    char (&destination)[N],
    const char* source,
    std::size_t count)
{
    return strncat_s(destination, N, source, count);
}

inline errno_t wcscpy_s(
    wchar_t* destination,
    std::size_t size,
    const wchar_t* source)
{
    if (destination == nullptr || size == 0)
    {
        return 1;
    }

    if (source == nullptr)
    {
        destination[0] = L'\0';
        return 1;
    }

    std::wcsncpy(destination, source, size - 1);
    destination[size - 1] = L'\0';
    return std::wcslen(source) >= size ? 1 : 0;
}

template <std::size_t N>
inline errno_t wcscpy_s(wchar_t (&destination)[N], const wchar_t* source)
{
    return wcscpy_s(destination, N, source);
}

inline errno_t wcsncpy_s(
    wchar_t* destination,
    std::size_t size,
    const wchar_t* source,
    std::size_t count)
{
    if (destination == nullptr || size == 0)
    {
        return 1;
    }

    if (source == nullptr)
    {
        destination[0] = L'\0';
        return 1;
    }

    std::size_t copyCount = count == _TRUNCATE ? size - 1 : count;
    if (copyCount > size - 1)
    {
        copyCount = size - 1;
    }

    std::wcsncpy(destination, source, copyCount);
    destination[copyCount] = L'\0';
    return count != _TRUNCATE && std::wcslen(source) > copyCount ? 1 : 0;
}

template <std::size_t N>
inline errno_t wcsncpy_s(
    wchar_t (&destination)[N],
    const wchar_t* source,
    std::size_t count)
{
    return wcsncpy_s(destination, N, source, count);
}

inline errno_t mbstowcs_s(
    std::size_t* converted,
    wchar_t* destination,
    std::size_t size,
    const char* source,
    std::size_t count)
{
    if (destination == nullptr || size == 0 || source == nullptr)
    {
        if (converted != nullptr)
        {
            *converted = 0;
        }
        return 1;
    }

    std::size_t limit = count == _TRUNCATE ? size - 1 : count;
    if (limit > size - 1)
    {
        limit = size - 1;
    }

    std::size_t written = std::mbstowcs(destination, source, limit);
    if (written == static_cast<std::size_t>(-1))
    {
        destination[0] = L'\0';
        if (converted != nullptr)
        {
            *converted = 0;
        }
        return 1;
    }

    destination[written] = L'\0';
    if (converted != nullptr)
    {
        *converted = written + 1;
    }
    return 0;
}

inline errno_t memcpy_s(
    void* destination,
    std::size_t destinationSize,
    const void* source,
    std::size_t count)
{
    if (destination == nullptr || source == nullptr || count > destinationSize)
    {
        return 1;
    }

    std::memcpy(destination, source, count);
    return 0;
}

inline int _vsnprintf_s(
    char* buffer,
    std::size_t size,
    std::size_t count,
    const char* format,
    va_list args)
{
    (void)count;
    return std::vsnprintf(buffer, size, format, args);
}

template <std::size_t N>
inline int _vsnprintf_s(
    char (&buffer)[N],
    std::size_t count,
    const char* format,
    va_list args)
{
    return _vsnprintf_s(buffer, N, count, format, args);
}

inline int _vsnwprintf_s(
    wchar_t* buffer,
    std::size_t size,
    std::size_t count,
    const wchar_t* format,
    va_list args)
{
    (void)count;
    return std::vswprintf(buffer, size, format, args);
}

template <std::size_t N>
inline int _vsnwprintf_s(
    wchar_t (&buffer)[N],
    std::size_t count,
    const wchar_t* format,
    va_list args)
{
    return _vsnwprintf_s(buffer, N, count, format, args);
}

#ifndef LCE_WIN32_SWPRINTF_S_DEFINED
#define LCE_WIN32_SWPRINTF_S_DEFINED
template <typename... Args>
inline int swprintf_s(wchar_t* buffer, std::size_t size,
                      const wchar_t* format, Args... args)
{
    return std::swprintf(buffer, size, format, args...);
}

template <std::size_t N, typename... Args>
inline int swprintf_s(wchar_t (&buffer)[N], const wchar_t* format,
                      Args... args)
{
    return std::swprintf(buffer, N, format, args...);
}
#endif

template <typename... Args>
inline int _snprintf_s(
    char* buffer,
    std::size_t size,
    std::size_t count,
    const char* format,
    Args... args)
{
    (void)count;
    return std::snprintf(buffer, size, format, args...);
}

inline errno_t _itoa_s(int value, char* buffer, std::size_t size, int radix)
{
    if (buffer == nullptr || size == 0)
    {
        return 1;
    }

    int result = -1;
    if (radix == 16)
    {
        result = std::snprintf(buffer, size, "%x", value);
    }
    else if (radix == 10)
    {
        result = std::snprintf(buffer, size, "%d", value);
    }
    else
    {
        buffer[0] = '\0';
        return 1;
    }

    return result < 0 || static_cast<std::size_t>(result) >= size ? 1 : 0;
}

inline errno_t _i64toa_s(
    std::int64_t value,
    char* buffer,
    std::size_t size,
    int radix)
{
    if (buffer == nullptr || size == 0)
    {
        return 1;
    }

    int result = -1;
    if (radix == 16)
    {
        result = std::snprintf(
            buffer,
            size,
            "%llx",
            static_cast<unsigned long long>(value));
    }
    else if (radix == 10)
    {
        result = std::snprintf(
            buffer,
            size,
            "%lld",
            static_cast<long long>(value));
    }
    else
    {
        buffer[0] = '\0';
        return 1;
    }

    return result < 0 || static_cast<std::size_t>(result) >= size ? 1 : 0;
}

template <typename... Args>
inline int swscanf_s(const wchar_t* buffer, const wchar_t* format, Args... args)
{
    return std::swscanf(buffer, format, args...);
}

#ifndef MAXULONG_PTR
#define MAXULONG_PTR (~((ULONG_PTR)0))
#endif

#define WAIT_TIMEOUT 258L
#define STATUS_ABANDONED_WAIT_0 ((DWORD)0x00000080L)
#define WAIT_ABANDONED (STATUS_ABANDONED_WAIT_0 + 0)
#define WAIT_FAILED ((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0 0
#define STILL_ACTIVE 259
#define INFINITE 0xFFFFFFFFu
#define NO_ERROR 0L
#define ERROR_SUCCESS 0L
#define ERROR_CANCELLED 1223L
#define ERROR_IO_PENDING 997L

#define CREATE_SUSPENDED 0x00000004

#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define GetFileExInfoStandard 0

#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4

#ifndef FILE_BEGIN
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#endif

#ifndef INVALID_FILE_SIZE
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#endif

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)0xFFFFFFFF)
#endif

#define THREAD_BASE_PRIORITY_LOWRT 15
#define THREAD_BASE_PRIORITY_MAX 2
#define THREAD_BASE_PRIORITY_MIN -2
#define THREAD_BASE_PRIORITY_IDLE -15

#define THREAD_PRIORITY_LOWEST THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL (THREAD_PRIORITY_LOWEST + 1)
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_HIGHEST THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL (THREAD_PRIORITY_HIGHEST - 1)
#define THREAD_PRIORITY_ERROR_RETURN 2147483647
#define THREAD_PRIORITY_TIME_CRITICAL THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE THREAD_BASE_PRIORITY_IDLE

#ifndef LCE_WIN32_PIX_STUBS_DEFINED
#define LCE_WIN32_PIX_STUBS_DEFINED
inline void PIXAddNamedCounter(int, const char*, ...) {}
inline void PIXBeginNamedEvent(int, const char*, ...) {}
inline void PIXEndNamedEvent() {}
inline void PIXSetMarkerDeprecated(int, const char*, ...) {}
#endif

struct SECURITY_ATTRIBUTES
{
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
};

typedef SECURITY_ATTRIBUTES* PSECURITY_ATTRIBUTES;
typedef SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;

struct _RTL_CRITICAL_SECTION
{
    void* native_mutex = nullptr;
};

typedef _RTL_CRITICAL_SECTION RTL_CRITICAL_SECTION;
typedef RTL_CRITICAL_SECTION* PRTL_CRITICAL_SECTION;
typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION PCRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION LPCRITICAL_SECTION;

typedef DWORD(WINAPI* PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

DWORD TlsAlloc(void);
BOOL TlsFree(DWORD dwTlsIndex);
LPVOID TlsGetValue(DWORD dwTlsIndex);
BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);

LONG InterlockedCompareExchange(
    LONG volatile* destination,
    LONG exchange,
    LONG comperand);
LONG InterlockedCompareExchangeRelease(
    LONG volatile* destination,
    LONG exchange,
    LONG comperand);
LONG64 InterlockedCompareExchange64(
    LONG64 volatile* destination,
    LONG64 exchange,
    LONG64 comperand);
LONG64 InterlockedCompareExchangeRelease64(
    LONG64 volatile* destination,
    LONG64 exchange,
    LONG64 comperand);

wchar_t* _itow(int value, wchar_t* buffer, int radix);
int _itow_s(int value, wchar_t* buffer, int radix);

VOID InitializeCriticalSection(PCRITICAL_SECTION criticalSection);
VOID InitializeCriticalSectionAndSpinCount(
    PCRITICAL_SECTION criticalSection,
    ULONG spinCount);
VOID DeleteCriticalSection(PCRITICAL_SECTION criticalSection);
VOID EnterCriticalSection(PCRITICAL_SECTION criticalSection);
VOID LeaveCriticalSection(PCRITICAL_SECTION criticalSection);
ULONG TryEnterCriticalSection(PCRITICAL_SECTION criticalSection);

HANDLE CreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName);
BOOL SetEvent(HANDLE hEvent);
BOOL ResetEvent(HANDLE hEvent);

HANDLE CreateThread(
    void* lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId);
DWORD ResumeThread(HANDLE hThread);
DWORD GetCurrentThreadId(void);
DWORD GetCurrentProcessId(void);
HANDLE GetCurrentThread(void);
BOOL SetThreadPriority(HANDLE hThread, int nPriority);
DWORD_PTR SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask);
BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WaitForMultipleObjects(
    DWORD nCount,
    CONST HANDLE* lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds);
BOOL CloseHandle(HANDLE hObject);
VOID Sleep(DWORD dwMilliseconds);
BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);
DWORD GetTickCount(void);
VOID GetSystemTime(LPSYSTEMTIME lpSystemTime);
VOID GetLocalTime(LPSYSTEMTIME lpSystemTime);
VOID GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);
BOOL FileTimeToSystemTime(
    CONST FILETIME* lpFileTime,
    LPSYSTEMTIME lpSystemTime);
BOOL SystemTimeToFileTime(
    CONST SYSTEMTIME* lpSystemTime,
    LPFILETIME lpFileTime);
VOID GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer);
int MultiByteToWideChar(
    UINT CodePage,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar);
int WideCharToMultiByte(
    UINT CodePage,
    DWORD dwFlags,
    LPCWSTR lpWideCharStr,
    int cchWideChar,
    LPSTR lpMultiByteStr,
    int cbMultiByte,
    LPCSTR lpDefaultChar,
    LPBOOL lpUsedDefaultChar);

DWORD GetLastError(void);
HMODULE GetModuleHandle(LPCSTR lpModuleName);
HMODULE GetModuleHandleA(LPCSTR lpModuleName);
DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize);
FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
BOOL DeleteFile(LPCSTR lpFileName);
BOOL DeleteFile(LPCWSTR lpFileName);
BOOL CreateDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL CreateDirectory(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
DWORD GetFileAttributes(LPCSTR lpFileName);
DWORD GetFileAttributes(LPCWSTR lpFileName);
BOOL GetFileAttributesEx(
    LPCSTR lpFileName,
    int fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation);
BOOL GetFileAttributesEx(
    LPCWSTR lpFileName,
    int fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation);
BOOL MoveFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
BOOL MoveFile(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
HANDLE CreateFile(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);
HANDLE CreateFile(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);
DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
DWORD SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod);
BOOL ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPVOID lpOverlapped);
BOOL WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPVOID lpOverlapped);
HANDLE FindFirstFile(LPCSTR lpFileName, WIN32_FIND_DATA* lpFindFileData);
HANDLE FindFirstFile(LPCWSTR lpFileName, WIN32_FIND_DATA* lpFindFileData);
BOOL FindNextFile(HANDLE hFindFile, WIN32_FIND_DATA* lpFindFileData);
BOOL FindClose(HANDLE hFindFile);

#endif
