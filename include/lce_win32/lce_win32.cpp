#include "lce_win32.h"

#if !defined(_WIN32)

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>

#include "lce_time/lce_time.h"

#include <unistd.h>

namespace
{
    enum class HandleKind
    {
        kEvent,
        kFile,
        kFind,
        kThread,
        kPseudoThread
    };

    struct HandleBase
    {
        explicit HandleBase(HandleKind handleKind)
            : kind(handleKind)
        {
        }

        virtual ~HandleBase() = default;

        HandleKind kind;
    };

    struct EventHandle : HandleBase
    {
        EventHandle(bool manual, bool initialState)
            : HandleBase(HandleKind::kEvent)
            , manualReset(manual)
            , signaled(initialState)
        {
        }

        std::mutex mutex;
        std::condition_variable cv;
        bool manualReset;
        bool signaled;
    };

    struct ThreadHandle : HandleBase
    {
        ThreadHandle()
            : HandleBase(HandleKind::kThread)
            , startRoutine(nullptr)
            , parameter(nullptr)
            , threadId(0)
            , startRequested(false)
            , finished(false)
            , exitCode(STILL_ACTIVE)
        {
        }

        explicit ThreadHandle(HandleKind kind)
            : HandleBase(kind)
            , startRoutine(nullptr)
            , parameter(nullptr)
            , threadId(0)
            , startRequested(true)
            , finished(false)
            , exitCode(STILL_ACTIVE)
        {
        }

        std::mutex mutex;
        std::condition_variable cv;
        std::thread worker;
        LPTHREAD_START_ROUTINE startRoutine;
        LPVOID parameter;
        DWORD threadId;
        bool startRequested;
        bool finished;
        DWORD exitCode;
    };

    struct FindHandle : HandleBase
    {
        explicit FindHandle(const std::filesystem::path& directory)
            : HandleBase(HandleKind::kFind)
            , iterator(directory)
            , end()
        {
        }

        std::filesystem::directory_iterator iterator;
        std::filesystem::directory_iterator end;
    };

    struct FileHandle : HandleBase
    {
        explicit FileHandle(std::FILE* file)
            : HandleBase(HandleKind::kFile)
            , nativeFile(file)
        {
        }

        std::FILE* nativeFile;
    };

    std::mutex g_tlsMutex;
    std::vector<bool> g_tlsSlots;
    std::atomic<DWORD> g_nextThreadId(1);
    thread_local DWORD g_lastError = 0;
    thread_local ThreadHandle g_currentPseudoThread(HandleKind::kPseudoThread);
    thread_local bool g_hasAssignedPseudoThreadId = false;

    struct CriticalSectionRegistry
    {
        std::mutex mutex;
        std::unordered_map<PCRITICAL_SECTION, std::recursive_mutex*> sections;
    };

    CriticalSectionRegistry& GetCriticalSectionRegistry()
    {
        static CriticalSectionRegistry* registry =
            new CriticalSectionRegistry();
        return *registry;
    }

    ThreadHandle* AsThreadHandle(HANDLE handle)
    {
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }

        HandleBase* base = static_cast<HandleBase*>(handle);
        if (base->kind != HandleKind::kThread &&
            base->kind != HandleKind::kPseudoThread)
        {
            return nullptr;
        }

