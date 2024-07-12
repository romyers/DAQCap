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
         * 
         * @param priority The priority of the task. Higher values indicate 
         * higher priority. Tasks with the same priority are executed in the 
         * order they were assigned.
         */
        void assignTask(T task, int priority = 0);

        /**
         * @brief Removes any tasks that have not yet been started and notifies
         * the worker thread to terminate after the current task is finished.
         * 
         * @note This method is called automatically by the destructor. The 
         * destructor will not wait for the worker thread to terminate.
         * @note If terminated has been called, assignTask() will have no effect.
         */
        void terminate();

        /**
         * @brief Waits for the worker thread to finish executing its current
         * task and then joins the worker thread.
         */
        void join();

    private:

        struct PriorityTask {

            PriorityTask(T task, int priority) 
                : task(std::move(task)), priority(priority) {}

            PriorityTask(const PriorityTask &other) = delete;
            PriorityTask &operator=(const PriorityTask &other) = delete;

            PriorityTask(PriorityTask &&other) noexcept
                : task(std::move(other.task)), priority(other.priority) {}

            PriorityTask &operator=(PriorityTask &&other) noexcept {
                task = std::move(other.task);
                priority = other.priority;
                return *this;
            }

            T task;
            int priority;

        };

        struct TaskComparator {

            bool operator()(const PriorityTask &lhs, const PriorityTask &rhs) {
                return lhs.priority < rhs.priority;
            }

        };

        std::thread t;

        std::priority_queue<
            PriorityTask, 
            std::vector<PriorityTask>, 
            TaskComparator
        > tasks;

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

            if(isTerminated) {
                lock.unlock();
                break;
            }

            // NOTE: We should know that tasks.size() > 0 thanks to the 
            //       condition we gave to cv.wait().

            // Get the next task
            // NOTE: Because priority_queue::top() forces copy construction
            //       on us by returning const T&, we have to do a 
            //       const_cast so we can bind it to a T&& and move it.
            //       This may leave top() in a broken state, so we must
            //       pop() it immediately and make sure that nothing else
            //       happens to the queue in between.
            // See: https://stackoverflow.com/a/20149745
            PriorityTask task = std::move(
                const_cast<PriorityTask&>(tasks.top())
            );
            tasks.pop();

            lock.unlock();

            // TODO: Remember that task() might throw an exception.
            task.task();

        }

    }) {

}

template<typename T>
DAQThread::Worker<T>::~Worker() {

    terminate();

}

template<typename T>
void DAQThread::Worker<T>::assignTask(T task, int priority) {

    if(isTerminated) return;

    std::unique_lock<std::mutex> lock(m);

    tasks.emplace(std::move(task), priority);

    lock.unlock();

    // Tell the worker thread to start the task
    cv.notify_one();

}

template<typename T>
void DAQThread::Worker<T>::terminate() {

    std::unique_lock<std::mutex> lock(m);

    while(!tasks.empty()) {

        tasks.pop();

    }

    isTerminated = true;

    lock.unlock();

    // Ensure that the worker thread will notice that it's time to terminate
    cv.notify_all();

}

template<typename T>
void DAQThread::Worker<T>::join() {

    t.join();

}