# The Mars Programming Language

Mars is a statically-typed, procedural language for kernel and embedded programming. 
Mars focuses on low-level control, with emphasis on code configuration.

This repository is also home to Iron, a lightweight and low-level compiler backend. Iron is 
developed alongside Mars, but can be built and used separately in other projects.

## Why?

Mars was born out of the desire for a language that *feels* like C (no hidden mechanics, minimal runtime), but without all the legacy baggage that C comes with today (obtuse syntax, incompatible standards, 
unportable compiler-specific features).

## Building
### Prerequisites:
- C Compiler

To build the Mars compiler, compile/run `mbuild.c`:
```shell
cc mbuild.c -o mbuild -Isrc
./mbuild mars -release
```
To build Iron, use `mbuild` as well:
```shell
./mbuild iron -release        # build as a standalone application
./mbuild iron static -release # build as a static library
```
Use `mbuild` with no arguments to see additional options, such as optimization and compiler flag control.