        return static_cast<ThreadHandle*>(handle);
    }

    EventHandle* AsEventHandle(HANDLE handle)
    {
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }

        HandleBase* base = static_cast<HandleBase*>(handle);
        if (base->kind != HandleKind::kEvent)
        {
            return nullptr;
        }

        return static_cast<EventHandle*>(handle);
    }

    FindHandle* AsFindHandle(HANDLE handle)
    {
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }

        HandleBase* base = static_cast<HandleBase*>(handle);
        if (base->kind != HandleKind::kFind)
        {
            return nullptr;
        }

        return static_cast<FindHandle*>(handle);
    }

    FileHandle* AsFileHandle(HANDLE handle)
    {
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }

        HandleBase* base = static_cast<HandleBase*>(handle);
        if (base->kind != HandleKind::kFile)
        {
            return nullptr;
        }

        return static_cast<FileHandle*>(handle);
    }

    DWORD EnsureCurrentThreadId()
    {
        if (!g_hasAssignedPseudoThreadId)
        {
            g_currentPseudoThread.threadId = g_nextThreadId.fetch_add(1);
            g_hasAssignedPseudoThreadId = true;
        }

        return g_currentPseudoThread.threadId;
    }

    ThreadHandle* EnsureCurrentThreadHandle()
    {
        EnsureCurrentThreadId();
        return &g_currentPseudoThread;
    }

    std::unordered_map<DWORD, LPVOID>& CurrentThreadTlsValues()
    {
        thread_local std::unordered_map<DWORD, LPVOID> values;
        return values;
    }

    void SetLastErrorFromCode(const std::error_code& errorCode)
    {
        g_lastError = static_cast<DWORD>(errorCode.value());
    }

    void ClearLastError()
    {
        g_lastError = 0;
    }

    constexpr uint64_t kUnixToWindowsEpoch100Ns = 116444736000000000ULL;

    void StoreFileTime(uint64_t value, LPFILETIME fileTime)
    {
        fileTime->dwLowDateTime = static_cast<DWORD>(value & 0xffffffffu);
        fileTime->dwHighDateTime = static_cast<DWORD>(value >> 32);
    }

    uint64_t LoadFileTime(const FILETIME& fileTime)
    {
        return (static_cast<uint64_t>(fileTime.dwHighDateTime) << 32) |
            fileTime.dwLowDateTime;
    }

    void FillSystemTime(const std::tm& time, int milliseconds, LPSYSTEMTIME out)
    {
        out->wYear = static_cast<WORD>(time.tm_year + 1900);
        out->wMonth = static_cast<WORD>(time.tm_mon + 1);
        out->wDayOfWeek = static_cast<WORD>(time.tm_wday);
        out->wDay = static_cast<WORD>(time.tm_mday);
        out->wHour = static_cast<WORD>(time.tm_hour);
        out->wMinute = static_cast<WORD>(time.tm_min);
        out->wSecond = static_cast<WORD>(time.tm_sec);
        out->wMilliseconds = static_cast<WORD>(milliseconds);
    }

    std::string NormalizePathString(const char* path)
    {
        if (path == nullptr)
        {
            return std::string();
        }

        std::string normalized(path);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        return normalized;
    }

    std::wstring NormalizePathString(const wchar_t* path)
    {
        if (path == nullptr)
        {
            return std::wstring();
        }

        std::wstring normalized(path);
        std::replace(normalized.begin(), normalized.end(), L'\\', L'/');
        return normalized;
    }

    std::wstring DecodeMultiByteString(UINT codePage, LPCSTR source, int count)
    {
        std::wstring result;
        if (source == nullptr)
        {
            return result;
        }

        const bool nullTerminated = count < 0;
        const std::size_t byteCount = nullTerminated
            ? std::strlen(source) + 1
            : static_cast<std::size_t>(count);

        for (std::size_t i = 0; i < byteCount;)
        {
            const unsigned char byte =
                static_cast<unsigned char>(source[i]);
            if (nullTerminated && byte == 0)
            {
                result.push_back(L'\0');
                break;
            }

            if (codePage != CP_UTF8 || byte < 0x80)
            {
                result.push_back(static_cast<wchar_t>(byte));
                ++i;
                continue;
            }

            uint32_t codePoint = 0;
            std::size_t extraBytes = 0;
            if ((byte & 0xe0) == 0xc0)
            {
                codePoint = byte & 0x1f;
                extraBytes = 1;
            }
            else if ((byte & 0xf0) == 0xe0)
            {
                codePoint = byte & 0x0f;
                extraBytes = 2;
            }
            else if ((byte & 0xf8) == 0xf0)
            {
                codePoint = byte & 0x07;
                extraBytes = 3;
            }
            else
            {
                result.push_back(L'?');
                ++i;
                continue;
            }

            if (i + extraBytes >= byteCount)
            {
                result.push_back(L'?');
                break;
            }

            bool valid = true;
            for (std::size_t j = 1; j <= extraBytes; ++j)
            {
                const unsigned char continuation =
                    static_cast<unsigned char>(source[i + j]);
                if ((continuation & 0xc0) != 0x80)
                {
                    valid = false;
                    break;
                }
                codePoint = (codePoint << 6) | (continuation & 0x3f);
            }

            result.push_back(valid ? static_cast<wchar_t>(codePoint) : L'?');
            i += valid ? extraBytes + 1 : 1;
        }

        return result;
    }

    std::string EncodeWideString(UINT codePage, LPCWSTR source, int count,
                                 LPCSTR defaultChar, LPBOOL usedDefaultChar)
    {
        std::string result;
        if (source == nullptr)
        {
            return result;
        }
        if (usedDefaultChar != nullptr)
        {
            *usedDefaultChar = FALSE;
        }

        const bool nullTerminated = count < 0;
        const std::size_t charCount = nullTerminated
            ? std::wcslen(source) + 1
            : static_cast<std::size_t>(count);
        const char fallback =
            (defaultChar != nullptr && defaultChar[0] != '\0') ? defaultChar[0] : '?';

        for (std::size_t i = 0; i < charCount; ++i)
        {
            const uint32_t codePoint = static_cast<uint32_t>(source[i]);
            if (nullTerminated && codePoint == 0)
            {
                result.push_back('\0');
                break;
            }

            if (codePage != CP_UTF8)
            {
                if (codePoint <= 0x7f)
                {
                    result.push_back(static_cast<char>(codePoint));
                }
                else
                {
                    result.push_back(fallback);
                    if (usedDefaultChar != nullptr)
                    {
                        *usedDefaultChar = TRUE;
                    }
                }
                continue;
            }

            if (codePoint <= 0x7f)
            {
                result.push_back(static_cast<char>(codePoint));
            }
            else if (codePoint <= 0x7ff)
            {
                result.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
                result.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
            else if (codePoint <= 0xffff)
            {
                result.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
                result.push_back(
                    static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
                result.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
            else
            {
                result.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
                result.push_back(
                    static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
                result.push_back(
                    static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
                result.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
        }

        return result;
    }

    std::filesystem::path MakePath(LPCSTR path)
    {
        return std::filesystem::path(NormalizePathString(path));
    }

    std::filesystem::path MakePath(LPCWSTR path)
    {
        return std::filesystem::path(NormalizePathString(path));
    }

    std::filesystem::path MakeFindDirectory(LPCSTR pattern)
    {
        std::string normalized = NormalizePathString(pattern);
        const std::size_t wildcard = normalized.find('*');
        if (wildcard != std::string::npos)
        {
            normalized.erase(wildcard);
        }
        while (!normalized.empty() && normalized.back() == '/')
        {
            normalized.pop_back();
        }

        return normalized.empty()
            ? std::filesystem::path(".")
            : std::filesystem::path(normalized);
    }

    std::filesystem::path MakeFindDirectory(LPCWSTR pattern)
    {
        std::wstring normalized = NormalizePathString(pattern);
        const std::size_t wildcard = normalized.find(L'*');
        if (wildcard != std::wstring::npos)
        {
            normalized.erase(wildcard);
        }
        while (!normalized.empty() && normalized.back() == L'/')
        {
            normalized.pop_back();
        }

        return normalized.empty()
            ? std::filesystem::path(".")
            : std::filesystem::path(normalized);
    }

    BOOL FillFindData(
        const std::filesystem::directory_entry& entry,
        WIN32_FIND_DATA* findData)
    {
        if (findData == nullptr)
        {
            return FALSE;
        }

        std::memset(findData, 0, sizeof(*findData));
        if (entry.is_directory())
        {
            findData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        }

        const std::string fileName = entry.path().filename().string();
        std::strncpy(findData->cFileName, fileName.c_str(), MAX_PATH - 1);
        findData->cFileName[MAX_PATH - 1] = '\0';
        return TRUE;
    }

    FILETIME ToFileTime(std::filesystem::file_time_type time)
    {
        const auto systemTime =
            std::chrono::time_point_cast<std::chrono::nanoseconds>(
                time - std::filesystem::file_time_type::clock::now() +
                std::chrono::system_clock::now());
        const auto ticksSinceUnixEpoch =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                systemTime.time_since_epoch()).count();
        const std::uint64_t windowsTicks =
            static_cast<std::uint64_t>(ticksSinceUnixEpoch / 100) +
            116444736000000000ULL;

        FILETIME fileTime;
        fileTime.dwLowDateTime = static_cast<DWORD>(windowsTicks & 0xffffffffu);
        fileTime.dwHighDateTime = static_cast<DWORD>(windowsTicks >> 32);
        return fileTime;
    }

    BOOL FillFileAttributes(
        const std::filesystem::path& path,
        WIN32_FILE_ATTRIBUTE_DATA* fileInformation)
    {
        if (fileInformation == nullptr)
        {
            return FALSE;
        }

        std::error_code errorCode;
        const std::filesystem::file_status status =
            std::filesystem::status(path, errorCode);
        if (errorCode || !std::filesystem::exists(status))
        {
            SetLastErrorFromCode(errorCode);
            return FALSE;
        }

        std::memset(fileInformation, 0, sizeof(*fileInformation));
        if (std::filesystem::is_directory(status))
        {
            fileInformation->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        }

        if (!std::filesystem::is_directory(status))
        {
            const std::uintmax_t size =
                std::filesystem::file_size(path, errorCode);
            if (!errorCode)
            {
                fileInformation->nFileSizeHigh =
                    static_cast<DWORD>((size >> 32) & 0xffffffffu);
                fileInformation->nFileSizeLow =
                    static_cast<DWORD>(size & 0xffffffffu);
            }
        }

        const std::filesystem::file_time_type lastWrite =
            std::filesystem::last_write_time(path, errorCode);
        if (!errorCode)
        {
            fileInformation->ftLastWriteTime = ToFileTime(lastWrite);
            fileInformation->ftLastAccessTime = fileInformation->ftLastWriteTime;
            fileInformation->ftCreationTime = fileInformation->ftLastWriteTime;
        }

        ClearLastError();
        return TRUE;
    }

    bool IsThreadSignaled(ThreadHandle* handle)
    {
        std::lock_guard<std::mutex> lock(handle->mutex);
        return handle->finished;
    }

    bool IsEventSignaled(EventHandle* handle, bool consumeSignal)
    {
        std::lock_guard<std::mutex> lock(handle->mutex);
        if (!handle->signaled)
        {
            return false;
        }

        if (consumeSignal && !handle->manualReset)
        {
            handle->signaled = false;
        }

        return true;
    }

    bool AreAllHandlesSignaled(
        DWORD nCount,
        CONST HANDLE* lpHandles,
        bool consumeSignals)
    {
        std::vector<EventHandle*> autoResetEvents;
        autoResetEvents.reserve(nCount);

        for (DWORD i = 0; i < nCount; ++i)
        {
            ThreadHandle* threadHandle = AsThreadHandle(lpHandles[i]);
            if (threadHandle != nullptr)
            {
                if (!IsThreadSignaled(threadHandle))
                {
                    return false;
                }

                continue;
            }

            EventHandle* eventHandle = AsEventHandle(lpHandles[i]);
            if (eventHandle == nullptr)
            {
                return false;
            }

            std::lock_guard<std::mutex> lock(eventHandle->mutex);
            if (!eventHandle->signaled)
            {
                return false;
            }

            if (consumeSignals && !eventHandle->manualReset)
            {
                autoResetEvents.push_back(eventHandle);
            }
        }

        if (consumeSignals)
        {
            for (EventHandle* eventHandle : autoResetEvents)
            {
                std::lock_guard<std::mutex> lock(eventHandle->mutex);
                eventHandle->signaled = false;
            }
        }

        return true;
    }

    DWORD FindAnySignaledHandle(
        DWORD nCount,
        CONST HANDLE* lpHandles,
        bool consumeSignal)
    {
        for (DWORD i = 0; i < nCount; ++i)
        {
            ThreadHandle* threadHandle = AsThreadHandle(lpHandles[i]);
            if (threadHandle != nullptr)
            {
                if (IsThreadSignaled(threadHandle))
                {
                    return WAIT_OBJECT_0 + i;
                }

                continue;
            }

            EventHandle* eventHandle = AsEventHandle(lpHandles[i]);
            if (eventHandle != nullptr &&
                IsEventSignaled(eventHandle, consumeSignal))
            {
                return WAIT_OBJECT_0 + i;
            }
        }

        return WAIT_FAILED;
    }
}

DWORD TlsAlloc(void)
{
    std::lock_guard<std::mutex> lock(g_tlsMutex);

    for (std::size_t i = 0; i < g_tlsSlots.size(); ++i)
    {
        if (!g_tlsSlots[i])
        {
            g_tlsSlots[i] = true;
            return static_cast<DWORD>(i);
        }
    }

    g_tlsSlots.push_back(true);
    return static_cast<DWORD>(g_tlsSlots.size() - 1);
}

BOOL TlsFree(DWORD dwTlsIndex)
{
    std::lock_guard<std::mutex> lock(g_tlsMutex);
    if (dwTlsIndex >= g_tlsSlots.size() || !g_tlsSlots[dwTlsIndex])
    {
        return FALSE;
    }

    g_tlsSlots[dwTlsIndex] = false;
    CurrentThreadTlsValues().erase(dwTlsIndex);
    return TRUE;
}

LPVOID TlsGetValue(DWORD dwTlsIndex)
{
    {
        std::lock_guard<std::mutex> lock(g_tlsMutex);
        if (dwTlsIndex >= g_tlsSlots.size() || !g_tlsSlots[dwTlsIndex])
        {
            return nullptr;
        }
    }

    std::unordered_map<DWORD, LPVOID>& values = CurrentThreadTlsValues();
    std::unordered_map<DWORD, LPVOID>::const_iterator it = values.find(dwTlsIndex);
    if (it == values.end())
    {
        return nullptr;
    }

    return it->second;
}

BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    {
        std::lock_guard<std::mutex> lock(g_tlsMutex);
        if (dwTlsIndex >= g_tlsSlots.size() || !g_tlsSlots[dwTlsIndex])
        {
            return FALSE;
        }
    }

    CurrentThreadTlsValues()[dwTlsIndex] = lpTlsValue;
    return TRUE;
}

LONG InterlockedCompareExchange(
    LONG volatile* destination,
    LONG exchange,
    LONG comperand)
{
    LONG expected = comperand;
    __atomic_compare_exchange_n(
        const_cast<LONG*>(destination),
        &expected,
        exchange,
        false,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST);
    return expected;
}

LONG InterlockedCompareExchangeRelease(
    LONG volatile* destination,
    LONG exchange,
    LONG comperand)
{
    LONG expected = comperand;
    __atomic_compare_exchange_n(
        const_cast<LONG*>(destination),
        &expected,
        exchange,
        false,
        __ATOMIC_RELEASE,
        __ATOMIC_RELAXED);
    return expected;
}

LONG64 InterlockedCompareExchange64(
    LONG64 volatile* destination,
    LONG64 exchange,
    LONG64 comperand)
{
    LONG64 expected = comperand;
    __atomic_compare_exchange_n(
        const_cast<LONG64*>(destination),
        &expected,
        exchange,
        false,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST);
    return expected;
}

LONG64 InterlockedCompareExchangeRelease64(
    LONG64 volatile* destination,
    LONG64 exchange,
    LONG64 comperand)
{
    LONG64 expected = comperand;
    __atomic_compare_exchange_n(
        const_cast<LONG64*>(destination),
        &expected,
        exchange,
        false,
        __ATOMIC_RELEASE,
        __ATOMIC_RELAXED);
    return expected;
}

wchar_t* _itow(int value, wchar_t* buffer, int radix)
{
    if (buffer == nullptr)
    {
        return nullptr;
    }

    if (radix < 2 || radix > 36)
    {
        buffer[0] = L'\0';
        return buffer;
    }

    wchar_t digits[36];
    for (int i = 0; i < 10; ++i)
    {
        digits[i] = static_cast<wchar_t>(L'0' + i);
    }
    for (int i = 10; i < 36; ++i)
    {
        digits[i] = static_cast<wchar_t>(L'a' + (i - 10));
    }

    wchar_t scratch[36];
    int scratchCount = 0;
    bool negative = false;
    std::uint32_t magnitude = 0;

    if (radix == 10 && value < 0)
    {
        negative = true;
        magnitude = static_cast<std::uint32_t>(-(value + 1)) + 1u;
    }
    else
    {
        magnitude = static_cast<std::uint32_t>(value);
    }

    do
    {
        scratch[scratchCount++] = digits[magnitude % radix];
        magnitude /= static_cast<std::uint32_t>(radix);
    } while (magnitude != 0);

    wchar_t* out = buffer;
    if (negative)
    {
        *out++ = L'-';
    }
    while (scratchCount > 0)
    {
        *out++ = scratch[--scratchCount];
    }
    *out = L'\0';
    return buffer;
}

int _itow_s(int value, wchar_t* buffer, int radix)
{
    return _itow(value, buffer, radix) == nullptr ? E_FAIL : S_OK;
}

VOID InitializeCriticalSection(PCRITICAL_SECTION criticalSection)
{
    if (criticalSection == nullptr)
    {
        return;
    }

    CriticalSectionRegistry& registry = GetCriticalSectionRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    std::unordered_map<PCRITICAL_SECTION, std::recursive_mutex*>::iterator it =
        registry.sections.find(criticalSection);
    if (it != registry.sections.end())
    {
        criticalSection->native_mutex = it->second;
        return;
    }

    std::recursive_mutex* nativeMutex = new std::recursive_mutex();
    registry.sections[criticalSection] = nativeMutex;
    criticalSection->native_mutex = nativeMutex;
}

VOID InitializeCriticalSectionAndSpinCount(
    PCRITICAL_SECTION criticalSection,
    ULONG spinCount)
{
    (void)spinCount;
    InitializeCriticalSection(criticalSection);
}

VOID DeleteCriticalSection(PCRITICAL_SECTION criticalSection)
{
    if (criticalSection == nullptr)
    {
        return;
    }

    CriticalSectionRegistry& registry = GetCriticalSectionRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    std::unordered_map<PCRITICAL_SECTION, std::recursive_mutex*>::iterator it =
        registry.sections.find(criticalSection);
    if (it != registry.sections.end())
    {
        delete it->second;
        registry.sections.erase(it);
    }
    criticalSection->native_mutex = nullptr;
}

VOID EnterCriticalSection(PCRITICAL_SECTION criticalSection)
{
    InitializeCriticalSection(criticalSection);
    static_cast<std::recursive_mutex*>(criticalSection->native_mutex)->lock();
}

VOID LeaveCriticalSection(PCRITICAL_SECTION criticalSection)
{
    if (criticalSection == nullptr || criticalSection->native_mutex == nullptr)
    {
        return;
    }

    static_cast<std::recursive_mutex*>(criticalSection->native_mutex)->unlock();
}

ULONG TryEnterCriticalSection(PCRITICAL_SECTION criticalSection)
{
    InitializeCriticalSection(criticalSection);
    return static_cast<std::recursive_mutex*>(criticalSection->native_mutex)
        ->try_lock()
        ? TRUE
        : FALSE;
}

HANDLE CreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName)
{
    (void)lpEventAttributes;
    (void)lpName;
    return new EventHandle(bManualReset != FALSE, bInitialState != FALSE);
}

BOOL SetEvent(HANDLE hEvent)
{
    EventHandle* handle = AsEventHandle(hEvent);
    if (handle == nullptr)
    {
        return FALSE;
    }

    {
        std::lock_guard<std::mutex> lock(handle->mutex);
        handle->signaled = true;
    }

    if (handle->manualReset)
    {
        handle->cv.notify_all();
    }
    else
    {
        handle->cv.notify_one();
    }

    return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
    EventHandle* handle = AsEventHandle(hEvent);
    if (handle == nullptr)
    {
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(handle->mutex);
    handle->signaled = false;
    return TRUE;
}

HANDLE CreateThread(
    void* lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId)
{
    (void)lpThreadAttributes;
    (void)dwStackSize;

    ThreadHandle* handle = new ThreadHandle();
    handle->startRoutine = lpStartAddress;
    handle->parameter = lpParameter;
    handle->threadId = g_nextThreadId.fetch_add(1);
    handle->startRequested = (dwCreationFlags & CREATE_SUSPENDED) == 0;

    handle->worker = std::thread([handle]() {
        EnsureCurrentThreadHandle();
        g_currentPseudoThread.threadId = handle->threadId;

        {
            std::unique_lock<std::mutex> lock(handle->mutex);
            handle->cv.wait(lock, [handle]() {
                return handle->startRequested;
            });
        }

        DWORD exitCode = STILL_ACTIVE;
        if (handle->startRoutine != nullptr)
        {
            exitCode = handle->startRoutine(handle->parameter);
        }

        {
            std::lock_guard<std::mutex> lock(handle->mutex);
            handle->exitCode = exitCode;
            handle->finished = true;
        }

        handle->cv.notify_all();
    });

    if (lpThreadId != nullptr)
    {
        *lpThreadId = handle->threadId;
    }

    return handle;
}

DWORD ResumeThread(HANDLE hThread)
{
    ThreadHandle* handle = AsThreadHandle(hThread);
    if (handle == nullptr || handle->kind == HandleKind::kPseudoThread)
    {
        return static_cast<DWORD>(-1);
    }

    {
        std::lock_guard<std::mutex> lock(handle->mutex);
        handle->startRequested = true;
    }
    handle->cv.notify_all();
    return 0;
}

DWORD GetCurrentThreadId(void)
{
    return EnsureCurrentThreadId();
}

DWORD GetCurrentProcessId(void)
{
    return static_cast<DWORD>(::getpid());
}

HANDLE GetCurrentThread(void)
{
    return EnsureCurrentThreadHandle();
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority)
{
    (void)hThread;
    (void)nPriority;
    return TRUE;
}

DWORD_PTR SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask)
{
    (void)hThread;
    return dwThreadAffinityMask;
}

BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode)
{
    ThreadHandle* handle = AsThreadHandle(hThread);
    if (handle == nullptr || lpExitCode == nullptr)
    {
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(handle->mutex);
    *lpExitCode = handle->finished ? handle->exitCode : STILL_ACTIVE;
    return TRUE;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    ThreadHandle* threadHandle = AsThreadHandle(hHandle);
    if (threadHandle != nullptr)
    {
        std::unique_lock<std::mutex> lock(threadHandle->mutex);
        if (dwMilliseconds == INFINITE)
        {
            threadHandle->cv.wait(lock, [threadHandle]() {
                return threadHandle->finished;
            });
            return WAIT_OBJECT_0;
        }

        const std::chrono::milliseconds timeout(dwMilliseconds);
        return threadHandle->cv.wait_for(lock, timeout, [threadHandle]() {
            return threadHandle->finished;
        })
            ? WAIT_OBJECT_0
            : WAIT_TIMEOUT;
    }

    EventHandle* eventHandle = AsEventHandle(hHandle);
    if (eventHandle != nullptr)
    {
        std::unique_lock<std::mutex> lock(eventHandle->mutex);
        if (dwMilliseconds == INFINITE)
        {
            eventHandle->cv.wait(lock, [eventHandle]() {
                return eventHandle->signaled;
            });
        }
        else
        {
            const std::chrono::milliseconds timeout(dwMilliseconds);
            if (!eventHandle->cv.wait_for(lock, timeout, [eventHandle]() {
                    return eventHandle->signaled;
                }))
            {
                return WAIT_TIMEOUT;
            }
        }

        if (!eventHandle->manualReset)
        {
            eventHandle->signaled = false;
        }

        return WAIT_OBJECT_0;
    }

    return WAIT_FAILED;
}

DWORD WaitForMultipleObjects(
    DWORD nCount,
    CONST HANDLE* lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds)
{
    if (nCount == 0 || lpHandles == nullptr)
    {
        return WAIT_FAILED;
    }

    const bool waitAll = bWaitAll != FALSE;
    const std::uint64_t start = LceGetMonotonicMilliseconds();

    while (true)
    {
        if (waitAll)
        {
            if (AreAllHandlesSignaled(nCount, lpHandles, true))
            {
                return WAIT_OBJECT_0;
            }
        }
        else
        {
            const DWORD result = FindAnySignaledHandle(nCount, lpHandles, true);
            if (result != WAIT_FAILED)
            {
                return result;
            }
        }

        if (dwMilliseconds != INFINITE)
        {
            const std::uint64_t elapsed =
                LceGetMonotonicMilliseconds() - start;
            if (elapsed >= dwMilliseconds)
            {
                return WAIT_TIMEOUT;
            }
        }

        LceSleepMilliseconds(1);
    }
}

BOOL CloseHandle(HANDLE hObject)
{
    ThreadHandle* threadHandle = AsThreadHandle(hObject);
    if (threadHandle != nullptr)
    {
        if (threadHandle->kind == HandleKind::kPseudoThread)
        {
            return FALSE;
        }

        if (threadHandle->worker.joinable())
        {
            {
                std::lock_guard<std::mutex> lock(threadHandle->mutex);
                threadHandle->startRequested = true;
            }
            threadHandle->cv.notify_all();
            threadHandle->worker.join();
        }

        delete threadHandle;
        return TRUE;
    }

    EventHandle* eventHandle = AsEventHandle(hObject);
    if (eventHandle != nullptr)
    {
        delete eventHandle;
        return TRUE;
    }

    FileHandle* fileHandle = AsFileHandle(hObject);
    if (fileHandle != nullptr)
    {
        if (fileHandle->nativeFile != nullptr)
        {
            std::fclose(fileHandle->nativeFile);
        }

        delete fileHandle;
        return TRUE;
    }

    return FALSE;
}

VOID Sleep(DWORD dwMilliseconds)
{
    LceSleepMilliseconds(dwMilliseconds);
}

BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
    if (lpPerformanceCount == nullptr)
    {
        return FALSE;
    }

    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    lpPerformanceCount->QuadPart =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency)
{
    if (lpFrequency == nullptr)
    {
        return FALSE;
    }

    lpFrequency->QuadPart = 1000000000LL;
    return TRUE;
}

DWORD GetTickCount(void)
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<DWORD>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

VOID GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
    if (lpSystemTime == nullptr)
    {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utcTime {};
    gmtime_r(&time, &utcTime);
    FillSystemTime(utcTime, static_cast<int>(millis), lpSystemTime);
}

VOID GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    if (lpSystemTime == nullptr)
    {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime {};
    localtime_r(&time, &localTime);
    FillSystemTime(localTime, static_cast<int>(millis), lpSystemTime);
}

VOID GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
    if (lpSystemTimeAsFileTime == nullptr)
    {
        return;
    }

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ticks = std::chrono::duration_cast<
        std::chrono::duration<uint64_t, std::ratio<1, 10000000>>>(now).count();
    StoreFileTime(ticks + kUnixToWindowsEpoch100Ns, lpSystemTimeAsFileTime);
}

