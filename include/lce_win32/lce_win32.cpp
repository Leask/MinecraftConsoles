#include "lce_win32.h"

#if !defined(_WIN32)

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "lce_time/lce_time.h"

namespace
{
    enum class HandleKind
    {
        kEvent,
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

    std::mutex g_tlsMutex;
    std::vector<bool> g_tlsSlots;
    std::atomic<DWORD> g_nextThreadId(1);
    thread_local ThreadHandle g_currentPseudoThread(HandleKind::kPseudoThread);
    thread_local bool g_hasAssignedPseudoThreadId = false;

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

VOID InitializeCriticalSection(PCRITICAL_SECTION criticalSection)
{
    if (criticalSection == nullptr)
    {
        return;
    }

    if (criticalSection->native_mutex == nullptr)
    {
        criticalSection->native_mutex = new std::recursive_mutex();
    }
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
    if (criticalSection == nullptr || criticalSection->native_mutex == nullptr)
    {
        return;
    }

    delete static_cast<std::recursive_mutex*>(criticalSection->native_mutex);
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

    return FALSE;
}

VOID Sleep(DWORD dwMilliseconds)
{
    LceSleepMilliseconds(dwMilliseconds);
}

#endif
