/* ES40 emulator.
 * Copyright (C) 2007-2008 by the ES40 Emulator Project
 *
 * WWW    : http://www.es40.org
 * E-mail : camiel@es40.org
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Although this is not required, the author would appreciate being notified of, 
 * and receiving any modifications you may make to the source code that might serve
 * the general public.
 *
 * Parts of this file based upon the Poco C++ Libraries, which is Copyright (C) 
 * 2004-2006, Applied Informatics Software Engineering GmbH. and Contributors.
 */

/**
 * $Id$
 *
 * X-1.1        Camiel Vanderhoeven                             31-MAY-2008
 *      Initial version for ES40 emulator.
 **/

//
// Mutex.h
//
// $Id$
//
// Library: Foundation
// Package: Threading
// Module:  Mutex
//
// Definition of the Mutex and FastMutex classes.
//
// Copyright (c) 2004-2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#ifndef Foundation_Mutex_INCLUDED
#define Foundation_Mutex_INCLUDED

#if defined(NO_LOCK_TIMEOUTS)
#define LOCK_TIMEOUT_MS
#else
#if !defined(LOCK_TIMEOUT_MS)
#define LOCK_TIMEOUT_MS 5000
#endif
#endif

/**
 * This Macro finds out what the current CThread object is, and returns it's name. If there's no
 * CThread object associated with this thread, it's assumed to be the main thread, and "main"
 * is returned.
 **/
#define CURRENT_THREAD_NAME CThread::current() ? CThread::current()->getName().c_str() : "main"


#include "Foundation.h"
#include "Exception.h"
#include "ScopedLock.h"
#include "../es40_debug.h"

#if defined(POCO_OS_FAMILY_WINDOWS)
#include "Mutex_WIN32.h"
#else
#include "Mutex_POSIX.h"
#endif

class CMutex: private CMutexImpl
	/// A Mutex (mutual exclusion) is a synchronization 
	/// mechanism used to control access to a shared resource
	/// in a concurrent (multithreaded) scenario.
	/// Mutexes are recursive, that is, the same mutex can be 
	/// locked multiple times by the same thread (but, of course,
	/// not by other threads).
	/// Using the ScopedLock class is the preferred way to automatically
	/// lock and unlock a mutex.
{
public:
	typedef CScopedLock<CMutex> CScopedLock;

    CMutex(const char* lName);

    char*                       lockName;



	CMutex();
		/// creates the Mutex.
		
	~CMutex();
		/// destroys the Mutex.

	void lock();
		/// Locks the mutex. Blocks if the mutex
		/// is held by another thread.
		
	void lock(long milliseconds);
		/// Locks the mutex. Blocks up to the given number of milliseconds
		/// if the mutex is held by another thread. Throws a TimeoutException
		/// if the mutex can not be locked within the given timeout.
		///
		/// Performance Note: On most platforms (including Windows), this member function is 
		/// implemented using a loop calling (the equivalent of) tryLock() and Thread::sleep().
		/// On POSIX platforms that support pthread_mutex_timedlock(), this is used.

	bool tryLock();
		/// Tries to lock the mutex. Returns false immediately
		/// if the mutex is already held by another thread.
		/// Returns true if the mutex was successfully locked.

	bool tryLock(long milliseconds);
		/// Locks the mutex. Blocks up to the given number of milliseconds
		/// if the mutex is held by another thread.
		/// Returns true if the mutex was successfully locked.
		///
		/// Performance Note: On most platforms (including Windows), this member function is 
		/// implemented using a loop calling (the equivalent of) tryLock() and Thread::sleep().
		/// On POSIX platforms that support pthread_mutex_timedlock(), this is used.

	void unlock();
		/// Unlocks the mutex so that it can be acquired by
		/// other threads.
	
private:
	CMutex(const CMutex&);
	CMutex& operator = (const CMutex&);
};


class CFastMutex: private CFastMutexImpl
	/// A FastMutex (mutual exclusion) is similar to a Mutex.
	/// Unlike a Mutex, however, a FastMutex is not recursive,
	/// which means that a deadlock will occur if the same
	/// thread tries to lock a mutex it has already locked again.
	/// Locking a FastMutex is faster than locking a recursive Mutex.
	/// Using the ScopedLock class is the preferred way to automatically
	/// lock and unlock a mutex.
{
public:
	typedef CScopedLock<CFastMutex> CScopedLock;


    CFastMutex(const char* lName);

    char*                           lockName;


	CFastMutex();
		/// creates the Mutex.
		
	~CFastMutex();
		/// destroys the Mutex.

	void lock();
		/// Locks the mutex. Blocks if the mutex
		/// is held by another thread.

	void lock(long milliseconds);
		/// Locks the mutex. Blocks up to the given number of milliseconds
		/// if the mutex is held by another thread. Throws a TimeoutException
		/// if the mutex can not be locked within the given timeout.
		///
		/// Performance Note: On most platforms (including Windows), this member function is 
		/// implemented using a loop calling (the equivalent of) tryLock() and Thread::sleep().
		/// On POSIX platforms that support pthread_mutex_timedlock(), this is used.

	bool tryLock();
		/// Tries to lock the mutex. Returns false immediately
		/// if the mutex is already held by another thread.
		/// Returns true if the mutex was successfully locked.

	bool tryLock(long milliseconds);
		/// Locks the mutex. Blocks up to the given number of milliseconds
		/// if the mutex is held by another thread.
		/// Returns true if the mutex was successfully locked.
		///
		/// Performance Note: On most platforms (including Windows), this member function is 
		/// implemented using a loop calling (the equivalent of) tryLock() and Thread::sleep().
		/// On POSIX platforms that support pthread_mutex_timedlock(), this is used.

	void unlock();
		/// Unlocks the mutex so that it can be acquired by
		/// other threads.
	
private:
	CFastMutex(const CFastMutex&);
	CFastMutex& operator = (const CFastMutex&);
};

