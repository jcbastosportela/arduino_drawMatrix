---
description: Reusable prompt for updating the @brief section in file headers
model: Auto
mode: agent
---

# File Header Brief Adjustment Prompt

## Purpose
This prompt guides the model to locate and update the `@brief` section in the header comment of a source file. The goal is to ensure the brief accurately summarizes the file’s purpose and functionality, while keeping the header Doxygen-compatible and aesthetically clear.

## Instructions
1. Find the file header comment block at the top of the file.
2. Identify the `@brief` tag or brief description line.
3. Replace or update the brief description to provide a concise summary of the file’s purpose.
4. Do not modify other header fields unless explicitly instructed.
5. Preserve formatting and style for Doxygen compatibility.

## Example Header
```cpp
/**
 * --------------------------------------------------------------------------- *
 *            DrawMatrix                                                       *
 * @file      ITask.hpp                                                        *
 * @brief     TODO: Add brief description                                      *
 * @date      Sat Aug 23 2025                                                  *
 * @author    Joao Carlos Bastos Portela (you@you.you)                         *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                          *
 *            <<licensename>>                                                  *
 * --------------------------------------------------------------------------- *
 */
```

## Important
- The file must be modified in place;
- Do not explain what was done, simply say "Updated @brief section".

## Notes
- The brief should be one or two sentences.
- Avoid unnecessary details; focus on the file’s main role.
- If no header exists, create one using the example above; make sure filename, date are set accordingly.
