---
description: Reusable prompt for updating missing documentation
model: Auto
mode: agent
---

# Source code documentation completition Prompt

## Purpose

## Instructions
1. Make sure to check the file .github/copilot-instructions.md and see what is told about documentation style
2. Check the already existing documentation and make sure it still makes sense; special focus on functions making sure arguments names match and all arguments are documented 
3. Find symbols, and functions that are not documented
4. Add documentation respecting the documentation style from the instructions (if none follow the project style)
5. If there is documentation not matching the style update it to match the style

## Important
- The file must be modified in place;
- Do not explain what was done, simply say "Updated @brief section".

## Notes
- Only change existing documentation if it describes something wrong;
- You can fix typos