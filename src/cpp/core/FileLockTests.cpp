/*
 * FileLocktests.cpp
 *
 * Copyright (C) 2009-14 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include <core/FileLock.hpp>

#include <core/Error.hpp>
#include <core/FilePath.hpp>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#define RSTUDIO_NO_TESTTHAT_ALIASES
#include <tests/TestThat.hpp>

namespace rstudio {
namespace core {
namespace tests {

int s_lockCount = 0;
boost::thread_group s_threads;
boost::mutex s_mutex;
FilePath s_lockFilePath("/tmp/rstudio-test-lock");
   
void acquireLock(std::size_t threadNumber)
{
   LinkBasedFileLock lock;
   Error error = lock.acquire(s_lockFilePath);
   if (error == Success())
   {
      s_mutex.lock();
      std::cout << "Lock acquired by thread: #" << threadNumber << std::endl;
      ++s_lockCount;
      s_mutex.unlock();
   }
}
   
TEST_CASE("File Locking")
{
   SECTION("A link-based lock can only be acquired once")
   {
      Error error;
      
      LinkBasedFileLock lock1;
      LinkBasedFileLock lock2;
      
      error = lock1.acquire(s_lockFilePath);
      if (error)
         LOG_ERROR(error);
      
      CHECK((error == Success()));
      
      error = lock2.acquire(s_lockFilePath);
      if (error)
         LOG_ERROR(error);
      
      CHECK_FALSE((error == Success()));
      
      // release and re-acquire
      error = lock1.release();
      if (error)
         LOG_ERROR(error);
      
      CHECK((error == Success()));
      
      error = lock2.acquire(s_lockFilePath);
      if (error)
         LOG_ERROR(error);
      
      CHECK((error == Success()));
      
      // clean up the lockfile when we're done
      s_lockFilePath.removeIfExists();
   }
   
   SECTION("Only one thread successfully acquires link-based file lock")
   {
      for (std::size_t i = 0; i < 1000; ++i)
         s_threads.create_thread(boost::bind(acquireLock, i));
      
      s_threads.join_all();
      CHECK(s_lockCount == 1);
      
      // clean up
      s_lockFilePath.removeIfExists();
   }
}

} // end namespace tests
} // end namespace core
} // end namespace rstudio
