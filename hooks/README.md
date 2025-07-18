# Git Hooks

This directory contains git hooks that help maintain code quality in the project.

## Installation

Run the installation script from the project root:

```bash
./hooks/install.sh
```

## Hooks

### pre-commit
Runs before each commit:
- Checks code formatting with clang-format
- Builds the project to catch compilation errors
- Looks for TODO comments and debug prints

### pre-push
Runs before pushing:
- Builds both release and debug versions
- Runs the test suite
- Warns about uncommitted changes

## Manual Installation

```bash
cp hooks/pre-commit .git/hooks/
chmod +x .git/hooks/pre-commit

cp hooks/pre-push .git/hooks/
chmod +x .git/hooks/pre-push
```

## Requirements

- `make` for building
- `clang-format` for formatting checks (optional)

## Skipping Hooks

Use `--no-verify` to skip hooks when needed:

```bash
git commit --no-verify -m "message"
git push --no-verify
```

## Removing Hooks

```bash
rm .git/hooks/pre-commit .git/hooks/pre-push
``` 