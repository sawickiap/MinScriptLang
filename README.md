MinScriptLang: a minimalistic scripting language with parser and interpreter in a single C++ header file.

**Version:** 0.0.1-development, 2021-11

**License:** MIT - see [LICENSE.txt](LICENSE.txt)

**Status:**
This project is work in progress, around halfway of what is planned.
The repository contains reference implementation of the language parser and interpreter (with most features already working) plus tests.
Standalone interpreter executable, modules with some standard library, samples, and solid documentation will come later.

# Introduction

MinScriptLang is a new, minimalistic scripting language, with main purpose of having reference implementation of its (hand-written) parser and interpreter contained in a single, short C++ header file, which currently has only around 3600 lines of code, while still being useful, supporting modern and convenient features. Main file is [MinScriptLang.hpp](src/MinScriptLang.hpp).

**Language features**

- Syntax similar mostly to JavaScript, also inspired by C++ and other languages in this group, using curly brackets `{}` etc.
- Dynamically typed.
- Two types of collections: `Array` (sequence of values indexed by number) and `Object` (set of values accessed through string key).
- Compatible with JSON - every JSON document is a valid expression of MinScriptLang.
- Object-oriented features (prototype-based) - classes and object instances can be used, although they are all just objects.
- Functional features - data types and functions are first-class values, callable objects with embedded state can be created.
- Extensible with custom variables, functions, and data types.

**Reference implementation**

- Contained within a single, short header file ("STB-style" - implementation needs to be extracted using a special macro).
- Implemented in C++ using some features of C++17. Exceptions are also used.
- The only external dependency is standard C and C++ library.
- Has form of a library that can be easily used by a program to make it sciptable in this language.
- Parser is hand-written - no parser generator is used.
- Interpreter works directly on abstract syntax tree - no intermediate representation or virtual machine bytecode is used.

Following are not the goals of this implementation:

- Maximum performance - the implementation cares for performance where possible (e.g using move semantics), but not at the expense of more complex code. A separate implementation of the language with a more complex, optimizing compiler and some intermediate representation could be created as a separate project.
- Maximum safety - although errors are correctly reported as exceptions, some tricky cases can still crash the host application.

**Other components**

- Standard library in form of individual modules providing functions and classes for... (TBD)
- Executable with standalone interpreter (TBD)
- Extensive set of tests (using [Catch2](https://github.com/catchorg/Catch2) library)
- Extensive documentation (TBD)
- Samples (TBD)

# Potential applications

MinScriptLang has been created as a hobby project.
As such, it has no specific purpose, but it is rather an experiment in creating a language with this unusual prerequisite of having its parser and interpreter as simple and short as possible.
It can still be useful as:

- Educational example
    - To demonstrate how easy it can be to create and implement a programming language.
    - To demonstrate how modern C++ can be used to write a complex program in a safe, convenient, concise way.
- Scripting language to be easily embedded in your program as a library, to make it programmable (scriptable).

# Author

- Adam Sawicki - GitHub: [@sawickiap](https://github.com/sawickiap), homepage and contact: [asawicki.info](https://asawicki.info)
