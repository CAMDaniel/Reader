/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      osdepend.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-03-23
 *
 * This file abstracts some often used system dependent functions
 * such as creatingThreads, sleep, thread locks, file sizes, file
 * existence, ...
 *
 * It should work on Windows, OS X and Linux
 */
#ifndef OSDEPEND_H
#define OSDEPEND_H
#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include <ctime>
#include <cmath>
#include "common.h"


#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
    #define THREAD_HANDLE  HANDLE
    #define WIN32_UTF8
    #ifdef _MSC_VER
        #ifdef snprintf
        #undef snprintf
        #endif
        #ifdef strncpy
        #undef strncpy
        #endif
        #define snprintf(a,b,...) _snprintf_s(a,b,b,__VA_ARGS__)
        #define strncpy(a,b,c) strncpy_s(a,c,b,((c)-1))
    #endif
#else
    #include <unistd.h>
    #include <dlfcn.h>
    #include <pthread.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <errno.h>
    #include <semaphore.h>
    #include <sys/time.h>
    #include <cstdarg>

    #define INVALID_HANDLE_VALUE   0
    #define INFINITE               0xFFFFFFFF
    #define THREAD_HANDLE          pthread_t
    #define _strnicmp               strncasecmp
    #define _stricmp                strcasecmp
#endif

#ifdef __APPLE__
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#define __useconds_t useconds_t
#endif

#define TIMEOUT_INF             0xffffffff
#define INVALID_THREAD_ID       0

//#######################################################################
//                          UNICODE
//#######################################################################

#ifdef WIN32
inline std::wstring utf8ToWString(const char* text)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, text, static_cast<int>(strlen(text)), NULL, 0);
    wchar_t* wstr = new wchar_t[n+1];
    MultiByteToWideChar(CP_UTF8, 0, text, static_cast<int>(strlen(text)), wstr, n);
    wstr[n] = L'\0';
    std::wstring result = std::wstring(wstr);
    delete[] wstr;
    return result;
}

inline std::string wstringToUtf8(std::wstring wstr)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    char* str = new char[n+1];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), str, n, NULL, NULL);
    str[n] = '\0';
    std::string result = std::string(str);
    delete[] str;
    return result;
}
#endif

inline int vsnprintfx(char* outbuff, size_t size, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
#ifdef _MSC_VER
    int n = _vsnprintf_s(outbuff, size, size-1, format, ap);
#else
    int n = vsnprintf(outbuff, size, format, ap);
#endif
    va_end(ap);
    return n;
}

inline int snprintfx(char* outbuff, size_t size, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
#ifdef _MSC_VER
    int n = _snprintf_s(outbuff, size, size-1, format, ap);
#else
    int n = snprintf(outbuff, size, format, ap);
#endif
    va_end(ap);
    return n;
}



//#######################################################################
//              GENERAL FILE IO
//#######################################################################

inline FILE* openFile(const char* fileName, const char* mode)
{
    FILE* file;
#ifdef WIN32
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(fileName);
    std::wstring wmode = utf8ToWString(mode);
    if (_wfopen_s(&file, wfile.c_str(), wmode.c_str()))
        file=NULL;
#else
    if (fopen_s(&file, fileName, mode))
        file=NULL;
#endif
#else
        file = fopen(fileName, mode);
#endif
    return file;
}

inline int fseek64(FILE* file, i64 offset, int origin)
{
#ifdef WIN32
    return _fseeki64(file, offset, origin);
#elif __APPLE__
    return fseeko(file, offset, origin);
#else
    return fseeko64(file, offset, origin);
#endif
}

inline i64 ftell64(FILE* file)
{
#ifdef WIN32
    return _ftelli64(file);
#elif __APPLE__
    return ftello(file);
#else
    return ftello64(file);
#endif
}

inline i64 getFileSize(const char* filePath)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    FILE* file;
    std::wstring wfile = utf8ToWString(filePath);
    if (_wfopen_s(&file, wfile.c_str(), L"rb"))
        file=NULL;
#else
    FILE* file;
    if (fopen_s(&file, filePath, "rb"))
        file=NULL;
#endif
#else
    FILE *file = fopen(filePath, "rb");
