# MISRA Compliance

This project C source files use MISRA C 2012 guidelines as underlying coding standard.

For MISRA validation, the project uses cppcheck. Cppcheck covers almost all the MISRA C 2012 rules.
Including the amendments. Together with a C compiler flags `-std=c99 -pedantic-errors -Wall -Wextra -Werror` we get full coverage.

## Deviations

The source code has the following deviations from MISRA C 2012:

| Rule                 | Category | Rationale for Skipping                                                                                     |
|----------------------|----------|-------------------------------------------------------------------------------------------------------------|
| [15.5]: A function should have a single point of exit at the end | Advisory | Early returns can reduce the need for nested conditional statements, making the code more straightforward and easier to read. The intent behind using an early return is to handle edge cases or preconditions at the beginning of the function, allowing the main function logic to remain unindented and clear. |
