#ifndef POLYCALL_PTHREAD_H
#define POLYCALL_PTHREAD_H

#ifdef _WIN32
#include <windows.h>

typedef CRITICAL_SECTION pthread_mutex_t;
typedef int pthread_mutexattr_t;

static inline int pthread_mutex_init(pthread_mutex_t* mutex,
                                     const pthread_mutexattr_t* attr) {
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t* mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

/* A plain spinlock-based once, not INIT_ONCE/InitOnceExecuteOnce: that API
 * needs Vista+ headers that not every "supports C11" mingw toolchain
 * actually declares. InterlockedCompareExchange has been available and
 * declared unconditionally since Windows 2000. States: 0 = untouched,
 * 1 = a thread is running init_fn, 2 = done. */
typedef volatile LONG pthread_once_t;
#define PTHREAD_ONCE_INIT 0

static inline int pthread_once(pthread_once_t *once, void (*init_fn)(void)) {
    if (InterlockedCompareExchange(once, 1, 0) == 0) {
        init_fn();
        InterlockedExchange(once, 2);
    } else {
        while (*once != 2) {
            Sleep(0);
        }
    }
    return 0;
}
#else
#include <pthread.h>
#endif

#endif
