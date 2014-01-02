/*
 * semaphore.hpp
 *
 *  Created on: Jan 2, 2014
 *      Author: VBOX
 */

#ifndef SEMAPHORE_HPP_
#define SEMAPHORE_HPP_

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>

class Semaphore
{
private:
    boost::mutex mutex_;
    boost::condition_variable condition_;
    unsigned long count_;

public:
    Semaphore()
    {
    	count_ = 0;
    }
    Semaphore(const Semaphore &sem) {
    	count_ = sem.count_;
    }

    void notify()
    {
        boost::mutex::scoped_lock lock(mutex_);
        ++count_;
        condition_.notify_one();
    }

    void wait()
    {
        boost::mutex::scoped_lock lock(mutex_);
        while(!count_)
            condition_.wait(lock);
        --count_;
    }
    bool timedWait(uint32_t microSeconds) {
    	boost::mutex::scoped_lock lock(mutex_);
    	boost::system_time tm = boost::get_system_time() + boost::posix_time::microseconds(microSeconds);
        while(!count_) {
        	if(!condition_.timed_wait(lock, tm)) {
        		return false;
        	}
        }
        --count_;
        return true;
    }
};


#endif /* SEMAPHORE_HPP_ */
