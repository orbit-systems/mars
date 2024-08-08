# The Mars Programming Language

Mars is a statically-typed, procedural language for kernel and embedded programming. 
Mars focuses on low-level control, with emphasis on code configuration.

This repository is also home to Iron, a lightweight and low-level compiler backend. Iron is 
developed alongside Mars, but can be built and used separately in other projects.

## Why?

C isn't an ordinary programming language. It's better categorized as a "high-level assembly".
Operations translate into their machine-code counterparts with almost no magic in-between.
Pointer dereferences are memory accesses, plain and simple, no fanfare. A programmer can look at
C code and know *almost exactly* what the generated machine code will look like.

Mars was born out of the desire for another "high level assembly" language, but without all the 
baggage that C comes with today (obtuse syntax, multiple—sometimes incompatible—standards, 
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
