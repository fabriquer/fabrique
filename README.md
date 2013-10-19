fabrique
========
Fabrique is a *meta* build tool.
Like other such tools (CMake, SCons...) it is the first in a two-step process:
it configures your project's build, defined in a programming language designed
for the purpose and generates a build file (Makefile, Ninja file...).
The build file is written in a language designed for *that* purpose, and in
some cases (e.g. Ninja) it does it rather well.
All meta build tools recognise that simply adding more syntax to Make
is not a readable, portable way to describe complex software.

*Unlike* other meta build tools:

 * Fabrique has very few dependencies
     * Fabrique is intended to be an alternative build system for FreeBSD
     * FreeBSD's bootstrap environment does not include Java, M4 or Python
     * it *does* include modern C++ tools (Clang and libc++), plus flex and byacc
 * Fabrique is designed for ease of readability and build maintenance
     * types, not macros
         * macro expansion is a neat trick for small scripts, but:
             * some CMake functions expect "a b c"; others expect "a;b;c" (!?)
             * how many escapes do I need in this Make-expanded inline shell script?
         * large-scale software is held together by *types* and *interfaces*
     * consistency, not context-dependence
         * no "black magic":
             * `CFLAGS=-DFOO make` vs `make CFLAGS=-DFOO`)
             * `make -V CFLAGS` vs `make -V .IMPSRC`
         * all configuration values are immutable and queryable
         * explicit configuration enables reproducible builds
     * functional, not imperative
         * builds should be parallel
         * build descriptions should not include:
             * iterative loops
             * time-varying variables