BOOL FileTimeToSystemTime(
    CONST FILETIME* lpFileTime,
    LPSYSTEMTIME lpSystemTime)
{
    if (lpFileTime == nullptr || lpSystemTime == nullptr)
    {
        return FALSE;
    }

    const uint64_t windowsTicks = LoadFileTime(*lpFileTime);
    if (windowsTicks < kUnixToWindowsEpoch100Ns)
    {
        return FALSE;
    }

    const uint64_t unixTicks = windowsTicks - kUnixToWindowsEpoch100Ns;
    const std::time_t seconds = static_cast<std::time_t>(unixTicks / 10000000);
    const int millis = static_cast<int>((unixTicks % 10000000) / 10000);
    std::tm utcTime {};
    gmtime_r(&seconds, &utcTime);
    FillSystemTime(utcTime, millis, lpSystemTime);
    return TRUE;
}

BOOL SystemTimeToFileTime(
    CONST SYSTEMTIME* lpSystemTime,
    LPFILETIME lpFileTime)
{
    if (lpSystemTime == nullptr || lpFileTime == nullptr)
    {
        return FALSE;
    }

    std::tm utcTime {};
    utcTime.tm_year = static_cast<int>(lpSystemTime->wYear) - 1900;
    utcTime.tm_mon = static_cast<int>(lpSystemTime->wMonth) - 1;
    utcTime.tm_mday = static_cast<int>(lpSystemTime->wDay);
    utcTime.tm_hour = static_cast<int>(lpSystemTime->wHour);
    utcTime.tm_min = static_cast<int>(lpSystemTime->wMinute);
    utcTime.tm_sec = static_cast<int>(lpSystemTime->wSecond);

    const std::time_t seconds = timegm(&utcTime);
    const uint64_t ticks =
        static_cast<uint64_t>(seconds) * 10000000ULL +
        static_cast<uint64_t>(lpSystemTime->wMilliseconds) * 10000ULL;
    StoreFileTime(ticks + kUnixToWindowsEpoch100Ns, lpFileTime);
    return TRUE;
}

