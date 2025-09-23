# Mars

> [!WARNING]  
> Mars and its toolchain are **work-in-progress** and not ready for production use.

Mars is a strongly-typed procedural language, built for ergonomic data-oriented design.

This repository is also home to various other compiler projects:

## Iron

Iron is a lightweight but powerful code optimization and generation library, built on a custom SSA-CFG. Iron provides a direct IR to binary pipeline, with a high-level integrated assembler and linker.

Iron is the backend library that powers all the language frontends in this repository.

## Coyote

Coyote is an implementation of the [Jackal](https://github.com/xrarch/newsdk) language by [hyenasky](https://github.com/hyenasky).
Coyote focuses on useful diagnostics, lower memory usage, faster compile times, and high quality code generation compared to the original XR/SDK implementation.

Coyote introduces various features to Jackal for ergonomics (pointers to scalar stack variables, implicit `IN`, `ALIGNOF` and `ALIGNOFVALUE`, etc.) and optimization (`NOALIAS` qualifier, etc.).

## Building

### Linux and WSL

You will need Make, a C compiler with support for C23 (with some GNU extensions), and a linker (`gcc` is the default for both of these).

To build from scratch:
```sh
make clean mars    # for Mars
make clean coyote  # for Coyote
make clean libiron # for Iron as a library
```

To build incrementally, after the initial `clean` build:
```sh
make mars          # for Mars
make coyote        # for Coyote
make libiron       # for Iron as a library
```

### Windows Native

Work in progress! Feel free to bother me about this. It shouldn't be too hard to figure out, it's just not a priority for me right now.
