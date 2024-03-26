# ccloc

This is a command line utility for POSIX systems that can count lines of code.

I would like to note that I do not write a lot of C, so this has really been a learning experience for me. There probably are memory management mistakes, design mistakes.. etc. [Issues](https://github.com/Struck713/ccloc/issues) are always accepted. Please feel free to open one if you see a mistake. 

## Features

These are the features I want to support with this tool. I will cross these off as I finish them.

- Display
    - ~~Lines of code~~
    - ~~Files~~
    - Whitespace
    - Comments
    - ~~Totals~~
- OS support
    - Windows
    - Mac (should work, haven't tested)
- More languages and extensions
- Releases
- Makefile (maybe, I do love a good ole build.sh)

## Benchmarks

While writing this README, I cloned the [Node.js source](https://github.com/nodejs/node.git) to my laptop running Debian 12 and ran ccloc: It executed in about 175 milliseconds.

I will do actually do these once I finish implementing the core features.

## Language support

Here is a list of the supported languages:

- Assembly
- Bourne shell scripts
- C++ (not all extensions)
- C (not all extensions)
- CMake
- CSS
- D
- Dart
- Docker
- Go
- Groovy
- Haskell
- HTML
- Jai
- Java
- JavaScript
- JSON
- JSX
- Lua
- Makefile
- Markdown
- Perl
- PHP
- Plain
- Python
- R
- Rust
- SQL
- TypeScript