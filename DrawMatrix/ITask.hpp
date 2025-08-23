/**
 * --------------------------------------------------------------------------- *
 *            DrawMatrix                                                       *
 * @file      ITask.hpp                                                        *
 * @brief     TODO: Add brief description                                      *
 * @date      Sat Aug 23 2025                                                  *
 * @author    Joao Carlos Bastos Portela (you@you.you)                         *
 * @copyright 2025 - 2025 Joao Carlos Bastos Portela                           *
 * @license   <<licensename>>                                                  *
 * --------------------------------------------------------------------------- *
 */

#ifndef DRAWMATRIX_ITASK
#define DRAWMATRIX_ITASK

#include <cstdint>

/**
 * @brief Interface for a scheduled task in the DrawMatrix system.
 *
 * Classes implementing ITask can be scheduled and executed periodically.
 */
struct ITask {
    /**
     * @brief Execute the task logic.
     * @param t Current time in milliseconds.
     * @param d Reference to delay until next execution (output).
     * @param repeat Reference to repeat flag (output).
     */
    virtual void execute(uint64_t t, uint64_t &d, bool &repeat) = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~ITask() = default;
};

#endif /* DRAWMATRIX_ITASK */
