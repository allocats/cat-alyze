# cat-alayze

My personal build system for C projects. Why? You might ask, well I got tired of writing makefiles over and over again, and CMake was not very appealing to me. I love working in Rust, so I thought why not build a Cargo like system for C! It is multithreaded, allowing for parallel compilation, it makes use of AVX2 SIMD as well as my own AVX2 powered arena allocator. I will add SSE and manual vectorisation as fallbacks for SIMD operations, I want to finish rewriting the lexer into a SIMD powered lexer first, and improve the overall performance. General improvements to compatability and fallbacks will come in the future as well.

## Quick Start

### Create a New Project
```
catalyze new myproject
cd myproject
```

### Initialize in Existing Directory
```
catalyze init
```

### Build Your Project
```
catalyze build           # Build all targets
catalyze build release   # Build specific target
```

### Run Your Project
```
catalyze run            # Run all executable targets
catalyze run myapp      # Run specific target
```

## Configuration

Catalyze uses `.cat` configuration files

```
config {
    compiler: clang
    build_dir: build/
    default_flags: -Wall -Wextra
}

target executable myapp {
    sources: src/main.c
    flags: -O3
    output: build/bin/myapp
}

target debug myapp_debug {
	sources: src/main.c
	flags: -O0 -g3 -fsanitize=address -Weverything
	output: build/debug/myapp_debug
}

target test myapp_tests {
    sources: tests/test_main.c
    flags: -g -Weverything
    output: build/test/myapp_tests
}

target static_lib myapp_lib {
    sources: src/lib.c src/utils.c
    flags: -fPIC
    output: build/lib/libmylib.a
}
```

### Configuration Options

#### Global Config
- `compiler`: The compiler to use (gcc, clang, etc.)
- `build_dir`: Directory for build objects
- `default_flags`: Flags applied to all targets

#### Target Types
- `executable`: Standard executable programs
- `test`: Test executables 
- `static_lib`: Static libraries (.a files)
- `shared_lib`: Shared libraries (.so files)
- `debug`: Debug builds with debugging symbols

#### Target Options
- `sources`: Source files to compile
- `flags`: Additional compiler flags for this target
- `output`: Output path and filename

## Commands

### Project Management
```
catalyze new <project_name>    # Create new project
catalyze init                  # Initialize in current directory
```

### Building
```
catalyze build [target]        # Build specific target or all targets
catalyze run [target]          # Build and run executable targets
catalyze test [target]         # Build and run test targets
catalyze debug [target]        # Build and run debug targets
```

### Help
```
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
    output: build/bin/hello
}
```

## Roadmap

#### Planned

- Testing and a test framework
- Static & Shared library target support
- Optimised building, only rebuild when needed to
