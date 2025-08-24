/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            src                                                                                                      *
 * @file      AsyncTasker.cpp                                                                                          *
 * @brief     Implements asynchronous task scheduling and event loop for Arduino.                                      *
 * @date      Sun Jul 27 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#include "AsyncTasker.hpp"

#include <Arduino.h>


// Definition of static member
std::list<AsyncTasker::ScheduledTask> AsyncTasker::scheduledTasks = {};

// --------------------------------------------------------------------------------------------------------------------
void AsyncTasker::schedule(uint64_t executionTime, Callback f, bool repeat) {
    scheduledTasks.emplace_back(executionTime, f, repeat);
}

// --------------------------------------------------------------------------------------------------------------------
void AsyncTasker::runEventLoop() {
    uint64_t currentTime = millis();

    // Iterate through the array and execute tasks if their time has come
    for (auto it_task = scheduledTasks.begin(); it_task != scheduledTasks.end(); ) {
        if (it_task->func && currentTime >= (it_task->startTime + it_task->executionTime)) {
            it_task->func(currentTime, it_task->executionTime, it_task->repeat);
            if (!it_task->repeat) {
                // erase returns the next valid iterator
                it_task = scheduledTasks.erase(it_task);
            } else {
                it_task->startTime = currentTime;
                ++it_task;
            }
        } else {
            ++it_task;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
AsyncTasker::ScheduledTask::ScheduledTask(uint64_t execTime, Callback cb, bool rep)
    : executionTime(execTime), startTime(millis()), func(cb), repeat(rep) {}

