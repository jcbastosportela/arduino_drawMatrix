/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            src                                                                                                      *
 * @file      AsyncTasker copy.hpp                                                                                     *
 * @brief     Implements asynchronous task scheduling and event loop for Arduino.                                      *
 * @date      Sun Aug 24 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#ifndef AsyncTasker_h
#define AsyncTasker_h

#include <cstdint>
#include <functional>
#include <list>

class AsyncTasker {
  public:
    using Callback = std::function<void(uint64_t, uint64_t &, bool &)>;

    /**
     * @brief Schedule a function with a time delay without blocking your loop.
     * In order for this to work, you must call `AsyncTasker::runEventLoop` in your loop function to update
     * the event loop.
     * DO NOT pass a function that yields using `delay()`. It will still stop execution.
     *
     * @param executionTime the time in milliseconds after which the function should be executed
     * @param func the function to be executed
     * @param repeat if true, the function will be executed repeatedly at the specified interval
     */
    static void schedule(uint64_t executionTime, Callback func, bool repeat = false);

    /**
     * @brief Run the event loop and execute scheduled tasks.
     */
    static void runEventLoop();

  private:
    /**
     * @brief Structure to store scheduled tasks information.
     */
    struct ScheduledTask {
        /**
         * @brief Construct a ScheduledTask.
         * @param execTime Time in milliseconds after which the function should be executed.
         * @param cb Callback function to execute.
         * @param rep If true, the function will repeat at the specified interval.
         */
        ScheduledTask(uint64_t execTime, Callback cb, bool rep = false);
        uint64_t executionTime = 0; ///< Time after which the function should be executed
        uint64_t startTime = 0;     ///< Time when the task was scheduled
        Callback func = nullptr;    ///< Function to execute
        bool repeat = false;        ///< If true, the function repeats
    };

    /**
     * @brief Array to store scheduled tasks.
     */
    static std::list<ScheduledTask> scheduledTasks;
};

#endif
