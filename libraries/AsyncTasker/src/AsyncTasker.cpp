#include "AsyncTasker.hpp"
#include <Arduino.h>

// Initialize all the array of scheduled tasks with {nullptr,0,0,false}
AsyncTasker::ScheduledTask AsyncTasker::scheduledTasks[20]={AsyncTasker::ScheduledTask{}};

void AsyncTasker::schedule(uint64_t executionTime, Callback f, bool repeat)
{
  // Find an empty slot in the array to store the task
  for (int i = 0; i < sizeof(scheduledTasks) / sizeof(scheduledTasks[0]); ++i)
  {
    if (scheduledTasks[i].func == nullptr)
    {
      scheduledTasks[i] = {f, executionTime, millis(), repeat};
      break;
    }
  }
}

// Run the event loop and execute scheduled tasks.
void AsyncTasker::runEventLoop()
{
  uint64_t currentTime = millis();

  // Iterate through the array and execute tasks if their time has come
  for (size_t i = 0; i < sizeof(scheduledTasks) / sizeof(scheduledTasks[0]); ++i)
  {
    if (scheduledTasks[i].func && currentTime >= (scheduledTasks[i].startTime + scheduledTasks[i].executionTime))
    {
      scheduledTasks[i].func(currentTime, scheduledTasks[i].executionTime);
      // Clear the task after execution
      if(!scheduledTasks[i].repeat)
      {
        scheduledTasks[i] = {nullptr, 0, 0, false};
      }
      else
      {
        scheduledTasks[i].startTime = currentTime;
      }
    }
  }
}
