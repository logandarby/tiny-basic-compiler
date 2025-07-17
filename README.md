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

## Project Structure

```
src/                # Source
tests/              # Criterion test suites
examples/           # Sample BASIC programs to test on
```

## Example Programs

Check the `examples/` directory for sample BASIC programs that demonstrate the supported syntax.