VOID GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
    if (lpBuffer == nullptr)
    {
        return;
    }

    std::memset(lpBuffer, 0, sizeof(MEMORYSTATUS));
    lpBuffer->dwLength = sizeof(MEMORYSTATUS);

    SIZE_T totalPhysical = 0;
    SIZE_T availablePhysical = 0;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    const long physicalPages = sysconf(_SC_PHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (physicalPages > 0 && pageSize > 0)
    {
        totalPhysical =
            static_cast<SIZE_T>(physicalPages) * static_cast<SIZE_T>(pageSize);
    }
#endif

#if defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGESIZE)
    const long availablePages = sysconf(_SC_AVPHYS_PAGES);
    const long availablePageSize = sysconf(_SC_PAGESIZE);
    if (availablePages > 0 && availablePageSize > 0)
    {
        availablePhysical = static_cast<SIZE_T>(availablePages) *
            static_cast<SIZE_T>(availablePageSize);
    }
#endif

    if (availablePhysical == 0 && totalPhysical != 0)
    {
        availablePhysical = totalPhysical;
    }

    lpBuffer->dwTotalPhys = totalPhysical;
    lpBuffer->dwAvailPhys = availablePhysical;
    lpBuffer->dwTotalPageFile = totalPhysical;
    lpBuffer->dwAvailPageFile = availablePhysical;
    lpBuffer->dwTotalVirtual = totalPhysical;
    lpBuffer->dwAvailVirtual = availablePhysical;
    if (totalPhysical != 0 && availablePhysical <= totalPhysical)
    {
        lpBuffer->dwMemoryLoad = static_cast<DWORD>(
            ((totalPhysical - availablePhysical) * 100) / totalPhysical);
    }
}

int MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                        int cbMultiByte, LPWSTR lpWideCharStr,
                        int cchWideChar)
{
    (void)dwFlags;
    if (lpMultiByteStr == nullptr || cbMultiByte == 0 || cchWideChar < 0)
    {
        return 0;
    }

    const std::wstring decoded =
        DecodeMultiByteString(CodePage, lpMultiByteStr, cbMultiByte);
    const int required = static_cast<int>(decoded.size());
    if (lpWideCharStr == nullptr || cchWideChar == 0)
    {
        return required;
    }
    if (cchWideChar < required)
    {
        return 0;
    }

    std::memcpy(
        lpWideCharStr,
        decoded.data(),
        decoded.size() * sizeof(wchar_t));
    return required;
}

int WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                        int cchWideChar, LPSTR lpMultiByteStr,
                        int cbMultiByte, LPCSTR lpDefaultChar,
                        LPBOOL lpUsedDefaultChar)
{
    (void)dwFlags;
    if (lpWideCharStr == nullptr || cchWideChar == 0 || cbMultiByte < 0)
    {
        return 0;
    }

    const std::string encoded =
        EncodeWideString(
            CodePage,
            lpWideCharStr,
            cchWideChar,
            lpDefaultChar,
            lpUsedDefaultChar);
    const int required = static_cast<int>(encoded.size());
    if (lpMultiByteStr == nullptr || cbMultiByte == 0)
    {
        return required;
    }
    if (cbMultiByte < required)
    {
        return 0;
    }

    std::memcpy(lpMultiByteStr, encoded.data(), encoded.size());
    return required;
}

