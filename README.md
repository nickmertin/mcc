# mcc

Mertin Compiler Collection - a compiler project that started as a grade 12 CS independent study unit project. Currently the only working compiler generates x86 assembly code to check a string against a compile-time-provided regular 
expression; the expression is parsed by the compiler, and a section of assembly is generated for each metacharacter. It works most consistently piping code through standard input/output (with the `--pipe` option).
