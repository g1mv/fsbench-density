/**
 * A portable threading interface
 * 
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */
#ifndef THREADS_HPP_BhjgkfG8
#define THREADS_HPP_BhjgkfG8

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)

#include <windows.h>
#include <process.h>

#define THREAD_HANDLE HANDLE
#define THREAD_RETURN_TYPE void
#define THREAD_RETURN
#define MUTEX HANDLE

// It's very simplified
inline int pthread_create(THREAD_HANDLE *thread, const void *ignored,
        THREAD_RETURN_TYPE (*start_routine)(void*), void *arg)
{
    uintptr_t handle = _beginthread(start_routine, 0, arg);
    if(handle == 0 || handle == (uintptr_t)-1L)
    {
        return errno;
    }
    *thread = (HANDLE)handle;
    return 0;
}
// It's very simplified
inline int pthread_join(THREAD_HANDLE thread, void **ignored)
{
    return WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0;
}
inline MUTEX create_mutex()
{
    return CreateMutex(0, 0, 0);
}
inline void destroy_mutex(MUTEX *mutex)
{
    CloseHandle(*mutex);
}
inline void lock_mutex(MUTEX *mutex)
{
    WaitForSingleObject(*mutex, INFINITE);
}
inline void unlock_mutex(MUTEX *mutex)
{
    ReleaseMutex(*mutex);
}
#else
#include <stdexcept>
#include <pthread.h>
#define THREAD_HANDLE pthread_t
#define THREAD_RETURN_TYPE void*
#define THREAD_RETURN NULL
#define MUTEX pthread_mutex_t
inline MUTEX create_mutex()
{
    MUTEX mutex;
    int ret = pthread_mutex_init(&mutex, NULL);
    if(ret != 0)
        throw std::runtime_error("Failed to create a mutex");
    return mutex;
}

inline void destroy_mutex(MUTEX *mutex)
{
    int ret = pthread_mutex_destroy(mutex);
    if(ret != 0)
        throw std::runtime_error("Failed to destroy a mutex");
}

inline void lock_mutex(MUTEX *mutex)
{
    pthread_mutex_lock(mutex);
}
inline void unlock_mutex(MUTEX *mutex)
{
    pthread_mutex_unlock(mutex);
}

#endif

#endif // THREADS_HPP_BhjgkfG8