DWORD GetLastError(void)
{
    return g_lastError;
}

HMODULE GetModuleHandle(LPCSTR lpModuleName)
{
    (void)lpModuleName;
    return nullptr;
}

HMODULE GetModuleHandleA(LPCSTR lpModuleName)
{
    return GetModuleHandle(lpModuleName);
}

DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize)
{
    (void)hModule;
    if (lpFilename == nullptr || nSize == 0)
    {
        return 0;
    }

    const std::string path = std::filesystem::current_path().string();
    if (path.size() + 1 > nSize)
    {
        lpFilename[0] = '\0';
        return 0;
    }

    std::strncpy(lpFilename, path.c_str(), nSize);
    lpFilename[nSize - 1] = '\0';
    return static_cast<DWORD>(std::strlen(lpFilename));
}

FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
    (void)hModule;
    (void)lpProcName;
    return nullptr;
}

BOOL DeleteFile(LPCSTR lpFileName)
{
    std::error_code errorCode;
    const bool removed = std::filesystem::remove(MakePath(lpFileName), errorCode);
    if (errorCode || !removed)
    {
        SetLastErrorFromCode(errorCode);
        return FALSE;
    }

    ClearLastError();
    return TRUE;
}

BOOL DeleteFile(LPCWSTR lpFileName)
{
    std::error_code errorCode;
    const bool removed = std::filesystem::remove(MakePath(lpFileName), errorCode);
    if (errorCode || !removed)
    {
        SetLastErrorFromCode(errorCode);
        return FALSE;
    }

    ClearLastError();
    return TRUE;
}

