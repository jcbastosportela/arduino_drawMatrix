#ifndef AsyncTasker_h
#define AsyncTasker_h

#include <cstdint>
#include <functional>

class AsyncTasker
{
public:
  using Callback = std::function<void(uint64_t, uint64_t&, bool&)>;

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
  static void schedule(uint64_t executionTime, Callback func, bool repeat=false);

  // Run the event loop and execute scheduled tasks.
  static void runEventLoop();

private:
  // Structure to store scheduled tasks
  struct ScheduledTask
  {
    Callback func = nullptr;
    uint64_t executionTime = 0;
    uint64_t startTime = 0;
    bool repeat = false;
  };

  // Array to store scheduled tasks
  static ScheduledTask scheduledTasks[20]; // Adjust the size as needed
};

#endif
