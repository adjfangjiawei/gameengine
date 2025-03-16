
# Game Engine Step by Step

This is a game engine built from scratch, focusing on modularity and clean architecture.

## Requirements

- CMake 3.15 or higher
- C++20 compatible compiler
- Linux/Unix environment (for now)

## Project Structure

```
Source/
├── Engine/
│   ├── Private/           # Implementation files
│   │   └── Core/         # Core systems implementation
│   └── Public/           # Header files
│       └── Core/         # Core systems interface
```

## Core Systems

Currently implemented systems:
- Basic type definitions (CoreTypes.h)
- Logging system (Log.h)

## Building the Project

To build the project, simply run:

```bash
chmod +x build.sh  # First time only
./build.sh
```

This will:
1. Create a Build directory
2. Configure CMake
3. Build the project
4. Run tests

## Running Tests

Tests are automatically run as part of the build process. You can also run them manually:

```bash
cd Build
ctest --output-on-failure
```

## Next Steps

Planned features:
1. Math Library (Vector, Matrix, Quaternion)
2. Memory Management System
3. Event System
4. Entity Component System (ECS)