BOOL CreateDirectory(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    (void)lpSecurityAttributes;

    std::error_code errorCode;
    const bool created =
        std::filesystem::create_directory(MakePath(lpPathName), errorCode);
    if (errorCode || !created)
    {
        SetLastErrorFromCode(errorCode);
        return FALSE;
    }

    ClearLastError();
    return TRUE;
}

BOOL CreateDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    (void)lpSecurityAttributes;

    std::error_code errorCode;
    const bool created =
        std::filesystem::create_directory(MakePath(lpPathName), errorCode);
    if (errorCode || !created)
    {
        SetLastErrorFromCode(errorCode);
        return FALSE;
    }

    ClearLastError();
    return TRUE;
}

DWORD GetFileAttributes(LPCSTR lpFileName)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!FillFileAttributes(MakePath(lpFileName), &data))
    {
        return static_cast<DWORD>(-1);
    }

    return data.dwFileAttributes;
}

DWORD GetFileAttributes(LPCWSTR lpFileName)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!FillFileAttributes(MakePath(lpFileName), &data))
    {
        return static_cast<DWORD>(-1);
    }

    return data.dwFileAttributes;
}

BOOL GetFileAttributesEx(
    LPCSTR lpFileName,
    int fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation)
{
    (void)fInfoLevelId;
    return FillFileAttributes(MakePath(lpFileName), lpFileInformation);
}

BOOL GetFileAttributesEx(
    LPCWSTR lpFileName,
    int fInfoLevelId,
    WIN32_FILE_ATTRIBUTE_DATA* lpFileInformation)
{
    (void)fInfoLevelId;
    return FillFileAttributes(MakePath(lpFileName), lpFileInformation);
}

BOOL MoveFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
    std::error_code errorCode;
    std::filesystem::rename(
        MakePath(lpExistingFileName),
        MakePath(lpNewFileName),
        errorCode);
    if (errorCode)
    {
        SetLastErrorFromCode(errorCode);
        return FALSE;
    }

    ClearLastError();
    return TRUE;
}

BOOL MoveFile(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    std::error_code errorCode;
    std::filesystem::rename(
        MakePath(lpExistingFileName),
        MakePath(lpNewFileName),
        errorCode);
    if (errorCode)
    {
        SetLastErrorFromCode(errorCode);
        return FALSE;
    }

    ClearLastError();
    return TRUE;
}

HANDLE CreateFile(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    (void)dwShareMode;
    (void)lpSecurityAttributes;
    (void)dwFlagsAndAttributes;
    (void)hTemplateFile;

    const std::filesystem::path path = MakePath(lpFileName);
    const bool wantsWrite = (dwDesiredAccess & GENERIC_WRITE) != 0;
    const char* mode = wantsWrite ? "r+b" : "rb";
    std::FILE* file = std::fopen(path.string().c_str(), mode);
    if (file == nullptr && wantsWrite &&
        dwCreationDisposition == OPEN_ALWAYS)
    {
        file = std::fopen(path.string().c_str(), "w+b");
    }

    if (file == nullptr)
    {
        g_lastError = 1;
        return INVALID_HANDLE_VALUE;
    }

    ClearLastError();
    return new FileHandle(file);
}

