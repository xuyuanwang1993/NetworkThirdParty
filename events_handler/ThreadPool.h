/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
*compile with std=c++11
* link with  -pthread
* */
#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <mutex>
#include <condition_variable>
#include<thread>
#include<future>
#include<vector>
#include<deque>
#include<queue>//priority_queue
#include <functional>//bind
namespace micagent {
using std::mutex;
template <typename _Callable, typename... _Args>
using invoke_result_t=typename std::result_of< _Callable( _Args...)>::type;
template <typename _Callable, typename... _Args>
using invoke_future_t = std::future<invoke_result_t<_Callable,_Args ...>>;
using clock_t = std::chrono::steady_clock;
class ThreadPool final{
public:
    static ThreadPool&Instance();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    int count() const;
    void spawn(int count = 1);
    void join();
    void run();
    bool runOne();

    template <typename _Callable, typename... _Args>
    auto enqueue(_Callable &&f, _Args &&...args) -> invoke_future_t<_Callable, _Args...>{
        return schedule(clock_t::now(), std::forward<_Callable>(f), std::forward<_Args>(args)...);
    }

    template <typename _Callable, typename... _Args>
    auto schedule(clock_t::duration delay, _Callable &&f, _Args &&...args) -> invoke_future_t<_Callable, _Args...>
    {
        return schedule(clock_t::now() + delay, std::forward<_Callable>(f), std::forward<_Args>(args)...);
    }

    template <typename _Callable, typename... _Args>
    auto schedule(clock_t::time_point time, _Callable &&f, _Args &&...args) -> invoke_future_t<_Callable, _Args...>
    {
        std::unique_lock<mutex> lock(mTasksMutex);
        using R=invoke_result_t<_Callable,_Args ...>;
        auto bound = std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...);
        auto task = std::make_shared<std::packaged_task<R()>>([bound]() {
            try {
                return bound();
            } catch (const std::exception &e) {
                /*run failed,print the exception string*/
                //printf("%s\r\n",e.what());
                throw;
            }
        });
        std::future<R> result = task->get_future();

        mTasks.push({time, [task]() { return (*task)(); }});
        mCondition.notify_one();
        return result;
    }
protected:
    ThreadPool() = default;
    ~ThreadPool();

    std::function<void()> dequeue(); // returns null function if joining

private:
    std::vector<std::thread> mWorkers;
    std::atomic<bool> mJoining {false};

    struct Task {
        clock_t::time_point time;
        std::function<void()> func;
        bool operator>(const Task &other) const { return time > other.time; }
        bool operator<(const Task &other) const { return time < other.time; }
    };
    std::priority_queue<Task, std::deque<Task>, std::greater<Task>> mTasks;

    mutable std::mutex mTasksMutex, mWorkersMutex;
    std::condition_variable mCondition;
};

}


#endif // THREADPOOL_H
