/**
 * @file WorkerThread.h
 *
 * @brief Provides a persistent thread that can be given tasks to execute.
 *
 * @author Robert Myers
 * Contact: romyers@umich.edu
 */

// TODO: Expand into a thread pool

#pragma once

#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <future>

namespace DAQThread {

    /**
     * @brief A persistent thread that can be given tasks to execute.
     * 
     * @tparam T The type of task to execute. This should be a callable
     * object.
     */
    template<typename T>
    class Worker {

    public:

        /**
         * @brief Default constructor. Starts the worker thread.
         */
        Worker();
        ~Worker();

        /**
         * @brief Assigns a task to the worker thread. The assigned task will
         * be executed as soon as the worker thread is available.
         * 
         * @param task The task to execute. This should be a callable object.
         */
        void assignTask(T task);

        /**
         * @brief Removes any tasks that have not yet been started and notifies
         * the worker thread to terminate after the current task is finished.
         * 
         * @param wait If true, the method will block until the worker thread
         * has terminated.
         * 
         * @note This method is called automatically by the destructor. The 
         * destructor will not wait for the worker thread to terminate.
         * @note If terminated has been called, assignTask() will have no effect.
         */
        void terminate(bool wait = false);

    private:

        std::thread t;

        std::queue<T> tasks;

        std::mutex m;
        std::condition_variable cv;

        bool isTerminated = false;

    };

} // namespace DAQThread

///////////////////////////////////////////////////////////////////////////////

template<typename T>
DAQThread::Worker<T>::Worker() 
    : t([this]() {

        while(!isTerminated) {

            std::unique_lock<std::mutex> lock(m);

            // Wait until we're told to start a task
            cv.wait(lock, [this]() { 
                return tasks.size() > 0 || isTerminated; 
            });

            // Check for tasks
            // TODO: Is this check redundant after the cv.wait predicate?
            if(tasks.size() > 0) {

                // Get the next task
                T task = std::move(tasks.front());
                tasks.pop();

                lock.unlock();

                // TODO: Remember that task() might throw an exception.
                task();

            } else {

                lock.unlock();

            }

        }

    }) {

}

template<typename T>
DAQThread::Worker<T>::~Worker() {

    terminate();

}

template<typename T>
void DAQThread::Worker<T>::assignTask(T task) {

    if(isTerminated) return;

    std::unique_lock<std::mutex> lock(m);

    tasks.push(std::move(task));

    lock.unlock();

    // Tell the worker thread to start the task
    cv.notify_one();

}

template<typename T>
void DAQThread::Worker<T>::terminate(bool wait) {

    std::unique_lock<std::mutex> lock(m);

    while(!tasks.empty()) {

        tasks.pop();

    }

    isTerminated = true;

    lock.unlock();

    // Ensure that the worker thread will notice that it's time to terminate
    cv.notify_all();

    if(wait) {

        t.join();

    }

}