#endif
    if (!file)
        return -1;
    i64 size = -1;
    if (!fseek64(file, 0, SEEK_END))
        size = ftell64(file);
    fclose(file);
    return size;
}

inline i64 getFileSize(std::string filePath)
{
    return getFileSize(filePath.c_str());
}

inline bool fileExists(const char* filePath)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    WIN32_FIND_DATAW fd;
    std::wstring wfile = utf8ToWString(filePath);
    HANDLE hFind = FindFirstFileW(wfile.c_str(), &fd);
#else
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(filePath, &fd);
#endif
    FindClose(hFind);
    return hFind != INVALID_HANDLE_VALUE;

#else
    struct stat fileStat;
    return stat(filePath, &fileStat) == 0;
#endif
}

inline bool fileExists(std::string filePath)
{
    return fileExists(filePath.c_str());
}

inline bool isDirectory(const char* filePath)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    WIN32_FIND_DATAW fd;
    std::wstring wfile = utf8ToWString(filePath);
    HANDLE hFind = FindFirstFileW(wfile.c_str(), &fd);
#else
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(filePath, &fd);
#endif
    FindClose(hFind);
    return (hFind != INVALID_HANDLE_VALUE && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat fileStat;
    return stat(filePath, &fileStat) == 0 && S_ISDIR(fileStat.st_mode);
#endif
}

inline bool isDirectory(std::string filePath)
{
    return isDirectory(filePath.c_str());
}

inline bool isFile(const char* filePath)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    WIN32_FIND_DATAW fd;
    std::wstring wfile = utf8ToWString(filePath);
    HANDLE hFind = ::FindFirstFileW(wfile.c_str(), &fd);
#else
    WIN32_FIND_DATAA fd;
    HANDLE hFind = ::FindFirstFileA(filePath, &fd);
#endif
    if (hFind == INVALID_HANDLE_VALUE)
        return false;
    FindClose(hFind);
    return (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? false : true;
#else
    struct stat fileStat;
    return stat(filePath, &fileStat) == 0 && S_ISREG(fileStat.st_mode);
#endif
}

inline bool isFile(std::string filePath)
{
    return isFile(filePath.c_str());
}

inline int createDir(const char *dirPath)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(dirPath);
    return CreateDirectoryW(wfile.c_str(), NULL) != 0;
#else
    return CreateDirectoryA(dirPath, NULL) != 0;
#endif
#else
    return mkdir(dirPath, S_IRWXU);
#endif
}

inline int createDir(std::string dirPath)
{
    return createDir(dirPath.c_str());
}

class File {
public:
    File(const char* _fileName, const char *mode)
        : file(0), fileName(_fileName)
    {
#ifdef WIN32
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(fileName);
    std::wstring wmode = utf8ToWString(mode);
    if (_wfopen_s(&file, wfile.c_str(), wmode.c_str()))
        file=NULL;
#else
    if (fopen_s(&file, fileName, mode))
        file=NULL;
#endif
#else
        file = fopen(fileName, mode);
#endif
    }

    ~File() {
        if (file) fclose(file);
    }

    FILE * open(const char *name, const char *mode) {
#ifdef WIN32
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(name);
    std::wstring wmode = utf8ToWString(mode);
    if (_wfopen_s(&file, wfile.c_str(), wmode.c_str()))
        file=NULL;
#else
    if (fopen_s(&file, fileName, mode))
        file=NULL;
#endif
#else
        file = fopen(name, mode);
#endif
        return file;
    }
    void close() { if (file) { fclose(file); file = 0; } }
    operator FILE*() { return file; }
private:
    FILE *file;
    const char *fileName;
};


//#######################################################################
//                     OTHER STUFF
//#######################################################################

static inline double getPrecTime() {
#ifdef WIN32
    static double initPrecTime = 0;
    static LARGE_INTEGER perfFrequency = {{0,0}};
    LARGE_INTEGER currentCount;
    if (perfFrequency.QuadPart == 0)
        QueryPerformanceFrequency(&perfFrequency);
    QueryPerformanceCounter(&currentCount);
    double precTime =  currentCount.QuadPart/(double)perfFrequency.QuadPart;
    if (initPrecTime == 0) {
        time_t initTime = 0;
        time(&initTime);
        SYSTEMTIME sysTime;
        GetSystemTime(&sysTime);
        initPrecTime = (double) initTime + sysTime.wMilliseconds * .001 - precTime;
    }
    return initPrecTime + precTime;
#else
    timeval timeNow;
    gettimeofday(&timeNow, 0);
    return (double) timeNow.tv_sec + ((double)timeNow.tv_usec)/1000000.0;
#endif
}

