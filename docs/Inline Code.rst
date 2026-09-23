Using Inline Code
=================

baregear lets you embed native C++ and assembly instructions directly in a
program. Inline blocks are delimited with `#clang` (C/C++) or `#asm`
(assembly) markers and terminated with `#end`.

C/C++ Inline Code
-----------------

Single-line form: code follows the `#clang` marker on the same line.

.. code-block:: baregear

    #clang int squareOf = 5 * 5;
    print squareOf

Multi-line form: the `#clang` marker sits on its own line and the block runs
until `#end` is found at the same or a lesser indentation than the marker.

.. code-block:: baregear

    #clang
    int squareOf = 5 * 5;
    #end

    print squareOf

Output CPP Transpilation Result Without Optimization:

.. code-block:: cpp

    #include <iostream>

    int squareOf = 5 * 5;

    int main() {
        std::cout << squareOf << std::endl;
        return 0;
    }

Inline code is placed at file scope, so it can declare globals or helper code
that the rest of the program uses.

Assembly Inline Code
--------------------

Assembly blocks use the `#asm` marker and the same `#end` terminator:

.. code-block:: baregear

    #asm
    mov eax, 1
    ret
    #end

Output CPP Transpilation Result:

.. code-block:: cpp

    #include <iostream>

    int main() {
        __asm__ volatile("mov eax , 1 ret")
        return 0;
    }

Assembly blocks are transpiled to GCC/LLVM basic `asm` statements. For LLVM
C++ (`clang++`) the emission is optimized as follows:

- The `volatile` qualifier is added so the optimizer never deletes or
  reorders the assembly statement, even when it produces no visible output.
- Double quotes and backslashes inside the assembly are escaped so the
  instruction text survives the surrounding C++ string literal untouched.

This keeps hand-written assembly stable across optimization levels when the
transpiled C++ is compiled with `-O2` or higher.

Notes
-----

- `#end` terminates a multi-line block and must appear at the same or a
  lesser column than the opening marker.
- Inline code is tokenized and re-joined, so quoted string literals and the
  `::` scope operator are currently reconstructed best-effort; avoid them in
  inline blocks for now.
- For assembly, keep instructions space-separated; a single instruction per
  line is recommended.