HANDLE CreateFile(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    const std::filesystem::path path = MakePath(lpFileName);
    return CreateFile(
        path.string().c_str(),
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
    FileHandle* fileHandle = AsFileHandle(hFile);
    if (fileHandle == nullptr || fileHandle->nativeFile == nullptr)
    {
        g_lastError = static_cast<DWORD>(EINVAL);
        if (lpFileSizeHigh != nullptr)
        {
            *lpFileSizeHigh = 0;
        }
        return INVALID_FILE_SIZE;
    }

    const off_t current = ftello(fileHandle->nativeFile);
    if (current < 0)
    {
        g_lastError = static_cast<DWORD>(errno);
        return INVALID_FILE_SIZE;
    }

    if (fseeko(fileHandle->nativeFile, 0, SEEK_END) != 0)
    {
        g_lastError = static_cast<DWORD>(errno);
        return INVALID_FILE_SIZE;
    }

    const off_t size = ftello(fileHandle->nativeFile);
    const int seekBackResult = fseeko(fileHandle->nativeFile, current, SEEK_SET);
    if (size < 0 || seekBackResult != 0)
    {
        g_lastError = static_cast<DWORD>(errno);
        return INVALID_FILE_SIZE;
    }

    if (lpFileSizeHigh != nullptr)
    {
        *lpFileSizeHigh = static_cast<DWORD>(
            static_cast<uint64_t>(size) >> 32);
    }

    ClearLastError();
    return static_cast<DWORD>(static_cast<uint64_t>(size) & 0xffffffffu);
}

DWORD SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    FileHandle* fileHandle = AsFileHandle(hFile);
    if (fileHandle == nullptr || fileHandle->nativeFile == nullptr)
    {
        g_lastError = static_cast<DWORD>(EINVAL);
        return INVALID_SET_FILE_POINTER;
    }

    int origin = SEEK_SET;
    switch (dwMoveMethod)
    {
    case FILE_BEGIN:
        origin = SEEK_SET;
        break;
    case FILE_CURRENT:
        origin = SEEK_CUR;
        break;
    case FILE_END:
        origin = SEEK_END;
        break;
    default:
        g_lastError = static_cast<DWORD>(EINVAL);
        return INVALID_SET_FILE_POINTER;
    }

    int64_t distance = lDistanceToMove;
    if (lpDistanceToMoveHigh != nullptr)
    {
        distance = (static_cast<int64_t>(*lpDistanceToMoveHigh) << 32) |
            static_cast<uint32_t>(lDistanceToMove);
    }

    if (fseeko(fileHandle->nativeFile, static_cast<off_t>(distance), origin) != 0)
    {
        g_lastError = static_cast<DWORD>(errno);
        return INVALID_SET_FILE_POINTER;
    }

    const off_t position = ftello(fileHandle->nativeFile);
    if (position < 0)
    {
        g_lastError = static_cast<DWORD>(errno);
        return INVALID_SET_FILE_POINTER;
    }

    const uint64_t packedPosition = static_cast<uint64_t>(position);
    if (lpDistanceToMoveHigh != nullptr)
    {
        *lpDistanceToMoveHigh = static_cast<LONG>(packedPosition >> 32);
    }

    ClearLastError();
    return static_cast<DWORD>(packedPosition & 0xffffffffu);
}

BOOL ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPVOID lpOverlapped)
{
    (void)lpOverlapped;

    FileHandle* fileHandle = AsFileHandle(hFile);
    if (fileHandle == nullptr || fileHandle->nativeFile == nullptr)
    {
        return FALSE;
    }

    const std::size_t bytesRead = std::fread(
        lpBuffer,
        1,
        nNumberOfBytesToRead,
        fileHandle->nativeFile);
    if (lpNumberOfBytesRead != nullptr)
    {
        *lpNumberOfBytesRead = static_cast<DWORD>(bytesRead);
    }

    return std::ferror(fileHandle->nativeFile) == 0 ? TRUE : FALSE;
}

BOOL WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPVOID lpOverlapped)
{
    (void)lpOverlapped;

    FileHandle* fileHandle = AsFileHandle(hFile);
    if (fileHandle == nullptr || fileHandle->nativeFile == nullptr)
    {
        return FALSE;
    }

    const std::size_t bytesWritten = std::fwrite(
        lpBuffer,
        1,
        nNumberOfBytesToWrite,
        fileHandle->nativeFile);
    if (lpNumberOfBytesWritten != nullptr)
    {
        *lpNumberOfBytesWritten = static_cast<DWORD>(bytesWritten);
    }

    return bytesWritten == nNumberOfBytesToWrite ? TRUE : FALSE;
}

HANDLE FindFirstFile(LPCSTR lpFileName, WIN32_FIND_DATA* lpFindFileData)
{
    try
    {
        FindHandle* handle = new FindHandle(MakeFindDirectory(lpFileName));
        if (handle->iterator == handle->end)
        {
            delete handle;
            return INVALID_HANDLE_VALUE;
        }

        FillFindData(*handle->iterator, lpFindFileData);
        ClearLastError();
        return handle;
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        g_lastError = static_cast<DWORD>(error.code().value());
        return INVALID_HANDLE_VALUE;
    }
}

HANDLE FindFirstFile(LPCWSTR lpFileName, WIN32_FIND_DATA* lpFindFileData)
{
    try
    {
        FindHandle* handle = new FindHandle(MakeFindDirectory(lpFileName));
        if (handle->iterator == handle->end)
        {
            delete handle;
            return INVALID_HANDLE_VALUE;
        }

        FillFindData(*handle->iterator, lpFindFileData);
        ClearLastError();
        return handle;
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        g_lastError = static_cast<DWORD>(error.code().value());
        return INVALID_HANDLE_VALUE;
    }
}

BOOL FindNextFile(HANDLE hFindFile, WIN32_FIND_DATA* lpFindFileData)
{
    FindHandle* handle = AsFindHandle(hFindFile);
    if (handle == nullptr)
    {
        return FALSE;
    }

    try
    {
        ++handle->iterator;
        if (handle->iterator == handle->end)
        {
            return FALSE;
        }

        FillFindData(*handle->iterator, lpFindFileData);
        ClearLastError();
        return TRUE;
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        g_lastError = static_cast<DWORD>(error.code().value());
        return FALSE;
    }
}

BOOL FindClose(HANDLE hFindFile)
{
    FindHandle* handle = AsFindHandle(hFindFile);
    if (handle == nullptr)
    {
        return FALSE;
    }

    delete handle;
    return TRUE;
}

#endif