#include "Thread.h"

//
// inlines
//
inline void CMutex::lock()
{
#if defined(DEBUG_LOCKS)
  printf("        LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    lockImpl();
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("      LOCKED mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
}


inline void CMutex::lock(long milliseconds)
{
#if defined(DEBUG_LOCKS)
  printf("        LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    if(!tryLockImpl(milliseconds))
      FAILURE(Timeout, "Timeout");
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("      LOCKED mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
}


inline bool CMutex::tryLock()
{
  bool  res;
#if defined(DEBUG_LOCKS)
  printf("    TRY LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    res = tryLockImpl();
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("  %s mutex %s from thread %s.   \n", res ? "    LOCKED" : "CAN'T LOCK",
         lockName, CURRENT_THREAD_NAME);
#endif
  return res;
}


inline bool CMutex::tryLock(long milliseconds)
{
  bool  res;
#if defined(DEBUG_LOCKS)
  printf("    TRY LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    res = tryLockImpl(milliseconds);
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("  %s mutex %s from thread %s.   \n", res ? "    LOCKED" : "CAN'T LOCK",
         lockName, CURRENT_THREAD_NAME);
#endif
  return res;
}


inline void CMutex::unlock()
{
#if defined(DEBUG_LOCKS)
  printf("      UNLOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    unlockImpl();
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to unlock mutex %s from thread %s.\n",
              e.message().c_str(), lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("    UNLOCKED mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
}


inline void CFastMutex::lock()
{
#if defined(DEBUG_LOCKS)
  printf("        LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    lockImpl();
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("      LOCKED mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
}

inline void CFastMutex::lock(long milliseconds)
{
#if defined(DEBUG_LOCKS)
  printf("        LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    if(!tryLockImpl(milliseconds))
      FAILURE(Timeout, "Timeout");
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("      LOCKED mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
}

inline bool CFastMutex::tryLock()
{
  bool  res;
#if defined(DEBUG_LOCKS)
  printf("    TRY LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    res = tryLockImpl();
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("  %s mutex %s from thread %s.   \n", res ? "    LOCKED" : "CAN'T LOCK",
         lockName, CURRENT_THREAD_NAME);
#endif
  return res;
}

inline bool CFastMutex::tryLock(long milliseconds)
{
  bool  res;
#if defined(DEBUG_LOCKS)
  printf("    TRY LOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    res = tryLockImpl(milliseconds);
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to lock mutex %s from thread %s.\n", e.message().c_str(),
                  lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("  %s mutex %s from thread %s.   \n", res ? "    LOCKED" : "CAN'T LOCK",
         lockName, CURRENT_THREAD_NAME);
#endif
  return res;
}

inline void CFastMutex::unlock()
{
#if defined(DEBUG_LOCKS)
  printf("      UNLOCK mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
  try
  {
    unlockImpl();
  }

  catch(CException & e)
  {
    FAILURE_3(Thread,
              "Locking error (%s) trying to unlock mutex %s from thread %s.\n",
              e.message().c_str(), lockName, CURRENT_THREAD_NAME);
  }

#if defined(DEBUG_LOCKS)
  printf("    UNLOCKED mutex %s from thread %s.   \n", lockName,
         CURRENT_THREAD_NAME);
#endif
}

#define MUTEX_LOCK(mutex)         mutex->lock(LOCK_TIMEOUT_MS)
#define MUTEX_READ_LOCK(mutex)    mutex->readLock(LOCK_TIMEOUT_MS)
#define MUTEX_WRITE_LOCK(mutex)   mutex->writeLock(LOCK_TIMEOUT_MS)
#define MUTEX_UNLOCK(mutex)       mutex->unlock()
#define SCOPED_M_LOCK(mutex)      CMutex::CScopedLock L_##__LINE__(mutex)
#define SCOPED_FM_LOCK(mutex)     CFastMutex::CScopedLock L_##__LINE__(mutex)

#endif // Foundation_Mutex_INCLUDED
