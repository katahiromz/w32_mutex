// w32_mutex by katahiromz --- std::mutex and std::recursive_mutex for old C++/Win32 compilers
// License: MIT
#pragma once

#ifndef _WIN32
    #error This libray is for Win32 only.
#endif

#include <windows.h>
#include <stdexcept>

#ifndef khmz_noexcept
    #if (__cplusplus >= 201103L) // C++11
        #define khmz_noexcept noexcept
    #else
        #define khmz_noexcept /* empty */
    #endif
#endif

#ifndef khmz_throw
    #ifdef KHMZ_DISABLE_THROW
        #define khmz_throw(e) do { } while (0)
    #else
        #define khmz_throw(e) throw (e)
    #endif
#endif

namespace khmz
{
    class mutex
    {
    public:
        typedef HANDLE native_handle_type;

        mutex()
            : m_hMutex(CreateMutex(NULL, FALSE, NULL))
        {
            if (!m_hMutex)
                khmz_throw(std::runtime_error("Failed to create mutex at khmz::mutex::mutex"));
        }

        ~mutex() khmz_noexcept
        {
            if (m_hMutex)
                CloseHandle(m_hMutex);
        }

        void lock()
        {
            DWORD dwWaitResult = WaitForSingleObject(m_hMutex, INFINITE);
            if (dwWaitResult != WAIT_OBJECT_0)
                khmz_throw(std::runtime_error("Failed to lock mutex at khmz::mutex::lock"));
        }

        void unlock()
        {
            if (!ReleaseMutex(m_hMutex))
                khmz_throw(std::runtime_error("Failed to release mutex at khmz::mutex::unlock"));
        }

        bool try_lock() khmz_noexcept
        {
            DWORD dwWaitResult = WaitForSingleObject(m_hMutex, 0);
            return dwWaitResult == WAIT_OBJECT_0;
        }

        native_handle_type native_handle() khmz_noexcept
        {
            return m_hMutex;
        }

    private:
        native_handle_type m_hMutex;
    };

    class recursive_mutex
    {
    public:
        typedef CRITICAL_SECTION *native_handle_type;

        recursive_mutex() khmz_noexcept
        {
            InitializeCriticalSection(&m_cs);
        }

        ~recursive_mutex() khmz_noexcept
        {
            DeleteCriticalSection(&m_cs);
        }

        void lock() khmz_noexcept
        {
            EnterCriticalSection(&m_cs);
        }

        void unlock() khmz_noexcept
        {
            LeaveCriticalSection(&m_cs);
        }

        bool try_lock() khmz_noexcept
        {
            return TryEnterCriticalSection(&m_cs) != 0;
        }

        native_handle_type native_handle() khmz_noexcept
        {
            return &m_cs;
        }

    private:
        CRITICAL_SECTION m_cs;
    };

    template <typename T_MUTEX>
    class lock_guard
    {
    public:
        explicit lock_guard(T_MUTEX& m) khmz_noexcept : m_mutex(m)
        {
            m_mutex.lock();
        }

        ~lock_guard() khmz_noexcept
        {
            m_mutex.unlock();
        }

    private:
        T_MUTEX& m_mutex;

        lock_guard(const lock_guard&) = delete;
        lock_guard& operator=(const lock_guard&) = delete;
    };

    template <typename T_MUTEX>
    class unique_lock
    {
    public:
        explicit unique_lock(T_MUTEX& m) khmz_noexcept
            : m_mutex(&m), m_owns_lock(true)
        {
            m_mutex->lock();
        }

        unique_lock() khmz_noexcept
            : m_mutex(nullptr), m_owns_lock(false)
        {
        }

#if (__cplusplus >= 201103L) // C++11
        unique_lock(unique_lock&& other) khmz_noexcept
            : m_mutex(other.m_mutex), m_owns_lock(other.m_owns_lock)
        {
            other.m_mutex = nullptr;
            other.m_owns_lock = false;
        }
        unique_lock& operator=(unique_lock&& other) khmz_noexcept
        {
            if (this != &other)
            {
                if (m_owns_lock && m_mutex)
                    m_mutex->unlock();
                m_mutex = other.m_mutex;
                m_owns_lock = other.m_owns_lock;
                other.m_mutex = nullptr;
                other.m_owns_lock = false;
            }
            return *this;
        }
#endif

        ~unique_lock() khmz_noexcept
        {
            if (m_owns_lock)
                m_mutex->unlock();
        }

        void lock()
        {
            if (m_owns_lock)
                khmz_throw(std::runtime_error("Lock already owned at khmz::unique_lock::lock"));
#ifndef KHMZ_DISABLE_THROW
            try
            {
#endif
                m_mutex->lock();
                m_owns_lock = true;
#ifndef KHMZ_DISABLE_THROW
            }
            catch (...)
            {
                m_owns_lock = false;
                throw;
            }
#endif
        }

        bool try_lock()
        {
            if (m_owns_lock)
                khmz_throw(std::runtime_error("Lock already owned at khmz::unique_lock::try_lock"));
            m_owns_lock = m_mutex->try_lock();
            return m_owns_lock;
        }

        void unlock()
        {
            if (!m_owns_lock)
                khmz_throw(std::runtime_error("No lock to release at khmz::unique_lock::unlock"));
            m_mutex->unlock();
            m_owns_lock = false;
        }

        void release() khmz_noexcept
        {
            m_mutex = nullptr;
            m_owns_lock = false;
        }

        bool owns_lock() const khmz_noexcept
        {
            return m_owns_lock;
        }

    private:
        T_MUTEX* m_mutex;
        bool m_owns_lock;

        unique_lock(const unique_lock&) = delete;
        unique_lock& operator=(const unique_lock&) = delete;
    };
} // namespace khmz