inline char* precTimeToStr(double time, char str[64])
{
    time_t itime = static_cast<time_t>(time);
#ifdef _MSC_VER
    char strtime[256];
    ctime_s(strtime, 256, &itime);
#else
    const char* strtime = ctime(&itime);
#endif
    unsigned us = static_cast<unsigned>((time - (double)itime)*1e6);
    snprintf(str, 64, "%.19s.%06u%.5s", strtime, us, strtime+19);
    return str;
}

inline unsigned getLastError()
{
#ifdef WIN32
    return GetLastError();
#else
    return errno;
#endif
}

inline std::string getComputerName()
{
#ifdef WIN32
    char output[512];
    DWORD size = 512;
    memset(output, 0, 512);
    std::string name = "";
    if (GetComputerNameA(output, &size))
        name = std::string(output);
    return name;
#else
    char output[1024];
    memset(output, 0, 1024);
    if (gethostname(output, 1024) != 0)
        return std::string("");
    return std::string(output);
#endif
}


//#######################################################################
//                      DYNAMIC LIBARIES IO
//#######################################################################

inline HMODULE loadLibrary(const char *filePath)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(filePath);
    return LoadLibraryW(wfile.c_str());
#else
    return LoadLibraryA(filePath);
#endif
#else
    HMODULE h = dlopen(filePath, RTLD_LAZY);
    if (!h)
        fprintf(stderr, "Cannot load library \"%s\": %s\n", filePath, dlerror());
    return h;
#endif
}

inline int freeLibrary(HMODULE handle)
{
#ifdef WIN32
    return FreeLibrary(handle) ? 0 : -1;
#else
    return dlclose(handle);
#endif
}

inline void* getSymbolAddress(HMODULE handle, const char *symbolName)
{
#ifdef WIN32
    return (void*)GetProcAddress(handle, symbolName);
#else
    return dlsym(handle, symbolName);
#endif
}


//#######################################################################
//                          THREADS
//#######################################################################

inline THREAD_HANDLE createThread(void (*func)(void*), void *pars)
{
#ifdef WIN32
    uintptr_t handle = _beginthread(func, 0, pars);
    return (handle == 1L ? INVALID_HANDLE_VALUE : (THREAD_HANDLE) handle);
#else
    struct ThreadWrapper {
        ThreadWrapper(void (*_func)(void *), void *_pars) : thfunc(_func), thpars(_pars) {}
        static void* func(void *pars) {
            if (pars) {
                ThreadWrapper *w = (ThreadWrapper*)pars;
                w->thfunc(w->thpars);
                delete w;
            }
            return 0;
        }
        void (*thfunc)(void *);
        void *thpars;
    };
    ThreadWrapper *w = new ThreadWrapper(func, pars);
    pthread_attr_t attr;
    pthread_t thread;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int rc = pthread_create(&thread, &attr, ThreadWrapper::func, w);
    pthread_attr_destroy(&attr);
    if (rc) {
        delete w;
        return INVALID_HANDLE_VALUE;
    }
    return thread;
#endif
}

inline void sleepThread(double seconds)
{
#ifdef WIN32
    DWORD milliseconds = (DWORD)(seconds * 1000);
    Sleep(milliseconds);
#else
    usleep((__useconds_t)(seconds * 1000000));
#endif
}

inline THREAD_HANDLE getCurThread()
{
#ifdef WIN32
    return GetCurrentThread();
#else
    return pthread_self();
#endif
}


