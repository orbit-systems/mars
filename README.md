# The Mars Programming Language

Mars is a statically-typed, procedural language for kernel and embedded programming. 
Mars focuses on low-level control, with emphasis on code configuration.

Mars is a work-in-progress and currently only being developed for the [Aphelion ISA](https://github.com/orbit-systems/aphelion). 
Backends to common processors and environments are not out of the question, but significant development of the language and the compiler must happen before then.

## Why?

C is the dominant choice for programming kernels and other embedded applications, 
and it's clear to see why. C code is explicit and translates clearly and effectively into machine code. 
C also does not tangle itself in complex abstractions, has virtually zero overhead. It is one of the fastest, 
if not *the fastest* programming language that exists, making it ideal for kernel applications where speed and 
responsiveness are key.

Mars was born out of a desire to hold on to C's simplicity (no runtime, implicit contexts, built-in allocators, 
large stdlibs, etc.) but dig deeper into the nitty-gritty and give the programmer more low-level control.
