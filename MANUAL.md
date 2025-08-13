# Programming Language Manual

## Overview

This is a simple programming language inspired by BASIC, designed for learning and educational purposes. It supports integer arithmetic, basic control flow, and simple input/output operations.

## Data Types

### Integers
All variables store signed integers (typically 32-bit or 64-bit depending on implementation). The language supports:
- Positive numbers: `123`, `456`
- Negative numbers: `-42`, `-1000`
- Zero: `0`

### Strings
Strings are only allowed in `PRINT` statements and cannot be stored in variables. Strings are enclosed in double quotes and support escaped quotes:
- Simple string: `"Hello, World!"`
- String with escaped quotes: `"He said \"Hello\" to me"`

## Variables and Identifiers

### Variable Names
- Must start with a letter (A-Z, a-z)
- Can contain letters, numbers, and underscores after the first character
- Can be any length
- Case-sensitive

**Valid examples:**
- `x`
- `counter`
- `player_score`
- `temp1`
- `myVeryLongVariableName`

**Invalid examples:**
- `123var` (starts with number)
- `_count` (starts with underscore)
- `my-var` (contains hyphen)

### Variable Assignment
Variables are created and assigned using the `LET` statement:
```
LET x = 10
LET counter = 0
LET result = x + counter
```

Variables can be reassigned:
```
LET x = 5
LET x = x + 1    // x is now 6
```

## Expressions and Operators

### Arithmetic Operators
Listed in order of precedence (highest to lowest):

1. **Unary operators**: `+`, `-`
   - `+5` (positive)
   - `-x` (negation)

2. **Multiplication and Division**: `*`, `/`
   - `5 * 3` (multiplication)
   - `10 / 2` (integer division)

3. **Addition and Subtraction**: `+`, `-`
   - `5 + 3` (addition)
   - `10 - 2` (subtraction)

### Comparison Operators
Used in `IF` and `WHILE` statements:
- `==` (equal to)
- `!=` (not equal to)
- `>` (greater than)
- `>=` (greater than or equal to)
- `<` (less than)
- `<=` (less than or equal to)

## Statements

### PRINT Statement
Outputs text or numbers to the console:
```
PRINT "Hello, World!"
PRINT x
PRINT x + 5
```

### INPUT Statement
Reads a value from the user into a variable:
```
INPUT x
PRINT "You entered: "
PRINT x
```

**Important INPUT Caveats:**
- If the user enters a valid integer (e.g., `123`), that number is stored
- If the user enters text with leading digits (e.g., `123abc`), only the numeric portion is stored (`123`)
- If the user enters pure text (e.g., `hello`), the ASCII value of the first character is stored (e.g., `104` for 'h')
- Empty input stores `0`

### IF Statement
Conditional execution of code blocks:
```
IF condition THEN
    // statements to execute if condition is true
ENDIF
```

Example:
```
IF x > 10 THEN
    PRINT "x is greater than 10"
    LET y = x * 2
ENDIF
```

### WHILE Statement
Repeats a block of code while a condition is true:
```
WHILE condition REPEAT
    // statements to repeat
ENDWHILE
```

Example:
```
LET counter = 1
WHILE counter <= 5 REPEAT
    PRINT counter
    LET counter = counter + 1
ENDWHILE
```

### LABEL and GOTO Statements
Create labels and jump to them:

**LABEL** creates a named location in the program:
```
LABEL start
```

**GOTO** jumps to a label:
```
GOTO start
```

Example:
```
LET x = 1
LABEL loop
PRINT x
LET x = x + 1
IF x <= 5 THEN
    GOTO loop
ENDIF
```

## Comments 

Comments start with the `REM` keyword, and continue until the end of line.

Example:
```
LET x = 1 REM This is an assignment 
REM Increment x
LET x = x + 1
```

## Error Handling

This compiler offers comprehensive error handling for Lexical and Semantic errors.

## Language Limitations

1. **No arrays or complex data structures**
2. **No functions or subroutines** (only GOTO for control flow)
3. **No string variables** (strings only in PRINT statements)
4. **Integer arithmetic only** (no floating-point numbers)
5. **No string manipulation** or concatenation

## Complete Example: Fibonacci Calculator

Please see [This code Example](./examples/fibonacci.basic)

