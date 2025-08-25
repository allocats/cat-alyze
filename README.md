# cat-alayze

My personal build system for C projects. Why? You might ask, well I got tired of writing makefiles over and over again, and CMake was not very appealing to me. I love working in Rust, so I thought why not build a Cargo like system for C! It is not the greatest and far from complete, but it works and something I am rather fond as well proud of. 

## Quick Start

### Create a New Project
```shell
catalyze new myproject
cd myproject
```

### Initialize in Existing Directory
```shell
catalyze init
```

### Build Your Project
```shell
catalyze build           # Build all targets
catalyze build release   # Build specific target
```

### Run Your Project
```shell
catalyze run            # Run all executable targets
catalyze run myapp      # Run specific target
```

## Configuration

Catalyze uses `.cat` configuration files with a clean, intuitive syntax:

```
config {
    compiler: clang
    build_dir: build/
    default_flags: -Wall -Wextra
}

target executable myapp {
    sources: src/main.c src/utils.c
    flags: -O3
    output: build/bin/myapp
}

target test unit_tests {
    sources: src/*.c tests/test_main.c
    flags: -g -DTEST_MODE
    output: build/test/unit_tests
}

target static_lib mylib {
    sources: src/lib.c src/utils.c
    flags: -fPIC
    output: build/lib/libmylib.a
}
```

### Configuration Options

#### Global Config
- `compiler`: The compiler to use (gcc, clang, etc.)
- `build_dir`: Directory for build artifacts
- `default_flags`: Flags applied to all targets

#### Target Types
- `executable`: Standard executable programs
- `test`: Test executables (built and run automatically)
- `static_lib`: Static libraries (.a files)
- `shared_lib`: Shared libraries (.so files)
- `debug`: Debug builds with debugging symbols

#### Target Options
- `sources`: Source files to compile
- `flags`: Additional compiler flags for this target
- `output`: Output path and filename

## Commands

### Project Management
```shell
catalyze new <project_name>    # Create new project
catalyze init                  # Initialize in current directory
```

### Building
```shell
catalyze build [target]        # Build specific target or all targets
catalyze run [target]          # Build and run executable targets
catalyze test [target]         # Build and run test targets
catalyze debug [target]        # Build and run debug targets
```

### Help
```shell
catalyze help                  # Show help message
```

## Examples

### Simple Executable
```
config {
    compiler: gcc
    build_dir: build/
    default_flags: -std=c99 -Wall
}

target executable hello {
    sources: src/main.c
    flags: -O2
    output: build/hello
}
```

## Roadmap

#### Planned

- Testing and a test framework
- Debug target support
- Static & Shared library target support
- File auto discovery 
- Optimised building, only rebuild when needed to
- Package manager like features