#ifdef WIN32
inline int setThreadPriority(THREAD_HANDLE hThread, int priority)
{
    return SetThreadPriority(hThread, priority) ? 0 : -1;
#else
inline int setThreadPriority(THREAD_HANDLE hThread, int)
{
    sched_param schedPar;
    int policy;
    pthread_getschedparam(hThread, &policy, &schedPar);
    #ifdef __APPLE__
    schedPar.sched_priority = 1;
    #else
    schedPar.__sched_priority = 1;
    #endif
    int rc = pthread_setschedparam(hThread, SCHED_RR, &schedPar);
    return rc;
#endif
}

inline THREADID getCurThreadId()
{
#ifdef WIN32
    return GetCurrentThreadId();
#else
    return (THREADID) pthread_self();
#endif
}

#ifdef WIN32
class ThreadSyncObject {
public:
    ThreadSyncObject()           { InitializeCriticalSection(&winLock); }
    virtual ~ThreadSyncObject()  { DeleteCriticalSection(&winLock);}
    unsigned getOwnerThreadId()  { return (unsigned) (INT_PTR) winLock.OwningThread;}
    bool isLocked()              { return winLock.RecursionCount > 0; }

    bool lock(unsigned timeout = TIMEOUT_INF) {
        bool succ;
        if (timeout == TIMEOUT_INF) {
            EnterCriticalSection(&winLock);
            succ = true;
        } else {
            unsigned time = 0;
            succ = TryEnterCriticalSection(&winLock) == TRUE;
            while (time < timeout && !succ) {
                Sleep(10);
                time += 10;
                succ = TryEnterCriticalSection(&winLock) == TRUE;
            }
        }
        return succ;
    }

    bool unlock() {
        if ((u32)(intptr_t)winLock.OwningThread != GetCurrentThreadId()) {
            assert(0);
            return false;
        }
        if (winLock.RecursionCount > 0)
            LeaveCriticalSection(&winLock);
        return true;
    }

    bool isLockedByThisThread() {
        return ((u32)(intptr_t)winLock.OwningThread == GetCurrentThreadId()) && isLocked();;
    }

private:
    CRITICAL_SECTION winLock;
    int recurCount;
};

#else

class ThreadSyncObject {
public:
    ThreadSyncObject()
        : threadId(INVALID_THREAD_ID)
        , recurCount(0)
    {
        pthread_mutexattr_t attrib;
        pthread_mutexattr_init(&attrib);
        pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&mutex, &attrib);
        pthread_mutexattr_destroy(&attrib);
    }
    virtual ~ThreadSyncObject() { pthread_mutex_destroy(&mutex); }
    THREADID getOwnerThreadId() { return threadId; }
    bool isLocked()             { return recurCount > 0; }

    bool lock(unsigned timeout = TIMEOUT_INF) {
        bool succ;
        if (timeout == TIMEOUT_INF) {
            if (pthread_mutex_lock(&mutex))
                return false;
            succ = true;
        } else {
            unsigned time = 0;
            succ = (pthread_mutex_trylock(&mutex) == 0);
            while (time < timeout && !succ) {
                usleep(1000);
                ++time;
                succ = (pthread_mutex_trylock(&mutex) == 0);
            }
        }
        if (succ) {
            ++recurCount;
            threadId = getCurThreadId();
        }
        return succ;
    }

    bool unlock() {
        if (threadId != getCurThreadId()) {
            assert(0);
            return false;
        }
        int lockCount = --recurCount;
        if (lockCount == 0)
            threadId = INVALID_THREAD_ID;
        assert(recurCount >= 0);
        pthread_mutex_unlock(&mutex);
        return true;
    }

    bool isLockedByThisThread() {
        return threadId == getCurThreadId() && isLocked();
    }

private:
    pthread_mutex_t mutex;
    THREADID threadId;
    int recurCount;
};

#endif


class ThreadLock {
public:
    ThreadLock(ThreadSyncObject *thSync = 0)
        : sync(thSync)
    {
        assert(sync);
        sync->lock();
    }
    ~ThreadLock() {
        sync->unlock();
    }
private:
    ThreadSyncObject *sync;
};

class ThreadLockTimeout {
public:
    ThreadLockTimeout(ThreadSyncObject *thSync = 0, unsigned timeoutInMs = TIMEOUT_INF)
        : sync(thSync)
        , mLocked(false)
    {
        assert(sync);
        mLocked = sync->lock(timeoutInMs);
    }
    ~ThreadLockTimeout() {
        if (mLocked)
            sync->unlock();
    }
    bool locked() { return mLocked;  }
    void unlock() { if (mLocked) { sync->unlock(); mLocked = false; } }
    void lock(unsigned timeoutInMs = TIMEOUT_INF) { if (!mLocked) { mLocked = sync->lock(timeoutInMs); } }
private:
    ThreadSyncObject *sync;
    bool mLocked;
};

