#pragma once

#if defined(_WIN32)
#include <windows.h>
#else

#include <cstddef>
#include <cstdint>
#include <cstring>

typedef int BOOL;
typedef char CHAR;
typedef unsigned char BYTE;
typedef BYTE* PBYTE;
typedef unsigned short WORD;
typedef unsigned short USHORT;
typedef short SHORT;
typedef int32_t LONG;
typedef int64_t LONG64;
typedef int64_t LONGLONG;
typedef std::uint32_t DWORD;
typedef std::uint32_t ULONG;
typedef unsigned int UINT;
typedef int INT;
typedef unsigned long long ULONGLONG;
typedef std::size_t SIZE_T;
typedef std::uintptr_t ULONG_PTR;
typedef std::uintptr_t DWORD_PTR;
typedef void VOID;
typedef float FLOAT;
typedef wchar_t WCHAR;
typedef void* HANDLE;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef DWORD* LPDWORD;
typedef LONG* PLONG;
typedef LONG HRESULT;
typedef VOID* XMEMCOMPRESSION_CONTEXT;
typedef VOID* XMEMDECOMPRESSION_CONTEXT;

struct FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

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

#define CREATE_SUSPENDED 0x00000004

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

inline void PIXAddNamedCounter(int, const char*, ...) {}
inline void PIXBeginNamedEvent(int, const char*, ...) {}
inline void PIXEndNamedEvent() {}
inline void PIXSetMarkerDeprecated(int, const char*, ...) {}

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
    void* native_mutex;
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

#endif
