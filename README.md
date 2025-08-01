# Teeny Tiny BASIC Compiler

A compiler for the TINY BASIC language specification with some additional features. This project implements a lexical analyzer and file processing system for parsing BASIC programs.

## What is TINY BASIC?

TINY BASIC is a simplified version of the BASIC programming language, designed to be minimal yet functional. This compiler supports standard BASIC constructs including:

- Variables and assignments (`LET`)
- Control flow (`IF`, `THEN`, `ELSE`, `WHILE`)
- Input/output operations (`PRINT`, `INPUT`)
- Labels and jumps (`GOTO`)
- Arithmetic and logical operators
- String and numeric literals

## System Requirements

- **Operating System**: Linux, macOS, or Unix-like system
- **Compiler**: GCC with C99 support
- **Memory**: Minimal requirements (uses address sanitizer for debugging)

## Building the Project

The project uses a Makefile for building. Several build targets are available:

### Setting Up For IDE 

Sets up files which are needed for LSP support for include directories. This should be re-ran everytime any external dependencies change. The user must have `bear` installed.

```bash
make setup
```

### Release Build
```bash
make
```
Builds the optimized release version (`teeny`) in `./builds/release/`

### Debug Build
```bash
make debug
```
Builds the debug version (`teeny-debug`) with full debugging symbols and no optimization in `./builds/debug/`

### Clean Build
```bash
make clean
```
Removes all build artifacts

### Formatting the Codebase 

Formats the whole codebase according to the clang-format file. Must have clang-format installed locally. 
```bash 
make format
```

## Running the Compiler

Currently, the compiler reads BASIC source files and processes them:

```bash
./builds/release/teeny <filename.basic>
```

For debugging:
```bash
./builds/debug/teeny-debug <filename.basic>
```

## Testing

The project includes comprehensive tests using the Criterion testing framework.

### Installing Criterion (Ubuntu/Debian)
```bash
sudo apt-get install libcriterion-dev
```

### Running Tests
```bash
make test
```

This builds and runs the complete test suite.

## Git Hooks

The project includes git hooks to maintain code quality and catch issues early. The hooks automatically:

- Check code formatting with clang-format
- Build the project before commits and pushes
- Run the test suite before pushing
- Validate that code compiles without errors

### Installing Hooks

To install the git hooks, run:

```bash
bash ./hooks/install.sh
```

This sets up automatic checks that run before commits and pushes. See `hooks/README.md` for more details.

## Project Structure

```
extern/             # External Libraries (stb, etc.)
src/                # Source code organized by compiler phases
├── main.c          # Entry point and main driver
├── common/         # Shared utilities and data structures
├── core/           # Core compiler infrastructure
├── frontend/       # Frontend compilation phases (source → AST)
│   ├── lexer/          # Lexical analysis (tokenization)
│   └── parser/         # Syntax analysis (parsing)
├── ast/            # Abstract Syntax Tree & utilities
├── middleend/      # Middle-end: AST analysis & optimization
├── backend/        # Backend compilation phases (AST → target code)
└── debug/          # Debugging macros and diagnostic utilities
hooks/              # Contains pre-push and pre-commit hooks, as well as an installation script for them
tests/              # Criterion test suites
examples/           # Sample BASIC programs to test on
builds/             # Build artifacts (release and debug versions)
```

## Developer Defines and Macros

The compiler includes several debugging and utility macros defined in `src/dz_debug.h` for development and debugging purposes.

### Compilation Flags

These flags can be defined during compilation to enable certain things

- **`DZ_DEBUG`** - Master debug flag that enables all debugging features when defined. Automatically test to 1 when running `make debug`
- **`DZ_TESTING`** - Testing flag that is automatically set to 1 when running `make test`, allows conditional compilation of test-specific code
  
### Debug Control Defines

These macros control various debugging features:

- **`DZ_ENABLE_ASSERTS`** - Controls whether assertion macros are active (enabled when `DZ_DEBUG` is set)
- **`DZ_ENABLE_DEBUGBREAK`** - Controls whether debug breaking is enabled (enabled when `DZ_DEBUG` is set)
- **`DZ_ENABLE_LOGS`** - Controls whether logging macros are active (enabled when `DZ_DEBUG` is set, error logging always available)

### Logging Macros

Hierarchical logging system with file, function, and line number information:

- **`DZ_TRACE(...)`** - Lowest level debugging information
- **`DZ_INFO(...)`** - General informational messages
- **`DZ_WARN(...)`** - Warning messages for potentially problematic situations
- **`DZ_WARNNO(...)`** - Warning messages that also display the current `errno` value
- **`DZ_ERROR(...)`** - Error messages (always available, regardless of `DZ_ENABLE_LOGS`)
- **`DZ_ERRNO(...)`** / **`DZ_ERRORNO(...)`** - Error messages that also display the current `errno` value

All logging macros accept printf-style format strings and arguments.

### Assertion Macros

Runtime and compile-time assertion utilities:

- **`DZ_ASSERT(condition [, message, ...])`** - Runtime assertion that checks a condition and optionally displays a custom message on failure
- **`DZ_STATIC_ASSERT(condition, message)`** - Compile-time assertion for constant expressions

Enabled only when `DZ_ENABLE_ASSERTS` or `DZ_DEBUG` is set.

### Control Flow Macros

- **`DZ_DEBUGBREAK(...)`** - Triggers a debug break (raises `SIGTRAP`) when debugging is enabled
- **`DZ_THROW(...)`** - Logs an error message, triggers a debug break, and terminates the program

### Usage Examples

```c
// Utility macros
int arr[] = {1, 2, 3, 4, 5};
int len = array_size(arr);  // len = 5
int maximum = max(10, 20); // maximum = 20

// Testing conditional compilation
#ifdef DZ_TESTING
    // This code only runs during tests
    printf("Running in test mode\n");
    enable_debug_output();
#endif

// Logging
DZ_INFO("Starting compilation of %s", filename);
DZ_WARN("Deprecated syntax detected on line %d", line_num);
DZ_ERROR("Failed to open file: %s", filename);

// Assertions
DZ_ASSERT(ptr != NULL, "Pointer cannot be null");
DZ_STATIC_ASSERT(sizeof(int) == 4, "This code assumes 32-bit integers");

// Error handling
if (file_error) {
    DZ_THROW("Critical file system error occurred");
}
```

## Example Programs

Check the `examples/` directory for sample BASIC programs that demonstrate the supported syntax.