class ThreadLockManual {
public:
    ThreadLockManual(ThreadSyncObject *thSync = 0, bool _locked = true)
        : locked(_locked)
        , sync(thSync)
    {
        assert(sync);
        if (_locked)
            sync->lock();
    }
    ~ThreadLockManual() {
        if (locked)
            sync->unlock();
    }
    void lock() {
        if (!locked){
            sync->lock();
            locked = true;
        }
    }
    bool unlock() {
        if (locked && sync->unlock()) {
            locked = false;
            return true;
        }
        return false;
    }
private:
    bool locked;
    ThreadSyncObject *sync;
};

template <typename T> void preciseWait(double seconds, T *abortFlag, T abortVal)
// if time less than 20 ms, does not sleep thread
{
    double endTime = getPrecTime() + seconds;
    while (endTime - getPrecTime() > 0.02) {
        if (*abortFlag == abortVal)
            return;
        sleepThread(0.01);
    }
    while (getPrecTime() < endTime) {
        if (*abortFlag == abortVal)
            return;
    }
}

template <typename T> void wait(double seconds, T *abortFlag, T abortVal)
{
    double endTime = getPrecTime() + seconds;
    while (endTime > getPrecTime()) {
        if (*abortFlag == abortVal) return;
        sleepThread(0.01);
    }
}

#ifdef WIN32
class Event
{
public:
    Event(bool initSet = false, bool manualReset = false, const char *name = 0) {
        hEvent = CreateEventA(NULL, (BOOL) manualReset, (BOOL) initSet, name);
    }
    ~Event() {
        if (hEvent != NULL)
            CloseHandle(hEvent);
    }
    bool set()                  {return SetEvent(hEvent) != FALSE;}
    bool reset()                {return ResetEvent(hEvent) != FALSE;}
    bool wait(u32 millisec)     {return WaitForSingleObject(hEvent, millisec) == WAIT_OBJECT_0;}
private:
    HANDLE hEvent;
};

#else

class Event
{
public:
    Event(bool initSet = false, bool _manualReset = false, const char *name = 0) {
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        signaled = initSet;
        manualReset = _manualReset;
        if (name) return; // this does nothing, just to remove compiler warning about unused parameter
    }
    ~Event() {
        pthread_cond_destroy(&cond);
    }
    bool set() {
        int rc = 0;
        rc |= pthread_mutex_lock(&mutex);
        signaled = true;
        rc |= pthread_cond_signal(&cond);
        rc |= pthread_mutex_unlock(&mutex);
        return rc == 0;
    }
    bool reset() {
        int rc = 0;
        rc |= pthread_mutex_lock(&mutex);
        signaled = false;
        rc |= pthread_mutex_unlock(&mutex);
        return rc == 0;
    }
    bool wait(unsigned millisec) {
        pthread_mutex_lock(&mutex);
        if (signaled) {
            if (!manualReset)
                signaled = false;
            pthread_mutex_unlock(&mutex);
            return true;
        }
        if (millisec == 0) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        timespec timeout;
        if (INFINITE != millisec) {
            timeval timeNow;
            gettimeofday(&timeNow, 0);
            timeout.tv_sec = timeNow.tv_sec + millisec / 1000;
            timeout.tv_nsec = (((millisec % 1000) * 1000 + timeNow.tv_usec) % 1000000) * 1000;
        }
        int rc;
        do {
            rc = (INFINITE == millisec ? pthread_cond_wait(&cond, &mutex) : pthread_cond_timedwait(&cond, &mutex, &timeout));
        } while (rc == 0 && !signaled);
        if (rc == 0 && !manualReset)
            signaled = false;
        pthread_mutex_unlock(&mutex);
        return rc == 0;
    }
private:
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    bool signaled;
    bool manualReset;
};

#endif

#endif /* end of include guard: OSDEPEND_H */





